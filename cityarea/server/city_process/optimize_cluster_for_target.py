import argparse
import pathlib
import sys

import numpy as np
import pandas as pd

sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *

CITY_DIR = DATA_DIR / 'city'


def _cosine_similarity(a: np.ndarray, b: np.ndarray) -> float:
    # 与 validate_cluster_reconstruction.py 中相同，
    # 使用余弦相似度衡量两个组成向量的方向一致性。
    denom = float(np.linalg.norm(a) * np.linalg.norm(b))
    if denom == 0:
        return 0.0
    return float(np.dot(a, b) / denom)


def _build_centroids(df: pd.DataFrame, feature_cols: list[str]) -> dict[int, np.ndarray]:
    # 由于这个脚本会不断修改 cluster_id，
    # 因此每轮都必须重新按当前 assignment 计算 centroid。
    # 否则后续“哪个 cluster 最弱”的判断就会基于过期原型。
    centroid_df = df.groupby('cluster_id', as_index=False)[feature_cols].mean()
    return {
        int(row['cluster_id']): row[feature_cols].to_numpy(dtype=float)
        for _, row in centroid_df.iterrows()
    }


def _evaluate(
    df: pd.DataFrame,
    feature_cols: list[str],
    good_threshold: float,
    zero_eps: float,
) -> tuple[pd.DataFrame, pd.DataFrame, dict]:
    centroids = _build_centroids(df, feature_cols)

    rows = []
    for _, row in df.iterrows():
        cid = int(row['cluster_id'])
        actual = row[feature_cols].to_numpy(dtype=float)
        proto = centroids[cid]
        # 这个脚本的局部优化目标主要盯住 cosine：
        # cosine 越高，说明该 block 越能被当前 cluster 的 centroid 代表。
        cos = _cosine_similarity(actual, proto)
        actual_sum = float(np.sum(actual))
        rows.append(
            {
                '_block_id': row['_block_id'],
                'cluster_id': cid,
                'cosine_similarity': cos,
                'actual_sum': actual_sum,
                'is_zero_vector': actual_sum <= zero_eps,
            }
        )

    block_eval = pd.DataFrame(rows)
    eval_df = block_eval[~block_eval['is_zero_vector']].copy()
    if len(eval_df) == 0:
        raise ValueError('No non-zero rows left for optimization metrics.')

    # 这里先按“最值得拆”的优先级排序 cluster：
    # 1. good_rate 低的优先
    # 2. cosine_mean 低的优先
    # 3. 在同样差的情况下，block 更多的大簇优先
    # 因为拆一个大的弱 cluster，通常比拆一个很小的弱 cluster 更可能
    # 对全局指标带来明显改善。
    cluster_eval = (
        eval_df.groupby('cluster_id', as_index=False)
        .agg(
            block_count=('cluster_id', 'size'),
            cosine_mean=('cosine_similarity', 'mean'),
            cosine_p10=('cosine_similarity', lambda s: float(np.percentile(s, 10))),
            good_rate=('cosine_similarity', lambda s: float((s >= good_threshold).mean())),
        )
        .sort_values(['good_rate', 'cosine_mean', 'block_count'], ascending=[True, True, False])
    )

    summary = {
        'blocks_total': int(len(block_eval)),
        'blocks_eval': int(len(eval_df)),
        'blocks_zero_vector': int(block_eval['is_zero_vector'].sum()),
        'clusters': int(df['cluster_id'].nunique()),
        'good_threshold': float(good_threshold),
        'good_rate': float((eval_df['cosine_similarity'] >= good_threshold).mean()),
        'cosine_mean': float(eval_df['cosine_similarity'].mean()),
    }
    return block_eval, cluster_eval, summary


def _pick_cluster_to_split(
    cluster_eval: pd.DataFrame,
    min_split_size: int,
    blocked_clusters: set[int],
) -> int | None:
    # 由于 cluster_eval 已经按“最弱 -> 最强”排好序，
    # 这里直接取第一个满足最小规模且未被封锁的 cluster，
    # 就等价于执行贪心策略：每轮优先拆最弱 cluster。
    candidate = cluster_eval[
        (cluster_eval['block_count'] >= min_split_size)
        & (~cluster_eval['cluster_id'].isin(blocked_clusters))
    ]
    if len(candidate) == 0:
        return None
    return int(candidate.iloc[0]['cluster_id'])


def _safe_signature(row: pd.Series, feature_cols: list[str], top_n: int = 8) -> str:
    pairs = []
    for col in feature_cols:
        v = float(row.get(col, 0.0))
        if v > 0:
            pairs.append((col.replace('val_pct_', ''), v))
    if not pairs:
        return 'empty'
    pairs.sort(key=lambda x: x[1], reverse=True)
    return ' | '.join([f"{name}:{pct:.2f}%" for name, pct in pairs[:top_n]])


