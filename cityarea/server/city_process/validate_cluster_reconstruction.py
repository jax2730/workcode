import argparse
import pathlib
import sys

import numpy as np
import pandas as pd

sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *

CITY_DIR = DATA_DIR / 'city'


def _safe_div(a: float, b: float) -> float:
    return float(a / b) if b != 0 else 0.0


def _cosine_similarity(a: np.ndarray, b: np.ndarray) -> float:
    # 余弦相似度衡量两个组成向量的“方向是否一致”。
    # 公式为：
    #   cos(a, b) = (a · b) / (||a|| ||b||)
    # 在这里，a 和 b 都是 val_pct 组成向量，因此高 cosine 表示：
    # - 两者主导维度的分布方向接近
    # - 即使个别维度幅度不同，只要整体结构接近，cosine 仍然会较高
    denom = float(np.linalg.norm(a) * np.linalg.norm(b))
    if denom == 0:
        return 0.0
    return float(np.dot(a, b) / denom)


def _topk_overlap(a: np.ndarray, b: np.ndarray, cols: list[str], k: int = 8) -> float:
    # 只比较前 k 个主导维度是否重合，不比较精确百分比。
    # 这个指标回答的问题是：
    # “真实 block 和所属 cluster 的 centroid，是否强调同一批主导 value？”
    a_idx = np.argsort(a)[::-1][:k]
    b_idx = np.argsort(b)[::-1][:k]
    set_a = {cols[i] for i in a_idx if a[i] > 0}
    set_b = {cols[i] for i in b_idx if b[i] > 0}
    union = len(set_a | set_b)
    if union == 0:
        return 1.0
    inter = len(set_a & set_b)
    return float(inter / union)


def _top_signature_from_row(row: pd.Series, cols: list[str], top_n: int = 8) -> str:
    pairs = []
    for col in cols:
        v = float(row.get(col, 0.0))
        if v > 0:
            pairs.append((col.replace('val_pct_', ''), v))
    if len(pairs) == 0:
        return 'empty'
    pairs.sort(key=lambda x: x[1], reverse=True)
    top = pairs[:top_n]
    return ' | '.join([f"{name}:{pct:.2f}%" for name, pct in top])


