import argparse
import contextlib
import hashlib
import io
import inspect
import pathlib
import shutil
import sys
from dataclasses import dataclass
from itertools import combinations, product

import numpy as np
import pandas as pd
from sklearn.metrics import adjusted_rand_score

sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
from city_process.cluster_block_archetypes import cluster_blocks
from city_process.validate_cluster_reconstruction import validate_reconstruction

CITY_DIR = DATA_DIR / 'city'


@dataclass
class Weights:
    cosine: float
    good: float
    overlap: float
    cluster_penalty: float
    tiny_penalty: float
    singleton_penalty: float


def _safe_float(v, default=0.0) -> float:
    try:
        return float(v)
    except Exception:
        return float(default)


def _get_good_pct(global_row: pd.Series) -> float:
    if 'good_blocks_pct' in global_row.index:
        return _safe_float(global_row['good_blocks_pct'])
    if 'good_blocks_cosine_ge_0_85_pct' in global_row.index:
        return _safe_float(global_row['good_blocks_cosine_ge_0_85_pct'])
    return 0.0


def _build_score(
    cosine_mean: float,
    good_pct: float,
    overlap_mean: float,
    clusters: int,
    tiny_ratio: float,
    singleton_ratio: float,
    weights: Weights,
) -> float:
    # 自动搜索时使用的复合评分函数。
    # 奖励项：
    # - cosine_mean：平均组成方向相似度越高越好
    # - good_pct：达到“好重建”阈值的 block 越多越好
    # - overlap_mean：主导 top-k value 越一致越好
    # 惩罚项：
    # - log1p(clusters)：簇越多，复杂度惩罚越大
    # - tiny_ratio：过小簇比例高，说明结果可能过碎
    # - singleton_ratio：单点簇比例高，通常是最明显的过拟合信号
    return (
        weights.cosine * cosine_mean
        + weights.good * (good_pct / 100.0)
        + weights.overlap * overlap_mean
        - weights.cluster_penalty * np.log1p(float(clusters))
        - weights.tiny_penalty * tiny_ratio
        - weights.singleton_penalty * singleton_ratio
    )


def _call_cluster_blocks_compat(**kwargs):
    sig = inspect.signature(cluster_blocks)
    accepted = set(sig.parameters.keys())
    filtered = {k: v for k, v in kwargs.items() if k in accepted}
    missing_required = []
    for name, p in sig.parameters.items():
        if p.default is inspect._empty and name not in filtered:
            missing_required.append(name)
    if missing_required:
        raise TypeError(f'cluster_blocks missing required args for current signature: {missing_required}')
    cluster_blocks(**filtered)


def _call_validate_compat(**kwargs):
    sig = inspect.signature(validate_reconstruction)
    accepted = set(sig.parameters.keys())
    filtered = {k: v for k, v in kwargs.items() if k in accepted}
    missing_required = []
    for name, p in sig.parameters.items():
        if p.default is inspect._empty and name not in filtered:
            missing_required.append(name)
    if missing_required:
        raise TypeError(f'validate_reconstruction missing required args for current signature: {missing_required}')
    validate_reconstruction(**filtered)


def _run_with_optional_silence(callable_fn, silent: bool, **kwargs):
    if not silent:
        return callable_fn(**kwargs)
    sink = io.StringIO()
    with contextlib.redirect_stdout(sink), contextlib.redirect_stderr(sink):
        return callable_fn(**kwargs)