def optimize_clustering(
    city_name: str,
    feature_file: str,
    assignment_file: str,
    target_good_pct: float,
    good_threshold: float,
    min_split_size: int,
    max_extra_clusters: int,
    max_iter: int,
    random_state: int,
    zero_eps: float,
):
    try:
        from sklearn.cluster import MiniBatchKMeans
    except Exception as exc:
        raise ImportError(f'scikit-learn is required: {exc}')

    city_dir = CITY_DIR / city_name
    feat_path = city_dir / feature_file
    assign_path = city_dir / assignment_file

    if not feat_path.exists():
        raise FileNotFoundError(f'Feature matrix not found: {feat_path}')
    if not assign_path.exists():
        raise FileNotFoundError(f'Assignment file not found: {assign_path}')

    print(f"📥 Loading features: {feat_path}")
    feat_df = pd.read_csv(feat_path)
    print(f"📥 Loading assignments: {assign_path}")
    assign_df = pd.read_csv(assign_path)

    val_cols = [c for c in feat_df.columns if str(c).startswith('val_pct_')]
    if len(val_cols) == 0:
        raise ValueError('No val_pct_* feature columns found')

    cols = ['_block_id', 'cluster_id']
    optional_cols = [c for c in ['sector_id', 'dominant_value', 'cluster_signature', 'construction_pct', 'construction_role'] if c in assign_df.columns]
    work = feat_df[['_block_id'] + val_cols].merge(assign_df[cols + optional_cols], on='_block_id', how='inner')

    if len(work) == 0:
        raise ValueError('No matched rows between feature and assignment files')

    for col in val_cols:
        work[col] = pd.to_numeric(work[col], errors='coerce').fillna(0.0)

    target_good_rate = target_good_pct / 100.0
    # 新生成的 cluster 使用单调递增的新编号。
    # 这样可以保持已有 cluster_id 尽量稳定，便于：
    # - diff 对比
    # - 后续人工跟踪
    # - 避免每轮都大规模重编号
    next_cluster_id = int(work['cluster_id'].max()) + 1
    blocked_clusters: set[int] = set()
    logs: list[dict] = []

    for i in range(1, max_iter + 1):
        # 每一轮都执行“评估 -> 判断是否停止 -> 选择最弱 cluster -> 局部拆分”。
        # 这是一个典型的贪心迭代修正流程：
        # - 不重做全局聚类
        # - 只对当前最差的局部结构动手术
        block_eval, cluster_eval, summary = _evaluate(
            df=work,
            feature_cols=val_cols,
            good_threshold=good_threshold,
            zero_eps=zero_eps,
        )

        log_row = {
            'iter': i,
            'good_rate_pct': summary['good_rate'] * 100.0,
            'cosine_mean': summary['cosine_mean'],
            'clusters': summary['clusters'],
            'blocks_eval': summary['blocks_eval'],
            'blocks_zero_vector': summary['blocks_zero_vector'],
        }
        logs.append(log_row)
        print(f"[iter={i}] good_rate={log_row['good_rate_pct']:.2f}% cosine_mean={log_row['cosine_mean']:.4f} clusters={log_row['clusters']}")

        if summary['good_rate'] >= target_good_rate:
            print('✅ Target reached.')
            break

        if (summary['clusters'] - int(assign_df['cluster_id'].nunique())) >= max_extra_clusters:
            print('⏹ Reached max extra clusters limit.')
            break

        split_cid = _pick_cluster_to_split(
            cluster_eval=cluster_eval,
            min_split_size=min_split_size,
            blocked_clusters=blocked_clusters,
        )

        if split_cid is None:
            print('⏹ No eligible cluster left to split.')
            break

        part_mask = work['cluster_id'] == split_cid
        part = work.loc[part_mask, val_cols]
        n_part = len(part)

        # split_k 的策略是保守的：
        # - 较小 cluster：优先尝试二分
        # - 更大 cluster：允许尝试三分
        # 但同时要保证平均下来不要把子簇切得过小，
        # 因此还要受 n_part // min_split_size 限制。
        split_k = 2 if n_part < min_split_size * 4 else 3
        split_k = min(split_k, max(2, n_part // min_split_size))
        if split_k < 2:
            blocked_clusters.add(split_cid)
            continue

        # 注意：这里不是全局重聚类。
        # 只在被选中的弱 cluster 内部再跑一次局部 KMeans，
        # 相当于对单个问题类做“局部切分修补”。
        km = MiniBatchKMeans(
            n_clusters=split_k,
            random_state=random_state + i,
            n_init=10,
            batch_size=1024,
        )
        labels = km.fit_predict(part.to_numpy(dtype=float))

        if len(set(labels)) <= 1:
            blocked_clusters.add(split_cid)
            print(f'⚠️ Cluster {split_cid} split failed (single label).')
            continue

        local_idx = work.index[part_mask]
        label_counts = pd.Series(labels).value_counts().sort_values(ascending=False)
        major_label = int(label_counts.index[0])

        for lab in sorted(set(labels)):
            idx_lab = local_idx[np.where(labels == lab)[0]]
            if lab == major_label:
                # 最大子簇继承原 cluster_id。
                # 这么做的目的是尽量保持标签连续性，减少编号抖动。
                work.loc[idx_lab, 'cluster_id'] = split_cid
            else:
                work.loc[idx_lab, 'cluster_id'] = next_cluster_id
                next_cluster_id += 1

        print(f'🔧 Split cluster {split_cid} (n={n_part}) into {len(set(labels))} parts.')

    # 用贪心迭代后的最终 assignment 状态生成收尾输出文件。
    block_eval, cluster_eval, summary = _evaluate(
        df=work,
        feature_cols=val_cols,
        good_threshold=good_threshold,
        zero_eps=zero_eps,
    )

    out_assign = city_dir / 'block_cluster_assignments_tuned.csv'
    out_proto = city_dir / 'block_cluster_prototypes_tuned.csv'
    out_iter = city_dir / 'cluster_tuning_log.csv'
    out_global = city_dir / 'cluster_tuning_global_eval.csv'

    out_assign_df = assign_df.drop(columns=['cluster_id'], errors='ignore').merge(
        work[['_block_id', 'cluster_id']], on='_block_id', how='inner', validate='one_to_one'
    )

    # 最终 prototype 表就是调优后聚类的 centroid 集合。
    proto = work.groupby('cluster_id', as_index=False)[val_cols].mean()
    proto['top_signature'] = proto.apply(lambda r: _safe_signature(r, val_cols, top_n=8), axis=1)
    proto['block_count'] = work.groupby('cluster_id').size().reindex(proto['cluster_id']).values

    out_assign_df.to_csv(out_assign, index=False, encoding='utf-8-sig')
    proto.to_csv(out_proto, index=False, encoding='utf-8-sig')
    pd.DataFrame(logs).to_csv(out_iter, index=False, encoding='utf-8-sig')
    pd.DataFrame([summary]).to_csv(out_global, index=False, encoding='utf-8-sig')

    print(f"✅ Tuned assignments: {out_assign}")
    print(f"✅ Tuned prototypes: {out_proto}")
    print(f"✅ Tuning log: {out_iter}")
    print(f"✅ Tuning global eval: {out_global}")
    print(f"📌 Final good_rate(>={good_threshold:.2f}) = {summary['good_rate'] * 100:.2f}%")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Iteratively split weak clusters to reach target reconstruction quality for small blocks.'
    )
    parser.add_argument('city', nargs='?', default=DEFAULT_CITY, help=f'City name (default: {DEFAULT_CITY})')  # type: ignore
    parser.add_argument('--feature-file', type=str, default='block_cluster_feature_matrix.csv', help='Input feature matrix (val_pct_*)')
    parser.add_argument('--assignment-file', type=str, default='block_cluster_assignments.csv', help='Input cluster assignment file')
    parser.add_argument('--target-good-pct', type=float, default=80.0, help='Target percent of good blocks')
    parser.add_argument('--good-threshold', type=float, default=0.85, help='Cosine threshold for good block')
    parser.add_argument('--min-split-size', type=int, default=30, help='Minimum cluster size to be eligible for splitting')
    parser.add_argument('--max-extra-clusters', type=int, default=40, help='Maximum new clusters allowed')
    parser.add_argument('--max-iter', type=int, default=30, help='Maximum optimization iterations')
    parser.add_argument('--random-state', type=int, default=42, help='Random seed')
    parser.add_argument('--zero-eps', type=float, default=1e-9, help='Rows with sum(val_pct_*) <= zero-eps are excluded from optimization denominator')

    args = parser.parse_args()

    optimize_clustering(
        city_name=args.city,
        feature_file=args.feature_file,
        assignment_file=args.assignment_file,
        target_good_pct=args.target_good_pct,
        good_threshold=args.good_threshold,
        min_split_size=args.min_split_size,
        max_extra_clusters=args.max_extra_clusters,
        max_iter=args.max_iter,
        random_state=args.random_state,
        zero_eps=args.zero_eps,
    )