def validate_reconstruction(
    city_name: str,
    feature_file: str,
    assignment_file: str,
    topk: int,
    good_cosine_threshold: float,
    skip_zero_vectors: bool,
    zero_eps: float,
    verbose: bool = True,
):
    city_dir = CITY_DIR / city_name
    feature_path = city_dir / feature_file
    assign_path = city_dir / assignment_file

    if not feature_path.exists():
        raise FileNotFoundError(f'Feature matrix not found: {feature_path}')
    if not assign_path.exists():
        raise FileNotFoundError(f'Assignments not found: {assign_path}')

    if verbose:
        print(f"📥 Loading feature matrix: {feature_path}")
    feature_df = pd.read_csv(feature_path)
    if verbose:
        print(f"📥 Loading assignments: {assign_path}")
    assign_df = pd.read_csv(assign_path)

    if '_block_id' not in feature_df.columns:
        raise ValueError('Feature matrix must contain _block_id')
    if '_block_id' not in assign_df.columns or 'cluster_id' not in assign_df.columns:
        raise ValueError('Assignments must contain _block_id and cluster_id')

    val_cols = [c for c in feature_df.columns if str(c).startswith('val_pct_')]
    if len(val_cols) == 0:
        raise ValueError('No val_pct_* feature columns found in feature matrix')

    merged = feature_df.merge(
        assign_df[['_block_id', 'cluster_id', 'construction_role', 'construction_pct']],
        on='_block_id',
        how='inner',
        validate='one_to_one',
    )

    if len(merged) == 0:
        raise ValueError('No matched rows between feature matrix and assignments')

    for col in val_cols:
        merged[col] = pd.to_numeric(merged[col], errors='coerce').fillna(0.0)

    # cluster prototype 的定义：
    # 对每个 cluster，把其中所有 block 的特征向量逐维求平均。
    # 也就是：
    #   prototype_c = mean(x_i), i 属于 cluster c
    # 这个 centroid 就是后面“重建该类成员”的 archetype 模板。
    centroid_df = merged.groupby('cluster_id', as_index=False)[val_cols].mean()
    centroid_map = {
        int(row['cluster_id']): row[val_cols].to_numpy(dtype=float)
        for _, row in centroid_df.iterrows()
    }

    records = []
    for _, row in merged.iterrows():
        cid = int(row['cluster_id'])
        actual = row[val_cols].to_numpy(dtype=float)
        proto = centroid_map[cid]
        actual_sum = float(np.sum(actual))
        proto_sum = float(np.sum(proto))

        # diff 是逐维残差向量：
        #   diff_j = actual_j - proto_j
        # 基于这个残差，再推导：
        # - MAE：逐维平均绝对误差，反映平均偏离量
        # - RMSE：逐维均方根误差，对大偏差更敏感
        # - cosine：看组成方向是否一致
        # - topk overlap：看主导 value 集合是否一致
        diff = actual - proto
        mae = float(np.mean(np.abs(diff)))
        rmse = float(np.sqrt(np.mean(diff ** 2)))
        cos = _cosine_similarity(actual, proto)
        overlap = _topk_overlap(actual, proto, val_cols, k=topk)

        records.append(
            {
                '_block_id': row['_block_id'],
                'cluster_id': cid,
                'construction_role': row.get('construction_role', 'unknown'),
                'construction_pct': float(row.get('construction_pct', 0.0)),
                'mae': mae,
                'rmse': rmse,
                'cosine_similarity': cos,
                f'top{topk}_overlap': overlap,
                'actual_sum': actual_sum,
                'proto_sum': proto_sum,
            }
        )

    block_eval = pd.DataFrame(records)
    block_eval['is_zero_vector'] = block_eval['actual_sum'] <= float(zero_eps)

    # zero vector 表示：
    # - 在参与验证的维度里，这个 block 几乎没有有效质量
    # - 即 sum(val_pct_*) <= zero_eps
    # 这种样本几乎不携带可比较的组成方向，因此可选择从全局分母中剔除。
    if skip_zero_vectors:
        metric_eval = block_eval[~block_eval['is_zero_vector']].copy()
    else:
        metric_eval = block_eval.copy()

    if len(metric_eval) == 0:
        raise ValueError('No rows left for metric calculation. Try lowering --zero-eps or disable --skip-zero-vectors.')

    # cluster 级评估表回答两个问题：
    # 1. 每个 cluster 的 centroid 能否代表其内部成员
    # 2. 质量问题是集中在少数弱 cluster，还是整体普遍存在
    cluster_eval = (
        metric_eval.groupby('cluster_id', as_index=False)
        .agg(
            block_count=('cluster_id', 'size'),
            mae_mean=('mae', 'mean'),
            mae_p90=('mae', lambda s: float(np.percentile(s, 90))),
            rmse_mean=('rmse', 'mean'),
            cosine_mean=('cosine_similarity', 'mean'),
            cosine_p10=('cosine_similarity', lambda s: float(np.percentile(s, 10))),
            topk_overlap_mean=(f'top{topk}_overlap', 'mean'),
        )
        .sort_values(['cosine_mean', 'mae_mean'], ascending=[False, True])
    )

    # global_eval 把这次聚类压缩成 1 行全局指标。
    # auto_optimize_cluster_count.py 后续就是读取这一行，
    # 再把 cosine / good_pct / overlap / cluster 数等组合成搜索评分。
    global_eval = pd.DataFrame(
        [
            {
                'blocks_total': int(len(block_eval)),
                'blocks_eval': int(len(metric_eval)),
                'blocks_zero_vector': int(block_eval['is_zero_vector'].sum()),
                'clusters': int(metric_eval['cluster_id'].nunique()),
                'mae_mean': float(metric_eval['mae'].mean()),
                'rmse_mean': float(metric_eval['rmse'].mean()),
                'cosine_mean': float(metric_eval['cosine_similarity'].mean()),
                f'top{topk}_overlap_mean': float(metric_eval[f'top{topk}_overlap'].mean()),
                'good_cosine_threshold': float(good_cosine_threshold),
                'good_blocks_pct': float((metric_eval['cosine_similarity'] >= good_cosine_threshold).mean() * 100.0),
                'hard_blocks_cosine_lt_0_60_pct': float((metric_eval['cosine_similarity'] < 0.60).mean() * 100.0),
                'skip_zero_vectors': bool(skip_zero_vectors),
                'zero_eps': float(zero_eps),
            }
        ]
    )

    # hard_cases：最难被所属 cluster centroid 代表的 block
    # best_cases：最接近 archetype 原型的 block
    hard_cases = block_eval.sort_values(['cosine_similarity', 'mae'], ascending=[True, False]).head(200)
    best_cases = block_eval.sort_values(['cosine_similarity', 'mae'], ascending=[False, True]).head(200)

    out_block = city_dir / 'cluster_reconstruction_block_eval.csv'
    out_cluster = city_dir / 'cluster_reconstruction_cluster_eval.csv'
    out_global = city_dir / 'cluster_reconstruction_global_eval.csv'
    out_hard = city_dir / 'cluster_reconstruction_hard_cases.csv'
    out_best = city_dir / 'cluster_reconstruction_best_cases.csv'
    out_class_catalog = city_dir / 'cluster_class_catalog.csv'
    out_block_recon = city_dir / 'block_reconstruction_from_class.csv'

    cluster_counts = merged.groupby('cluster_id', as_index=False).size().rename(columns={'size': 'block_count_total'})
    cons_stats = (
        merged.groupby('cluster_id', as_index=False)
        .agg(
            construction_pct_mean=('construction_pct', 'mean'),
            construction_pct_p90=('construction_pct', lambda s: float(np.percentile(s, 90))),
            construction_dominant_pct=('construction_role', lambda s: float((s == 'construction_dominant').mean() * 100.0)),
            construction_mixed_pct=('construction_role', lambda s: float((s == 'construction_mixed').mean() * 100.0)),
            construction_minor_pct=('construction_role', lambda s: float((s == 'construction_minor').mean() * 100.0)),
        )
    )

    # class_catalog 本质上就是“cluster 原型词典”：
    # - centroid 各维度
    # - block 数量
    # - construction 相关统计
    # - top signature
    # 后续可用于解释、筛选、导出。
    class_catalog = centroid_df.merge(cluster_counts, on='cluster_id', how='left').merge(cons_stats, on='cluster_id', how='left')
    class_catalog['top_signature'] = class_catalog.apply(lambda r: _top_signature_from_row(r, val_cols, top_n=8), axis=1)
    class_catalog = class_catalog.sort_values(['block_count_total', 'cluster_id'], ascending=[False, True])

    centroid_recon = centroid_df.rename(columns={c: c for c in val_cols})
    # block_recon 的含义是：
    # 给每个 block 挂上“自己所属 cluster 的 centroid 百分比向量”。
    # 这样 GIS 导出时，就能把 archetype 的组成信息直接挂回空间对象。
    block_recon = merged[['_block_id', 'cluster_id', 'construction_role', 'construction_pct']].merge(
        centroid_recon,
        on='cluster_id',
        how='left',
        validate='many_to_one',
    )

    block_eval.to_csv(out_block, index=False, encoding='utf-8-sig')
    cluster_eval.to_csv(out_cluster, index=False, encoding='utf-8-sig')
    global_eval.to_csv(out_global, index=False, encoding='utf-8-sig')
    hard_cases.to_csv(out_hard, index=False, encoding='utf-8-sig')
    best_cases.to_csv(out_best, index=False, encoding='utf-8-sig')
    class_catalog.to_csv(out_class_catalog, index=False, encoding='utf-8-sig')
    block_recon.to_csv(out_block_recon, index=False, encoding='utf-8-sig')

    if verbose:
        print(f"✅ Block-level eval: {out_block}")
        print(f"✅ Cluster-level eval: {out_cluster}")
        print(f"✅ Global eval: {out_global}")
        print(f"✅ Hard cases: {out_hard}")
        print(f"✅ Best cases: {out_best}")
        print(f"✅ Class catalog (cluster prototypes): {out_class_catalog}")
        print(f"✅ Block reconstruction from class: {out_block_recon}")

        print("\n📌 Global summary:")
        for key, val in global_eval.iloc[0].to_dict().items():
            if isinstance(val, float):
                print(f"  {key}: {val:.6f}")
            else:
                print(f"  {key}: {val}")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Validate whether cluster labels can reconstruct block composition using centroid prototypes.'
    )
    parser.add_argument('city', nargs='?', default=DEFAULT_CITY, help=f'City name (default: {DEFAULT_CITY})')  # type: ignore
    parser.add_argument('--feature-file', type=str, default='block_cluster_feature_matrix.csv', help='Feature matrix generated by cluster script')
    parser.add_argument('--assignment-file', type=str, default='block_cluster_assignments.csv', help='Assignments generated by cluster script')
    parser.add_argument('--topk', type=int, default=8, help='Top-K features for overlap validation')
    parser.add_argument('--good-cosine-threshold', type=float, default=0.85, help='Threshold used to define good reconstruction')
    parser.add_argument('--skip-zero-vectors', action='store_true', help='Exclude rows whose non-construction feature sum <= zero-eps from metric denominator')
    parser.add_argument('--zero-eps', type=float, default=1e-9, help='Zero-vector epsilon for filtering')
    parser.add_argument('--quiet', action='store_true', help='Suppress verbose per-run logs')

    args = parser.parse_args()

    validate_reconstruction(
        city_name=args.city,
        feature_file=args.feature_file,
        assignment_file=args.assignment_file,
        topk=args.topk,
        good_cosine_threshold=args.good_cosine_threshold,
        skip_zero_vectors=args.skip_zero_vectors,
        zero_eps=args.zero_eps,
        verbose=not args.quiet,
    )
