import argparse
import pathlib
import sys
from collections import defaultdict

import numpy as np
import pandas as pd

sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *

CITY_DIR = DATA_DIR / 'city'


def _safe_value_name(text: str) -> str:
    return str(text).strip().lower().replace(':', '_').replace('-', '_').replace(' ', '_')


def _coerce_numeric_pct(series: pd.Series) -> pd.Series:
    if series.dtype.kind in {'i', 'u', 'f'}:
        return series.astype(float)
    cleaned = series.astype(str).str.replace('%', '', regex=False).str.strip()
    return pd.to_numeric(cleaned, errors='coerce').fillna(0.0)


def _top_signature(row: pd.Series, feature_cols: list[str], top_n: int = 6) -> str:
    pairs = []
    for col in feature_cols:
        val = float(row.get(col, 0.0))
        if val > 0:
            pairs.append((col.replace('val_pct_', ''), val))
    if len(pairs) == 0:
        return 'empty'
    pairs.sort(key=lambda x: x[1], reverse=True)
    top = pairs[:top_n]
    return ' | '.join([f"{name}:{pct:.2f}%" for name, pct in top])


def _select_k_by_silhouette(x: np.ndarray, k_min: int = 3, k_max: int = 20) -> int:
    from sklearn.cluster import MiniBatchKMeans
    from sklearn.metrics import silhouette_score

    n = x.shape[0]
    if n < 20:
        return 2

    k_min = max(2, min(k_min, n - 1))
    k_max = max(k_min, min(k_max, max(2, n // 20)))

    best_k = k_min
    best_score = -1.0
    for k in range(k_min, k_max + 1):
        km = MiniBatchKMeans(n_clusters=k, random_state=42, n_init=10, batch_size=2048)
        labels = km.fit_predict(x)
        if len(set(labels)) <= 1:
            continue
        score = silhouette_score(x, labels, metric='euclidean')
        if score > best_score:
            best_score = score
            best_k = k
    return best_k


def _stable_int_key(text: str) -> int:
    out = 0
    for idx, ch in enumerate(str(text)):
        out += (idx + 1) * ord(ch)
    return int(out)


def _select_k_by_silhouette_seeded(
    x: np.ndarray,
    k_min: int,
    k_max: int,
    random_state: int,
) -> int:
    from sklearn.cluster import MiniBatchKMeans
    from sklearn.metrics import silhouette_score

    # x 的形状是 [样本数, 特征数]。
    # 这里每一行代表 1 个 block，每一列代表 1 个经过归一化后的
    # val_pct 组成维度，因此后续 KMeans 和 silhouette 都是在
    # “block 的功能组成向量空间”里工作。
    n = x.shape[0]
    if n < 20:
        # 样本太少时，silhouette 分数会非常不稳定，KMeans 也容易
        # 把偶然噪声解释成“可分结构”。
        # 因此这里直接返回 2，表示：
        # - 后续若真要拆，最多按“二分思路”理解
        # - 不在极小样本上继续精细搜索 k，避免过拟合
        return 2

    k_min = max(2, min(k_min, n - 1))
    k_max = max(k_min, min(k_max, max(2, n - 1)))

    best_k = k_min
    best_score = -1.0
    for k in range(k_min, k_max + 1):
        # 对每个候选 k，都执行同一套流程：
        # 1. 在当前 bucket 的特征子空间上拟合一次 MiniBatchKMeans
        # 2. 得到每个样本的簇标签
        # 3. 用 silhouette_score 评估“类内紧密 + 类间分离”的综合效果
        # 4. 保留 silhouette 最高的 k 作为当前 bucket 的最优局部簇数
        km = MiniBatchKMeans(n_clusters=k, random_state=random_state, n_init=10, batch_size=2048)
        labels = km.fit_predict(x)
        if len(set(labels)) <= 1:
            # 如果所有样本都塌缩到 1 个标签里，说明这个 k 下没有形成
            # 有意义的多簇结构。此时 silhouette 无法成立，直接跳过。
            continue
        score = silhouette_score(x, labels, metric='euclidean')
        if score > best_score:
            best_score = score
            best_k = k
    return best_k


def build_feature_table(
    long_df: pd.DataFrame,
    excluded_values: set[str],
    min_value_pct: float,
) -> tuple[pd.DataFrame, pd.DataFrame, list[str]]:
    long_df = long_df.copy()
    long_df['value_norm'] = long_df['value'].astype(str).map(_safe_value_name)
    long_df['val_pct'] = _coerce_numeric_pct(long_df['val_pct'])

    work = long_df[~long_df['value_norm'].isin(excluded_values)].copy()
    if len(work) == 0:
        raise ValueError('All rows are excluded after applying excluded values.')

    work = work[work['val_pct'] > 0].copy()
    if len(work) == 0:
        raise ValueError('No positive val_pct rows available in long-file after cleaning.')

    raw = work.pivot_table(
        index=['_block_id', 'sector_id'],
        columns='value_norm',
        values='val_pct',
        aggfunc='sum',
        fill_value=0.0,
    )

    raw.columns = [f'val_pct_{c}' for c in raw.columns]
    raw = raw.reset_index()

    feature_cols = [c for c in raw.columns if c.startswith('val_pct_')]
    if len(feature_cols) == 0:
        raise ValueError('No feature columns available after pivot.')

    # 这里做的是“全局特征筛选”，不是按单行删值：
    # - 如果某个维度在所有 block 中的最大值都很小
    # - 说明它几乎从未真正主导过任何 block 的组成
    # - 保留它只会在距离计算中引入噪声
    # 因此把这类弱维度整体移除。
    keep_cols = []
    for col in feature_cols:
        if float(raw[col].max()) >= min_value_pct:
            keep_cols.append(col)

    if len(keep_cols) == 0:
        keep_cols = feature_cols

    # 关键一步：对保留下来的维度重新归一化到 100。
    # 原因有两个：
    # 1. construction 等值被人为排除后，每行总和不再是 100
    # 2. 微弱维度被删掉后，每行也会损失一部分质量
    # 如果不重归一化，那么两个 block 的欧式距离会同时受
    # “组成结构差异” 和 “剩余总量大小差异” 影响，不利于比较纯结构。
    # 这里通过除以行和，再乘 100，把每个 block 重新投影回
    # “剩余维度组成占比空间”。
    row_sum = raw[keep_cols].sum(axis=1)
    row_sum = row_sum.replace(0, np.nan)
    norm = raw[['_block_id', 'sector_id']].copy()
    norm[keep_cols] = raw[keep_cols].div(row_sum, axis=0) * 100.0
    norm[keep_cols] = norm[keep_cols].fillna(0.0)

    return raw, norm, keep_cols


def build_feature_table_from_wide(
    wide_df: pd.DataFrame,
    excluded_values: set[str],
    min_value_pct: float,
) -> tuple[pd.DataFrame, pd.DataFrame, list[str]]:
    df = wide_df.copy()
    if '_block_id' not in df.columns:
        if 'block_id' in df.columns:
            df['_block_id'] = df['block_id'].astype(str)
        else:
            df = df.reset_index(drop=True)
            df['_block_id'] = df.index.astype(str)

    if 'sector_id' not in df.columns:
        df['sector_id'] = df['_block_id'].astype(str)

    feature_cols = [c for c in df.columns if str(c).startswith('val_pct_')]
    if len(feature_cols) == 0:
        raise ValueError('No val_pct_* columns found in wide file.')

    # 宽表模式下没有原始 value 列，因此通过 val_pct_* 的列名后缀
    # 来识别是否属于要排除的值，如 construction。
    keep = []
    for col in feature_cols:
        value_name = _safe_value_name(col.replace('val_pct_', '', 1))
        if value_name in excluded_values:
            continue
        keep.append(col)

    if len(keep) == 0:
        raise ValueError('All val_pct_* columns are excluded.')

    for col in keep:
        df[col] = _coerce_numeric_pct(df[col])

    keep_cols = [col for col in keep if float(df[col].max()) >= min_value_pct]
    if len(keep_cols) == 0:
        keep_cols = keep

    raw = df[['_block_id', 'sector_id'] + keep_cols].copy()
    row_sum = raw[keep_cols].sum(axis=1).replace(0, np.nan)

    norm = raw[['_block_id', 'sector_id']].copy()
    # 与长表模式同理：
    # 先保留真正参与聚类的维度，再把每行重新归一化到 100，
    # 让距离计算只表达“剩余组成结构”。
    norm[keep_cols] = raw[keep_cols].div(row_sum, axis=0) * 100.0
    norm[keep_cols] = norm[keep_cols].fillna(0.0)

    return raw, norm, keep_cols


def run_two_stage_clustering(
    norm_df: pd.DataFrame,
    feature_cols: list[str],
    max_global_clusters: int,
    random_state: int,
    local_k_min: int,
    local_k_divisor: int,
    min_bucket_size: int,
) -> pd.DataFrame:
    try:
        from sklearn.cluster import MiniBatchKMeans
    except Exception as exc:
        raise ImportError(f'scikit-learn is required for clustering: {exc}')

    assignments = norm_df[['_block_id', 'sector_id']].copy()

    # 第一阶段：先按 dominant value 的“家族前缀”分 macro bucket。
    # 例如：
    # - dominant_value = commercial_retail
    # - macro_key = commercial
    # 这样做的目的，是先把明显不同的大类功能结构分开，
    # 避免一次全局 KMeans 把“商业主导”和“居住主导”这种
    # 差异很大的 block 过早混在一起，再在错误边界上细分。
    assignments['dominant_value'] = norm_df[feature_cols].idxmax(axis=1).str.replace('val_pct_', '', regex=False)

    macro_groups = defaultdict(list)
    for idx, val in assignments['dominant_value'].items():
        key = val.split('_')[0] if '_' in val else val
        macro_groups[key].append(idx)

    cluster_labels = np.full(len(norm_df), -1, dtype=int)
    prototype_rows = []
    global_cluster_id = 0

    for macro_key, idx_list in macro_groups.items():
        part = norm_df.loc[idx_list, feature_cols].copy()
        n = len(part)
        if n < max(2, min_bucket_size):
            # 这个 macro bucket 太小，不继续拆。
            # 原因：
            # - 样本量不足时，局部聚类结果很容易受偶然点影响
            # - 强行再分只会制造统计意义很弱的小簇
            # 因此整桶直接视作 1 个 cluster。
            labels = np.zeros(n, dtype=int)
        else:
            divisor = max(2, int(local_k_divisor))
            # 局部最大簇数 local_k_max 的构造逻辑：
            # - 样本越多，允许的局部复杂度可以越高
            # - 但不能无限增大，因此还要受 max_global_clusters 限制
            # - local_k_divisor 越小，n/divisor 越大，越容易产生更多簇
            # - local_k_divisor 越大，搜索越保守
            # 近似关系可以理解成：
            #   local_k_max ≈ n / local_k_divisor
            # 然后再与 max_global_clusters 取较小值。
            local_k_max = min(max_global_clusters, max(2, n // divisor))
            local_k_max = max(2, local_k_max)
            # 为每个 macro bucket 派生一个稳定但不同的随机种子。
            # 这样做有两个好处：
            # 1. 同一全局 random_state 下可复现
            # 2. 不同 macro bucket 不会完全共享同一初始化轨迹
            macro_seed = int(random_state + _stable_int_key(macro_key) % 100000)
            local_k = _select_k_by_silhouette_seeded(
                part.values,
                k_min=max(2, int(local_k_min)),
                k_max=local_k_max,
                random_state=macro_seed,
            )
            # 第二阶段：在当前 macro bucket 内，用 silhouette 选出的
            # 最优局部簇数 local_k，执行最终一次局部 KMeans。
            km = MiniBatchKMeans(n_clusters=local_k, random_state=macro_seed, n_init=10, batch_size=1024)
            labels = km.fit_predict(part.values)

        for local_label in sorted(set(labels)):
            local_idx = [idx_list[i] for i, l in enumerate(labels) if l == local_label]
            cluster_labels[local_idx] = global_cluster_id

            # 这里的 proto 是“簇原型”：
            # - 对当前 cluster 内全部 block 的归一化向量逐维求均值
            # - 得到该类在特征空间中的质心 centroid
            # 后续 validate_reconstruction 会把这个 centroid 当作
            # “该类是否能代表其成员”的重建模板。
            proto = norm_df.loc[local_idx, feature_cols].mean().sort_values(ascending=False)
            top_vals = proto[proto > 0.5].head(10)
            prototype_rows.append(
                {
                    'cluster_id': global_cluster_id,
                    'macro_bucket': macro_key,
                    'block_count': len(local_idx),
                    'top_signature': ' | '.join([f"{k.replace('val_pct_', '')}:{v:.2f}%" for k, v in top_vals.items()]),
                }
            )
            global_cluster_id += 1

    assignments['cluster_id'] = cluster_labels
    proto_df = pd.DataFrame(prototype_rows)
    return assignments, proto_df


def add_construction_back(
    assignments_df: pd.DataFrame,
    raw_long_df: pd.DataFrame,
    construction_values: set[str],
) -> pd.DataFrame:
    temp = raw_long_df.copy()
    temp['value_norm'] = temp['value'].astype(str).map(_safe_value_name)
    temp['is_construction'] = temp['value_norm'].isin(construction_values)

    cons = temp[temp['is_construction']].groupby('_block_id', as_index=False)['val_pct'].sum().rename(columns={'val_pct': 'construction_pct'})
    out = assignments_df.merge(cons, on='_block_id', how='left')
    out['construction_pct'] = out['construction_pct'].fillna(0.0)

    out['construction_role'] = np.where(
        out['construction_pct'] >= 40.0,
        'construction_dominant',
        np.where(out['construction_pct'] >= 10.0, 'construction_mixed', 'construction_minor')
    )
    return out


def cluster_blocks(
    city_name: str,
    long_file: str,
    wide_file: str,
    prefer_wide: bool,
    exclude_values: list[str],
    min_value_pct: float,
    max_global_clusters: int,
    random_state: int,
    local_k_min: int,
    local_k_divisor: int,
    min_bucket_size: int,
    verbose: bool = True,
):
    city_dir = CITY_DIR / city_name
    long_path = city_dir / long_file
    wide_path = city_dir / wide_file

    excluded_values = {_safe_value_name(v) for v in exclude_values}
    if verbose:
        print(f"🚫 Excluding values for clustering: {sorted(excluded_values)}")

    raw_df = None
    norm_df = None
    feature_cols = None
    long_df = None

    errors = []

    if prefer_wide:
        if not wide_path.exists():
            errors.append(f'Wide file not found: {wide_path}')
        else:
            try:
                if verbose:
                    print(f"📥 Loading wide features: {wide_path}")
                if str(wide_path).lower().endswith('.xlsx'):
                    wide_df = pd.read_excel(wide_path)
                else:
                    wide_df = pd.read_csv(wide_path)
                raw_df, norm_df, feature_cols = build_feature_table_from_wide(
                    wide_df=wide_df,
                    excluded_values=excluded_values,
                    min_value_pct=min_value_pct,
                )
            except Exception as exc:
                errors.append(f'Wide mode failed: {exc}')

    if norm_df is None:
        if not long_path.exists():
            errors.append(f'Long file not found: {long_path}')
        else:
            try:
                if verbose:
                    print(f"📥 Loading long features: {long_path}")
                if str(long_path).lower().endswith('.xlsx'):
                    long_df = pd.read_excel(long_path)
                else:
                    long_df = pd.read_csv(long_path)

                required_cols = {'_block_id', 'sector_id', 'value', 'val_pct'}
                miss = required_cols - set(long_df.columns)
                if miss:
                    raise ValueError(f'Missing required columns in long file: {sorted(miss)}')

                raw_df, norm_df, feature_cols = build_feature_table(
                    long_df=long_df,
                    excluded_values=excluded_values,
                    min_value_pct=min_value_pct,
                )
            except Exception as exc:
                errors.append(f'Long mode failed: {exc}')

    if norm_df is None or feature_cols is None:
        raise ValueError('Cannot build features for clustering. ' + ' | '.join(errors))

    if verbose:
        print(f"🧠 Clustering on {len(feature_cols)} feature dimensions, blocks={len(norm_df)}")
    assignments, prototypes = run_two_stage_clustering(
        norm_df=norm_df,
        feature_cols=feature_cols,
        max_global_clusters=max_global_clusters,
        random_state=random_state,
        local_k_min=local_k_min,
        local_k_divisor=local_k_divisor,
        min_bucket_size=min_bucket_size,
    )

    # 给每个 block 补一个可读 signature，便于人工查看其主导组成。
    # 这部分不参与聚类数学，只服务于解释和排查。
    temp = norm_df.copy()
    temp['cluster_signature'] = temp.apply(lambda r: _top_signature(r, feature_cols, top_n=8), axis=1)
    assignments = assignments.merge(temp[['_block_id', 'cluster_signature']], on='_block_id', how='left')

    if long_df is None and long_path.exists():
        try:
            long_df = pd.read_csv(long_path)
        except Exception:
            long_df = None

    if long_df is not None:
        assignments = add_construction_back(
            assignments_df=assignments,
            raw_long_df=long_df,
            construction_values={_safe_value_name('construction')},
        )
    else:
        assignments['construction_pct'] = 0.0
        assignments['construction_role'] = 'construction_unknown'

    # 为每个 cluster 原型补充统计信息：
    # - block 数量
    # - construction 占比均值
    # - construction 占比高位分位数
    # 注意 construction 在聚类距离里被排除，但会在解释层面加回，
    # 方便理解某一 archetype 是否带有较强施工特征。
    cluster_stats = (
        assignments.groupby('cluster_id', as_index=False)
        .agg(
            block_count=('cluster_id', 'size'),
            construction_pct_mean=('construction_pct', 'mean'),
            construction_pct_p90=('construction_pct', lambda s: float(np.percentile(s, 90))),
        )
        .sort_values('block_count', ascending=False)
    )

    prototypes = prototypes.merge(cluster_stats, on='cluster_id', how='left')

    out_assign = city_dir / 'block_cluster_assignments.csv'
    out_proto = city_dir / 'block_cluster_prototypes.csv'
    out_features = city_dir / 'block_cluster_feature_matrix.csv'

    assignments.to_csv(out_assign, index=False, encoding='utf-8-sig')
    prototypes.to_csv(out_proto, index=False, encoding='utf-8-sig')
    norm_df.to_csv(out_features, index=False, encoding='utf-8-sig')

    if verbose:
        print(f"✅ Cluster assignments: {out_assign}")
        print(f"✅ Cluster prototypes: {out_proto}")
        print(f"✅ Normalized feature matrix: {out_features}")
        print(f"✅ Total clusters: {assignments['cluster_id'].nunique()}")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Cluster blocks into archetypes (construction excluded first, then added back as role).'
    )
    parser.add_argument('city', nargs='?', default=DEFAULT_CITY, help=f'City name (default: {DEFAULT_CITY})')  # type: ignore
    parser.add_argument('--long-file', type=str, default='block_value_features_long.csv', help='Long feature file from extract_block_features.py')
    parser.add_argument('--wide-file', type=str, default='block_value_features.xlsx', help='Wide feature file (xlsx/csv/tsv) from extract_block_features.py')
    parser.add_argument('--prefer-wide', action='store_true', help='Prefer wide-file input first, then fallback to long-file')
    parser.add_argument('--exclude-values', nargs='+', default=['construction'], help='Values excluded during clustering stage')
    parser.add_argument('--min-value-pct', type=float, default=0.2, help='Keep feature columns whose max pct >= this threshold')
    parser.add_argument('--max-global-clusters', type=int, default=20, help='Max clusters per macro bucket')
    parser.add_argument('--random-state', type=int, default=42, help='Random seed for silhouette and KMeans')
    parser.add_argument('--local-k-min', type=int, default=2, help='Minimum local k in each macro bucket')
    parser.add_argument('--local-k-divisor', type=int, default=80, help='local_k_max approx n/divisor (before global cap)')
    parser.add_argument('--min-bucket-size', type=int, default=8, help='Buckets smaller than this use single cluster')
    parser.add_argument('--quiet', action='store_true', help='Suppress verbose per-run logs')
    args = parser.parse_args()

    cluster_blocks(
        city_name=args.city,
        long_file=args.long_file,
        wide_file=args.wide_file,
        prefer_wide=args.prefer_wide,
        exclude_values=args.exclude_values,
        min_value_pct=args.min_value_pct,
        max_global_clusters=args.max_global_clusters,
        random_state=args.random_state,
        local_k_min=args.local_k_min,
        local_k_divisor=args.local_k_divisor,
        min_bucket_size=args.min_bucket_size,
        verbose=not args.quiet,
    )