def auto_optimize(
    city_name: str,
    long_file: str,
    wide_file: str,
    prefer_wide: bool,
    exclude_values: list[str],
    topk: int,
    good_threshold: float,
    skip_zero_vectors: bool,
    zero_eps: float,
    max_global_list: list[int],
    min_value_pct_list: list[float],
    local_k_min_list: list[int],
    local_k_divisor_list: list[int],
    min_bucket_size_list: list[int],
    seed_list: list[int],
    tiny_cluster_size: int,
    weights: Weights,
    min_value_pct: float | None = None,
    loop_verbose: bool = False,
):
    city_dir = CITY_DIR / city_name
    if not city_dir.exists():
        raise FileNotFoundError(f'City directory not found: {city_dir}')

    if len(max_global_list) == 0:
        raise ValueError('max_global_list is empty')
    if len(seed_list) == 0:
        raise ValueError('seed_list is empty')

    if len(min_value_pct_list) == 0:
        if min_value_pct is None:
            min_value_pct_list = [0.2]
        else:
            min_value_pct_list = [float(min_value_pct)]

    trial_rows = []
    combo_rows = []
    best_score = -1e18
    best_meta: dict | None = None
    partition_cache: dict[str, dict] = {}
    repeated_partition_hits = 0
    unique_partition_count = 0

    out_log_trials = city_dir / 'cluster_auto_optimize_trials.csv'
    out_log_combos = city_dir / 'cluster_auto_optimize_runs.csv'
    out_best_summary = city_dir / 'cluster_auto_optimize_best.csv'

    # 这里用笛卡尔积构造完整参数网格。
    # 每个元素代表 1 组待测试的聚类超参数组合。
    combos = list(
        product(
            max_global_list,
            min_value_pct_list,
            local_k_min_list,
            local_k_divisor_list,
            min_bucket_size_list,
        )
    )
    print(f'🔎 Searching {len(combos)} parameter combos × {len(seed_list)} seeds')

    for combo_id, (max_g, min_val, local_k_min, local_k_div, min_bucket) in enumerate(combos, start=1):
        print(
            f"\n===== Combo {combo_id}/{len(combos)}: max_g={max_g}, min_val={min_val}, "
            f"local_k_min={local_k_min}, local_k_div={local_k_div}, min_bucket={min_bucket} ====="
        )

        combo_trial_rows = []
        label_vectors: list[np.ndarray] = []
        block_ids_ref: np.ndarray | None = None
        best_trial_score = -1e18
        best_trial_seed = None

        for seed in seed_list:
            # 1 次 trial = 1 组参数组合 + 1 个随机种子。
            # 目的：
            # - 同一参数下观察随机初始化带来的波动
            # - 为后续稳定性评估准备多组标签结果
            _run_with_optional_silence(
                _call_cluster_blocks_compat,
                silent=not loop_verbose,
                city_name=city_name,
                long_file=long_file,
                wide_file=wide_file,
                prefer_wide=prefer_wide,
                exclude_values=exclude_values,
                min_value_pct=min_val,
                max_global_clusters=max_g,
                random_state=seed,
                local_k_min=local_k_min,
                local_k_divisor=local_k_div,
                min_bucket_size=min_bucket,
                verbose=False,
            )
            assign_path = city_dir / 'block_cluster_assignments.csv'
            assign_df = pd.read_csv(assign_path)[['_block_id', 'cluster_id']].copy()
            assign_df['_block_id'] = assign_df['_block_id'].astype(str)
            assign_df = assign_df.sort_values('_block_id')

            # part_key 表示“这次 trial 生成的精确分区结果”，
            # 而不是参数组合本身。
            # 如果不同 seed 最终得到完全相同的 _block_id -> cluster_id 映射，
            # 那么后续 validate 的结果也一定一样，因此直接复用缓存。
            part_key = hashlib.md5(
                ('|'.join(assign_df['_block_id'].tolist()) + '||' + '|'.join(assign_df['cluster_id'].astype(str).tolist())).encode('utf-8')
            ).hexdigest()

            if part_key in partition_cache:
                repeated_partition_hits += 1
                cached = partition_cache[part_key]
                g = cached['global_row']
                cluster_eval_df = cached['cluster_eval_df']
                current_ids = cached['current_ids']
                current_labels = cached['current_labels']
            else:
                unique_partition_count += 1
                _run_with_optional_silence(
                    _call_validate_compat,
                    silent=not loop_verbose,
                    city_name=city_name,
                    feature_file='block_cluster_feature_matrix.csv',
                    assignment_file='block_cluster_assignments.csv',
                    topk=topk,
                    good_cosine_threshold=good_threshold,
                    skip_zero_vectors=skip_zero_vectors,
                    zero_eps=zero_eps,
                    verbose=False,
                )

                global_path = city_dir / 'cluster_reconstruction_global_eval.csv'
                cluster_eval_path = city_dir / 'cluster_reconstruction_cluster_eval.csv'

                g = pd.read_csv(global_path).iloc[0]
                cluster_eval_df = pd.read_csv(cluster_eval_path)
                current_ids = assign_df['_block_id'].to_numpy(dtype=str)
                current_labels = assign_df['cluster_id'].to_numpy(dtype=int)
                partition_cache[part_key] = {
                    'global_row': g,
                    'cluster_eval_df': cluster_eval_df,
                    'current_ids': current_ids,
                    'current_labels': current_labels,
                }

            if block_ids_ref is None:
                block_ids_ref = current_ids
                label_vectors.append(current_labels)
            else:
                if not np.array_equal(block_ids_ref, current_ids):
                    # ARI 比较的是“同一批 block 的标签划分”。
                    # 因此不同 trial 在计算 ARI 前，必须先按相同 block 顺序对齐。
                    # 这里统一重排到首个 trial 的 block_ids_ref 顺序。
                    align = assign_df.set_index('_block_id').reindex(block_ids_ref)
                    current_labels = align['cluster_id'].fillna(-9999).astype(int).to_numpy()
                label_vectors.append(current_labels)

            overlap_col = f'top{topk}_overlap_mean'
            overlap_mean = _safe_float(g.get(overlap_col, 0.0))
            cosine_mean = _safe_float(g.get('cosine_mean', 0.0))
            good_pct = _get_good_pct(g)
            clusters = int(_safe_float(g.get('clusters', 0)))

            if len(cluster_eval_df) > 0:
                # tiny_ratio：
                #   所有 cluster 中，block_count < tiny_cluster_size 的比例
                # singleton_ratio：
                #   所有 cluster 中，block_count == 1 的比例
                # 这两个量都用于惩罚“聚类过碎”的方案。
                tiny_ratio = float((cluster_eval_df['block_count'] < tiny_cluster_size).mean())
                singleton_ratio = float((cluster_eval_df['block_count'] == 1).mean())
            else:
                tiny_ratio = 1.0
                singleton_ratio = 1.0

            trial_score = _build_score(
                cosine_mean=cosine_mean,
                good_pct=good_pct,
                overlap_mean=overlap_mean,
                clusters=clusters,
                tiny_ratio=tiny_ratio,
                singleton_ratio=singleton_ratio,
                weights=weights,
            )

            trial_rec = {
                'combo_id': combo_id,
                'seed': seed,
                'max_global_clusters_param': max_g,
                'min_value_pct_param': min_val,
                'local_k_min_param': local_k_min,
                'local_k_divisor_param': local_k_div,
                'min_bucket_size_param': min_bucket,
                'clusters_effective': clusters,
                'cosine_mean': cosine_mean,
                'good_pct': good_pct,
                overlap_col: overlap_mean,
                'tiny_cluster_ratio': tiny_ratio,
                'singleton_cluster_ratio': singleton_ratio,
                'trial_score': trial_score,
            }
            combo_trial_rows.append(trial_rec)
            trial_rows.append(trial_rec)

            print(
                f"  seed={seed} trial_score={trial_score:.6f} cosine={cosine_mean:.4f} good%={good_pct:.2f} "
                f"{overlap_col}={overlap_mean:.4f} clusters={clusters}"
            )

            if trial_score > best_trial_score:
                best_trial_score = trial_score
                best_trial_seed = seed
                shutil.copy2(city_dir / 'block_cluster_assignments.csv', city_dir / 'block_cluster_assignments_combo_best.csv')
                shutil.copy2(city_dir / 'block_cluster_prototypes.csv', city_dir / 'block_cluster_prototypes_combo_best.csv')
                shutil.copy2(city_dir / 'block_cluster_feature_matrix.csv', city_dir / 'block_cluster_feature_matrix_combo_best.csv')
                shutil.copy2(city_dir / 'cluster_reconstruction_global_eval.csv', city_dir / 'cluster_reconstruction_global_eval_combo_best.csv')
                shutil.copy2(city_dir / 'cluster_reconstruction_cluster_eval.csv', city_dir / 'cluster_reconstruction_cluster_eval_combo_best.csv')
                shutil.copy2(city_dir / 'cluster_reconstruction_block_eval.csv', city_dir / 'cluster_reconstruction_block_eval_combo_best.csv')
                shutil.copy2(city_dir / 'cluster_class_catalog.csv', city_dir / 'cluster_class_catalog_combo_best.csv')
                shutil.copy2(city_dir / 'block_reconstruction_from_class.csv', city_dir / 'block_reconstruction_from_class_combo_best.csv')

        if len(combo_trial_rows) == 0:
            continue

        # 稳定性评估：
        # 对同一参数组合下，各 seed 结果两两计算 ARI。
        # ARI = 1 表示两个分区结构完全一致（不要求 cluster 编号相同）。
        # 因此：
        # - ari_mean 越高，说明此参数组合越稳定
        # - ari_std 越低，说明这种稳定性越持续，不依赖偶然 seed
        ari_values = []
        if len(label_vectors) >= 2:
            for i, j in combinations(range(len(label_vectors)), 2):
                ari = adjusted_rand_score(label_vectors[i], label_vectors[j])
                ari_values.append(float(ari))
        ari_mean = float(np.mean(ari_values)) if len(ari_values) > 0 else 1.0
        ari_std = float(np.std(ari_values)) if len(ari_values) > 0 else 0.0

        combo_df = pd.DataFrame(combo_trial_rows)
        overlap_col = f'top{topk}_overlap_mean'
        combo_cos = float(combo_df['cosine_mean'].mean())
        combo_good = float(combo_df['good_pct'].mean())
        combo_overlap = float(combo_df[overlap_col].mean())
        combo_clusters = int(round(float(combo_df['clusters_effective'].mean())))
        combo_tiny = float(combo_df['tiny_cluster_ratio'].mean())
        combo_single = float(combo_df['singleton_cluster_ratio'].mean())

        # 先用跨 seed 的平均重建质量计算基础分，
        # 再显式叠加稳定性奖励/惩罚。
        combo_score = _build_score(
            cosine_mean=combo_cos,
            good_pct=combo_good,
            overlap_mean=combo_overlap,
            clusters=combo_clusters,
            tiny_ratio=combo_tiny,
            singleton_ratio=combo_single,
            weights=weights,
        )
        # 稳定性修正项：
        # - 平均 ARI 高：加分
        # - ARI 波动大：扣分
        combo_score = combo_score + 0.08 * ari_mean - 0.04 * ari_std

        combo_rec = {
            'combo_id': combo_id,
            'max_global_clusters_param': max_g,
            'min_value_pct_param': min_val,
            'local_k_min_param': local_k_min,
            'local_k_divisor_param': local_k_div,
            'min_bucket_size_param': min_bucket,
            'seeds': ','.join([str(s) for s in seed_list]),
            'best_trial_seed': best_trial_seed,
            'clusters_effective': combo_clusters,
            'cosine_mean': combo_cos,
            'good_pct': combo_good,
            overlap_col: combo_overlap,
            'tiny_cluster_ratio': combo_tiny,
            'singleton_cluster_ratio': combo_single,
            'stability_ari_mean': ari_mean,
            'stability_ari_std': ari_std,
            'score': combo_score,
        }
        combo_rows.append(combo_rec)

        print(
            f"  => combo_score={combo_score:.6f} stability_ari_mean={ari_mean:.4f} "
            f"cosine={combo_cos:.4f} good%={combo_good:.2f} clusters={combo_clusters}"
        )

        if combo_score > best_score:
            # 一旦当前组合成为全局最优，就把对应产物整套复制为 bestauto。
            # 这样后续 export 或人工检查时，可以直接使用最优结果，
            # 不必重新回放整个搜索流程。
            best_score = combo_score
            best_meta = combo_rec.copy()
            shutil.copy2(city_dir / 'block_cluster_assignments_combo_best.csv', city_dir / 'block_cluster_assignments_bestauto.csv')
            shutil.copy2(city_dir / 'block_cluster_prototypes_combo_best.csv', city_dir / 'block_cluster_prototypes_bestauto.csv')
            shutil.copy2(city_dir / 'block_cluster_feature_matrix_combo_best.csv', city_dir / 'block_cluster_feature_matrix_bestauto.csv')
            shutil.copy2(city_dir / 'cluster_reconstruction_global_eval_combo_best.csv', city_dir / 'cluster_reconstruction_global_eval_bestauto.csv')
            shutil.copy2(city_dir / 'cluster_reconstruction_cluster_eval_combo_best.csv', city_dir / 'cluster_reconstruction_cluster_eval_bestauto.csv')
            shutil.copy2(city_dir / 'cluster_reconstruction_block_eval_combo_best.csv', city_dir / 'cluster_reconstruction_block_eval_bestauto.csv')
            shutil.copy2(city_dir / 'cluster_class_catalog_combo_best.csv', city_dir / 'cluster_class_catalog_bestauto.csv')
            shutil.copy2(city_dir / 'block_reconstruction_from_class_combo_best.csv', city_dir / 'block_reconstruction_from_class_bestauto.csv')

    trials_df = pd.DataFrame(trial_rows)
    if len(trials_df) > 0:
        trials_df = trials_df.sort_values(['combo_id', 'trial_score'], ascending=[True, False])
    trials_df.to_csv(out_log_trials, index=False, encoding='utf-8-sig')

    runs_df = pd.DataFrame(combo_rows).sort_values('score', ascending=False)
    runs_df.to_csv(out_log_combos, index=False, encoding='utf-8-sig')

    if best_meta is None:
        raise RuntimeError('No valid run produced any score.')

    pd.DataFrame([best_meta]).to_csv(out_best_summary, index=False, encoding='utf-8-sig')

    print('\n✅ Auto optimization finished')
    print(f'✅ Trial log: {out_log_trials}')
    print(f'✅ Combo log: {out_log_combos}')
    print(f'✅ Best summary: {out_best_summary}')
    print(
        '✅ Best artifacts: block_cluster_assignments_bestauto.csv, '
        'cluster_class_catalog_bestauto.csv, block_reconstruction_from_class_bestauto.csv'
    )
    print(
        f"📌 Best param max_global_clusters={int(best_meta['max_global_clusters_param'])}, "
        f"effective_clusters={int(best_meta['clusters_effective'])}, score={best_meta['score']:.6f}"
    )
    print(f"📌 Unique partitions evaluated: {unique_partition_count}; cache hits (repeated partitions): {repeated_partition_hits}")
    if unique_partition_count <= max(1, len(seed_list)):
        print('⚠️ Search space produced very few unique clusterings. Expand parameter ranges or sync latest cluster_block_archetypes.py on server.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Auto-optimize cluster complexity on server by searching and validating multiple cluster settings.'
    )
    parser.add_argument('city', nargs='?', default=DEFAULT_CITY, help=f'City name (default: {DEFAULT_CITY})')  # type: ignore

    parser.add_argument('--long-file', type=str, default='block_value_features_long.csv', help='Long feature file')
    parser.add_argument('--wide-file', type=str, default='block_value_features.xlsx', help='Wide feature file')
    parser.add_argument('--prefer-wide', action='store_true', help='Prefer wide input first')
    parser.add_argument('--exclude-values', nargs='+', default=['construction'], help='Excluded values in clustering')
    parser.add_argument('--min-value-pct-list', type=str, default='0.1,0.2,0.5', help='Comma-separated list, e.g. 0.1,0.2,0.5')

    parser.add_argument('--topk', type=int, default=8, help='TopK overlap metric in validation')
    parser.add_argument('--good-threshold', type=float, default=0.85, help='Good-block cosine threshold')
    parser.add_argument('--skip-zero-vectors', action='store_true', help='Exclude zero vectors from denominator in validation')
    parser.add_argument('--zero-eps', type=float, default=1e-9, help='Zero vector epsilon')

    parser.add_argument('--max-global-list', type=str, default='8,12,16,20,24,28,32', help='Comma-separated max-global-clusters candidates')
    parser.add_argument('--local-k-min-list', type=str, default='2,3', help='Comma-separated local_k_min candidates')
    parser.add_argument('--local-k-divisor-list', type=str, default='50,80,120', help='Comma-separated local_k_divisor candidates')
    parser.add_argument('--min-bucket-size-list', type=str, default='6,8,12', help='Comma-separated min_bucket_size candidates')
    parser.add_argument('--seed-list', type=str, default='13,42,77', help='Comma-separated random seed list for stability')
    parser.add_argument('--tiny-cluster-size', type=int, default=8, help='Cluster smaller than this is tiny')

    parser.add_argument('--w-cosine', type=float, default=0.55, help='Weight for cosine mean')
    parser.add_argument('--w-good', type=float, default=0.30, help='Weight for good block ratio')
    parser.add_argument('--w-overlap', type=float, default=0.15, help='Weight for topK overlap')
    parser.add_argument('--w-cluster-penalty', type=float, default=0.03, help='Penalty for too many clusters')
    parser.add_argument('--w-tiny-penalty', type=float, default=0.08, help='Penalty for tiny-cluster ratio')
    parser.add_argument('--w-singleton-penalty', type=float, default=0.12, help='Penalty for singleton-cluster ratio')
    parser.add_argument('--loop-verbose', action='store_true', help='Show full per-trial logs from clustering/validation internals')

    args = parser.parse_args()

    def _parse_int_list(text: str) -> list[int]:
        return [int(t.strip()) for t in str(text).split(',') if str(t).strip() != '']

    def _parse_float_list(text: str) -> list[float]:
        return [float(t.strip()) for t in str(text).split(',') if str(t).strip() != '']

    max_global_list = sorted(list(set(_parse_int_list(args.max_global_list))))
    min_value_pct_list = sorted(list(set(_parse_float_list(args.min_value_pct_list))))
    local_k_min_list = sorted(list(set(_parse_int_list(args.local_k_min_list))))
    local_k_divisor_list = sorted(list(set(_parse_int_list(args.local_k_divisor_list))))
    min_bucket_size_list = sorted(list(set(_parse_int_list(args.min_bucket_size_list))))
    seed_list = sorted(list(set(_parse_int_list(args.seed_list))))

    auto_optimize(
        city_name=args.city,
        long_file=args.long_file,
        wide_file=args.wide_file,
        prefer_wide=args.prefer_wide,
        exclude_values=args.exclude_values,
        topk=args.topk,
        good_threshold=args.good_threshold,
        skip_zero_vectors=args.skip_zero_vectors,
        zero_eps=args.zero_eps,
        max_global_list=max_global_list,
        min_value_pct_list=min_value_pct_list,
        local_k_min_list=local_k_min_list,
        local_k_divisor_list=local_k_divisor_list,
        min_bucket_size_list=min_bucket_size_list,
        seed_list=seed_list,
        tiny_cluster_size=args.tiny_cluster_size,
        weights=Weights(
            cosine=args.w_cosine,
            good=args.w_good,
            overlap=args.w_overlap,
            cluster_penalty=args.w_cluster_penalty,
            tiny_penalty=args.w_tiny_penalty,
            singleton_penalty=args.w_singleton_penalty,
        ),
        loop_verbose=args.loop_verbose,
    )
