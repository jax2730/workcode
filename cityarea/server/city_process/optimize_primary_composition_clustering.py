import argparse
import json
import math
import pathlib
import sys

import numpy as np
import pandas as pd

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from server.city_process.cluster_primary_merged_archetypes import (
    DEFAULT_DATA_DIR,
    _primary_name,
    build_classification_text,
    build_pca_projection,
    compute_clustering_metrics,
    normalize_primary_features,
    to_bestauto_prototypes,
    write_clustering_plots,
)


def _select_k_by_sampled_silhouette(
    x: np.ndarray,
    k_min: int,
    k_max: int,
    random_state: int,
    sample_size: int = 600,
) -> int:
    from sklearn.cluster import MiniBatchKMeans
    from sklearn.metrics import silhouette_score

    n = x.shape[0]
    if n < 20:
        return 2 if n >= 2 else 1

    k_min = max(2, min(int(k_min), n - 1))
    k_max = max(k_min, min(int(k_max), n - 1))
    base_candidates = [2, 3, 4, 5, 6, 8, 10, 12, 16, 20, 24, 32, 40]
    candidates = sorted({k_min, k_max, *[k for k in base_candidates if k_min <= k <= k_max]})

    best_k = k_min
    best_score = -1.0
    for k in candidates:
        km = MiniBatchKMeans(n_clusters=k, random_state=random_state, n_init=3, max_iter=100, batch_size=1024)
        labels = km.fit_predict(x)
        if len(set(labels)) <= 1:
            continue
        score = silhouette_score(
            x,
            labels,
            metric="euclidean",
            sample_size=min(int(sample_size), n),
            random_state=int(random_state),
        )
        if score > best_score:
            best_score = score
            best_k = k
    return best_k


def transform_composition(
    x_percent: pd.DataFrame,
    method: str,
    zero_replacement: float = 1e-6,
) -> pd.DataFrame:
    x = x_percent.astype(float).copy()
    proportions = x / 100.0

    if method == "raw_percent":
        return x

    if method == "hellinger":
        # Hellinger 变换：h_j = sqrt(p_j)。
        # 它保留“组成比例”含义，同时降低纯类型 100% 顶点对欧氏距离的支配。
        return np.sqrt(proportions).rename(columns=dict(zip(proportions.columns, x.columns)))

    if method == "clr":
        # CLR（centered log-ratio）把组成数据从单纯形映射到实数空间：
        # clr(p_j)=log(p_j)-mean_k(log(p_k))。
        # 因为 log(0) 不存在，先用极小伪计数替换 0，再重新闭合到总和 1。
        replaced = proportions.clip(lower=float(zero_replacement))
        closed = replaced.div(replaced.sum(axis=1), axis=0)
        logged = np.log(closed)
        return logged.sub(logged.mean(axis=1), axis=0)

    raise ValueError(f"Unsupported composition transform: {method}")


def build_candidate_score(candidate: dict) -> float:
    silhouette = float(candidate.get("silhouette_score") or 0.0)
    db = float(candidate.get("davies_bouldin_score") or 999.0)
    ch = float(candidate.get("calinski_harabasz_score") or 0.0)
    small_share = float(candidate.get("small_cluster_sample_share") or 0.0)
    size_cv = float(candidate.get("cluster_size_cv") or 0.0)
    stability = float(candidate.get("stability_ari_mean") if candidate.get("stability_ari_mean") is not None else 0.70)

    # 分数只用于候选排序，不是数学定理：
    # 1) silhouette 越高，簇越紧凑且分离；
    # 2) DB 越低越好，用 1/(1+DB) 转成越高越好；
    # 3) CH 用 log 压缩，避免大数值支配；
    # 4) 小簇样本占比、簇规模变异、稳定性作为惩罚项。
    compact_separation = (
        0.45 * max(0.0, min((silhouette + 1.0) / 2.0, 1.0))
        + 0.30 * (1.0 / (1.0 + max(db, 0.0)))
        + 0.25 * max(0.0, min(math.log1p(max(ch, 0.0)) / 7.0, 1.0))
    )
    small_cluster_penalty = max(0.0, 1.0 - small_share) ** 2
    balance_penalty = max(0.20, 1.0 - min(size_cv / 3.0, 1.0))
    stability_penalty = max(0.20, min(stability, 1.0))
    return float(compact_separation * small_cluster_penalty * balance_penalty * stability_penalty)


def prepare_primary_matrix(
    input_file: pathlib.Path,
    sheet_name: str,
    include_unmapped: bool,
    zero_eps: float,
) -> tuple[pd.DataFrame, pd.DataFrame, list[str]]:
    if input_file.suffix.lower() in {".xlsx", ".xlsm", ".xls"}:
        df = pd.read_excel(input_file, sheet_name=sheet_name)
    else:
        df = pd.read_csv(input_file)
    norm, feature_cols = normalize_primary_features(df, include_unmapped=include_unmapped, zero_eps=zero_eps)
    work = norm[~norm["is_zero_primary_vector"]].copy().reset_index(drop=True)
    return df, work, feature_cols


def diagnose_primary_data(
    raw_df: pd.DataFrame,
    work: pd.DataFrame,
    feature_cols: list[str],
) -> dict:
    x = work[feature_cols].astype(float)
    row_sums = x.sum(axis=1)
    max_pct = x.max(axis=1)
    zero_cell_share = float((x <= 0).sum().sum() / (x.shape[0] * x.shape[1])) if len(x) else None
    unmapped = pd.to_numeric(raw_df.get("primary_pct_未映射", pd.Series(dtype=float)), errors="coerce").fillna(0.0)

    return {
        "raw_block_count": int(len(raw_df)),
        "clusterable_block_count": int(len(work)),
        "zero_primary_vector_count": int(len(raw_df) - len(work)),
        "feature_count": int(len(feature_cols)),
        "feature_columns": feature_cols,
        "row_sum_min": float(row_sums.min()) if len(row_sums) else None,
        "row_sum_max": float(row_sums.max()) if len(row_sums) else None,
        "zero_cell_share": zero_cell_share,
        "unmapped_pct_mean": float(unmapped.mean()) if len(unmapped) else None,
        "unmapped_pct_max": float(unmapped.max()) if len(unmapped) else None,
        "dominant_pct_mean": float(max_pct.mean()) if len(max_pct) else None,
        "dominant_pct_median": float(max_pct.median()) if len(max_pct) else None,
        "dominant_pct_ge_80_share": float((max_pct >= 80).mean()) if len(max_pct) else None,
        "dominant_pct_ge_95_share": float((max_pct >= 95).mean()) if len(max_pct) else None,
    }


def _cluster_size_stats(labels: np.ndarray, min_cluster_size: int) -> dict:
    sizes = pd.Series(labels).value_counts().sort_index()
    small_sizes = sizes[sizes < int(min_cluster_size)]
    return {
        "min_cluster_size_actual": int(sizes.min()),
        "max_cluster_size_actual": int(sizes.max()),
        "small_cluster_count": int(len(small_sizes)),
        "small_cluster_sample_share": float(small_sizes.sum() / sizes.sum()) if sizes.sum() else 0.0,
        "cluster_size_cv": float(sizes.std(ddof=0) / sizes.mean()) if sizes.mean() else 0.0,
    }


def _labels_global(x: pd.DataFrame, k: int, random_state: int) -> np.ndarray:
    from sklearn.cluster import MiniBatchKMeans

    km = MiniBatchKMeans(n_clusters=int(k), random_state=int(random_state), n_init=3, max_iter=100, batch_size=1024)
    return km.fit_predict(x.to_numpy(dtype=float))


def _labels_two_stage(
    x_transformed: pd.DataFrame,
    x_raw_percent: pd.DataFrame,
    feature_cols: list[str],
    max_clusters_per_primary: int,
    local_k_min: int,
    local_k_divisor: int,
    min_bucket_size: int,
    random_state: int,
) -> tuple[np.ndarray, pd.Series]:
    from sklearn.cluster import MiniBatchKMeans

    macro_primary = x_raw_percent[feature_cols].idxmax(axis=1).map(_primary_name)
    labels_out = np.full(len(x_raw_percent), -1, dtype=int)
    next_cluster_id = 0

    for macro_name, group_index in macro_primary.groupby(macro_primary, sort=False).groups.items():
        idx = list(group_index)
        part = x_transformed.iloc[idx]
        n = len(part)
        if n < max(2, int(min_bucket_size)):
            local_labels = np.zeros(n, dtype=int)
        else:
            local_k_max = min(int(max_clusters_per_primary), max(2, n // max(2, int(local_k_divisor))))
            local_k_max = max(2, min(local_k_max, n - 1))
            macro_seed = int(random_state + sum((i + 1) * ord(ch) for i, ch in enumerate(str(macro_name))) % 100000)
            local_k = _select_k_by_sampled_silhouette(
                part.to_numpy(dtype=float),
                k_min=max(2, int(local_k_min)),
                k_max=local_k_max,
                random_state=macro_seed,
            )
            km = MiniBatchKMeans(n_clusters=local_k, random_state=macro_seed, n_init=3, max_iter=100, batch_size=1024)
            local_labels = km.fit_predict(part.to_numpy(dtype=float))

        for local_label in sorted(set(local_labels)):
            local_idx = [idx[pos] for pos, value in enumerate(local_labels) if value == local_label]
            labels_out[local_idx] = next_cluster_id
            next_cluster_id += 1

    return labels_out, macro_primary


def evaluate_labels(
    x_for_distance: pd.DataFrame,
    labels: np.ndarray,
    min_cluster_size: int,
    silhouette_sample_size: int = 800,
    random_state: int = 42,
) -> dict:
    from sklearn.metrics import calinski_harabasz_score, davies_bouldin_score, silhouette_score

    x_arr = x_for_distance.to_numpy(dtype=float)
    labels_arr = np.asarray(labels, dtype=int)
    unique_labels = sorted(set(labels_arr.tolist()))
    sample_count = int(len(labels_arr))
    cluster_count = int(len(unique_labels))

    total_wcss = 0.0
    for cluster_id in unique_labels:
        cluster_x = x_arr[labels_arr == cluster_id]
        centroid = cluster_x.mean(axis=0)
        total_wcss += float(((cluster_x - centroid) ** 2).sum())

    metrics = {
        "sample_count": sample_count,
        "feature_count": int(x_arr.shape[1]),
        "cluster_count": cluster_count,
        "within_cluster_ss": total_wcss,
        "mean_within_cluster_ss_per_sample": float(total_wcss / sample_count) if sample_count else None,
        "silhouette_score": None,
        "silhouette_sample_size": int(min(silhouette_sample_size, sample_count)),
        "calinski_harabasz_score": None,
        "davies_bouldin_score": None,
    }
    if 1 < cluster_count < sample_count:
        # 候选搜索阶段用抽样 silhouette，避免 O(n^2) 距离矩阵在几十个候选上反复计算。
        # 推荐方案选出后，脚本会再用 compute_clustering_metrics 做全量复核。
        metrics["silhouette_score"] = float(
            silhouette_score(
                x_arr,
                labels_arr,
                metric="euclidean",
                sample_size=min(int(silhouette_sample_size), sample_count),
                random_state=int(random_state),
            )
        )
        metrics["calinski_harabasz_score"] = float(calinski_harabasz_score(x_arr, labels_arr))
        metrics["davies_bouldin_score"] = float(davies_bouldin_score(x_arr, labels_arr))
    candidate = {
        **metrics,
        **_cluster_size_stats(labels, min_cluster_size=min_cluster_size),
    }
    candidate["candidate_score"] = build_candidate_score(candidate)
    return candidate


def make_catalog(
    work: pd.DataFrame,
    feature_cols: list[str],
    labels: np.ndarray,
    strategy: str,
    macro_primary: pd.Series | None = None,
) -> pd.DataFrame:
    rows = []
    raw_x = work[feature_cols].astype(float)
    for cluster_id in sorted(set(int(v) for v in labels)):
        mask = labels == cluster_id
        centroid = raw_x.loc[mask].mean()
        text = build_classification_text(centroid, feature_cols)
        if macro_primary is not None:
            macro_value = macro_primary.loc[mask].mode().iloc[0]
        else:
            macro_value = text["macro_class"]
        rows.append(
            {
                "primary_cluster_id": int(cluster_id),
                "macro_primary": macro_value,
                "strategy": strategy,
                "block_count": int(mask.sum()),
                **text,
                **{col: float(centroid[col]) for col in feature_cols},
            }
        )
    return pd.DataFrame(rows)


def _mean_pairwise_ari(label_sets: list[np.ndarray]) -> float | None:
    from sklearn.metrics import adjusted_rand_score

    if len(label_sets) < 2:
        return None
    scores = []
    for i in range(len(label_sets)):
        for j in range(i + 1, len(label_sets)):
            scores.append(adjusted_rand_score(label_sets[i], label_sets[j]))
    return float(np.mean(scores)) if scores else None


def compute_stability_for_candidate(
    candidate: dict,
    transforms: dict[str, pd.DataFrame],
    raw_x: pd.DataFrame,
    feature_cols: list[str],
    min_bucket_size: int,
    random_state: int,
    stability_runs: int,
) -> float | None:
    if stability_runs < 2:
        return None
    labels = []
    x_trans = transforms[candidate["transform"]]
    for offset in range(stability_runs):
        seed = int(random_state + 101 * offset)
        if candidate["strategy"] == "global_kmeans":
            labels.append(_labels_global(x_trans, int(candidate["k"]), seed))
        else:
            stage_labels, _ = _labels_two_stage(
                x_transformed=x_trans,
                x_raw_percent=raw_x,
                feature_cols=feature_cols,
                max_clusters_per_primary=int(candidate["max_clusters_per_primary"]),
                local_k_min=int(candidate["local_k_min"]),
                local_k_divisor=int(candidate["local_k_divisor"]),
                min_bucket_size=min_bucket_size,
                random_state=seed,
            )
            labels.append(stage_labels)
    return _mean_pairwise_ari(labels)


def health_label(candidate: dict) -> str:
    silhouette = float(candidate.get("silhouette_score") or 0.0)
    db = float(candidate.get("davies_bouldin_score") or 999.0)
    small_share = float(candidate.get("small_cluster_sample_share") or 0.0)
    stability = candidate.get("stability_ari_mean")
    stability = 0.70 if stability is None or pd.isna(stability) else float(stability)
    if silhouette >= 0.50 and db <= 1.00 and small_share <= 0.05 and stability >= 0.75:
        return "理想"
    if silhouette >= 0.35 and db <= 1.50 and small_share <= 0.12 and stability >= 0.60:
        return "可用但需人工复核"
    return "不建议直接采用"


def json_safe(value):
    if isinstance(value, dict):
        return {key: json_safe(item) for key, item in value.items()}
    if isinstance(value, list):
        return [json_safe(item) for item in value]
    if isinstance(value, tuple):
        return [json_safe(item) for item in value]
    if isinstance(value, float) and (math.isnan(value) or math.isinf(value)):
        return None
    if pd.isna(value) if not isinstance(value, (dict, list, tuple, np.ndarray)) else False:
        return None
    if isinstance(value, np.integer):
        return int(value)
    if isinstance(value, np.floating):
        return None if np.isnan(value) or np.isinf(value) else float(value)
    return value


def optimize_primary_composition_clustering(
    input_file: pathlib.Path,
    output_prefix: pathlib.Path,
    sheet_name: str = "primary_merged_matrix",
    include_unmapped: bool = False,
    zero_eps: float = 1e-9,
    transforms_to_try: tuple[str, ...] = ("raw_percent", "hellinger", "clr"),
    min_k: int = 12,
    max_k: int = 52,
    k_step: int = 4,
    two_stage_divisors: tuple[int, ...] = (8, 10, 14, 20, 30),
    max_clusters_per_primary: int = 40,
    local_k_min: int = 2,
    min_bucket_size: int = 8,
    random_state: int = 42,
    stability_top_n: int = 8,
    stability_runs: int = 5,
) -> dict:
    raw_df, work, feature_cols = prepare_primary_matrix(
        input_file=input_file,
        sheet_name=sheet_name,
        include_unmapped=include_unmapped,
        zero_eps=zero_eps,
    )
    raw_x = work[feature_cols].astype(float).reset_index(drop=True)
    transforms = {method: transform_composition(raw_x, method=method) for method in transforms_to_try}
    data_quality = diagnose_primary_data(raw_df, work, feature_cols)

    candidate_rows = []
    for method, x_trans in transforms.items():
        for k in range(int(min_k), int(max_k) + 1, int(k_step)):
            labels = _labels_global(x_trans, k=k, random_state=random_state)
            row = evaluate_labels(x_trans, labels, min_cluster_size=min_bucket_size, random_state=random_state)
            row.update({"strategy": "global_kmeans", "transform": method, "k": int(k)})
            candidate_rows.append(row)

        for divisor in two_stage_divisors:
            labels, _ = _labels_two_stage(
                x_transformed=x_trans,
                x_raw_percent=raw_x,
                feature_cols=feature_cols,
                max_clusters_per_primary=max_clusters_per_primary,
                local_k_min=local_k_min,
                local_k_divisor=int(divisor),
                min_bucket_size=min_bucket_size,
                random_state=random_state,
            )
            row = evaluate_labels(x_trans, labels, min_cluster_size=min_bucket_size, random_state=random_state)
            row.update(
                {
                    "strategy": "two_stage_macro_kmeans",
                    "transform": method,
                    "k": None,
                    "max_clusters_per_primary": int(max_clusters_per_primary),
                    "local_k_min": int(local_k_min),
                    "local_k_divisor": int(divisor),
                }
            )
            candidate_rows.append(row)

    candidates = pd.DataFrame(candidate_rows).sort_values("candidate_score", ascending=False).reset_index(drop=True)

    for idx in candidates.head(int(stability_top_n)).index:
        candidate = candidates.loc[idx].to_dict()
        stability = compute_stability_for_candidate(
            candidate=candidate,
            transforms=transforms,
            raw_x=raw_x,
            feature_cols=feature_cols,
            min_bucket_size=min_bucket_size,
            random_state=random_state,
            stability_runs=stability_runs,
        )
        candidates.loc[idx, "stability_ari_mean"] = stability
        updated = candidates.loc[idx].to_dict()
        candidates.loc[idx, "candidate_score"] = build_candidate_score(updated)

    candidates["health"] = candidates.apply(lambda row: health_label(row.to_dict()), axis=1)
    candidates = candidates.sort_values("candidate_score", ascending=False).reset_index(drop=True)
    best = candidates.iloc[0].to_dict()
    best_x = transforms[best["transform"]]
    if best["strategy"] == "global_kmeans":
        best_labels = _labels_global(best_x, k=int(best["k"]), random_state=random_state)
        best_macro = None
    else:
        best_labels, best_macro = _labels_two_stage(
            x_transformed=best_x,
            x_raw_percent=raw_x,
            feature_cols=feature_cols,
            max_clusters_per_primary=int(best["max_clusters_per_primary"]),
            local_k_min=int(best["local_k_min"]),
            local_k_divisor=int(best["local_k_divisor"]),
            min_bucket_size=min_bucket_size,
            random_state=random_state,
        )

    id_cols = [c for c in ["_block_id", "sector_id"] if c in work.columns]
    assignments = work[id_cols].copy()
    assignments["cluster_id"] = best_labels.astype(int)
    assignments["transform"] = best["transform"]
    assignments["strategy"] = best["strategy"]
    assignments["macro_primary"] = raw_x[feature_cols].idxmax(axis=1).map(_primary_name)
    assignments["primary_signature"] = work.get("primary_signature", pd.Series([""] * len(work))).to_numpy()

    catalog = make_catalog(
        work=work,
        feature_cols=feature_cols,
        labels=best_labels,
        strategy=best["strategy"],
        macro_primary=assignments["macro_primary"] if best_macro is not None else None,
    )
    prototypes = to_bestauto_prototypes(catalog)
    metrics, per_cluster_metrics = compute_clustering_metrics(best_x, best_labels)
    per_cluster_metrics = per_cluster_metrics.merge(
        catalog[["primary_cluster_id", "macro_primary", "micro_class", "primary_signature"]],
        left_on="cluster_id",
        right_on="primary_cluster_id",
        how="left",
    )

    pca_coords, pca_summary = build_pca_projection(best_x, best_labels)
    pca_coords = pd.concat(
        [assignments[[c for c in ["_block_id", "sector_id", "macro_primary"] if c in assignments.columns]].reset_index(drop=True), pca_coords],
        axis=1,
    )

    output_prefix.parent.mkdir(parents=True, exist_ok=True)
    candidates_path = output_prefix.with_name(output_prefix.name + "_candidates.csv")
    data_quality_path = output_prefix.with_name(output_prefix.name + "_data_quality.json")
    best_metrics_path = output_prefix.with_name(output_prefix.name + "_recommended_metrics.json")
    assignments_path = output_prefix.with_name(output_prefix.name + "_recommended_assignments.csv")
    catalog_path = output_prefix.with_name(output_prefix.name + "_recommended_catalog.csv")
    prototypes_path = output_prefix.with_name(output_prefix.name + "_recommended_prototypes_bestauto.csv")
    per_cluster_path = output_prefix.with_name(output_prefix.name + "_recommended_per_cluster_metrics.csv")
    pca_path = output_prefix.with_name(output_prefix.name + "_recommended_pca_coordinates.csv")
    report_path = output_prefix.with_name(output_prefix.name + "_report.json")
    notes_path = output_prefix.with_name(output_prefix.name + "_diagnostics.md")
    xlsx_path = output_prefix.with_name(output_prefix.name + "_recommended_classification.xlsx")

    candidates.to_csv(candidates_path, index=False, encoding="utf-8-sig")
    data_quality_path.write_text(json.dumps(data_quality, ensure_ascii=False, indent=2), encoding="utf-8")
    assignments.to_csv(assignments_path, index=False, encoding="utf-8-sig")
    catalog.to_csv(catalog_path, index=False, encoding="utf-8-sig")
    prototypes.to_csv(prototypes_path, index=False, encoding="utf-8-sig")
    per_cluster_metrics.to_csv(per_cluster_path, index=False, encoding="utf-8-sig")
    pca_coords.to_csv(pca_path, index=False, encoding="utf-8-sig")
    plot_paths = write_clustering_plots(
        pca_coords=pca_coords,
        catalog=catalog,
        feature_cols=feature_cols,
        output_prefix=output_prefix.with_name(output_prefix.name + "_recommended"),
        pca_summary=pca_summary,
    )

    best_metrics = {
        **metrics,
        **pca_summary,
        "recommended_candidate": best,
        "health": health_label(best),
        "data_quality": data_quality,
    }
    best_metrics_path.write_text(json.dumps(json_safe(best_metrics), ensure_ascii=False, indent=2), encoding="utf-8")

    with pd.ExcelWriter(xlsx_path, engine="openpyxl") as writer:
        candidates.to_excel(writer, sheet_name="candidate_scores", index=False)
        prototypes.to_excel(writer, sheet_name="recommended_prototypes", index=False)
        catalog.to_excel(writer, sheet_name="recommended_catalog", index=False)
        assignments.to_excel(writer, sheet_name="recommended_assignments", index=False)
        per_cluster_metrics.to_excel(writer, sheet_name="per_cluster_metrics", index=False)
        pca_coords.to_excel(writer, sheet_name="pca_coordinates", index=False)

    notes_path.write_text(
        build_diagnostics_markdown(best_metrics, candidates.head(12), data_quality, plot_paths),
        encoding="utf-8",
    )

    report = {
        "input_file": str(input_file),
        "sheet_name": sheet_name,
        "candidate_count": int(len(candidates)),
        "recommended": best,
        "health": best_metrics["health"],
        "outputs": {
            "candidates": str(candidates_path),
            "data_quality": str(data_quality_path),
            "recommended_metrics": str(best_metrics_path),
            "recommended_assignments": str(assignments_path),
            "recommended_catalog": str(catalog_path),
            "recommended_prototypes_bestauto": str(prototypes_path),
            "recommended_per_cluster_metrics": str(per_cluster_path),
            "recommended_pca_coordinates": str(pca_path),
            "recommended_classification_xlsx": str(xlsx_path),
            "diagnostics": str(notes_path),
            **plot_paths,
        },
    }
    report_path.write_text(json.dumps(json_safe(report), ensure_ascii=False, indent=2), encoding="utf-8")
    return report


def build_diagnostics_markdown(
    best_metrics: dict,
    top_candidates: pd.DataFrame,
    data_quality: dict,
    plot_paths: dict,
) -> str:
    best = best_metrics["recommended_candidate"]
    table_cols = [
        "strategy",
        "transform",
        "k",
        "local_k_divisor",
        "cluster_count",
        "silhouette_score",
        "davies_bouldin_score",
        "small_cluster_sample_share",
        "stability_ari_mean",
        "candidate_score",
        "health",
    ]
    top_table = top_candidates[table_cols].copy()
    for col in top_table.columns:
        if pd.api.types.is_float_dtype(top_table[col]):
            top_table[col] = top_table[col].map(lambda v: "" if pd.isna(v) else f"{v:.4f}")
    markdown_rows = [
        "| " + " | ".join(table_cols) + " |",
        "| " + " | ".join(["---"] * len(table_cols)) + " |",
    ]
    for _, row in top_table.iterrows():
        markdown_rows.append("| " + " | ".join(str(row[col]) for col in table_cols) + " |")
    lines = [
        "# primary组成聚类优化诊断",
        "",
        "## 数据合理性",
        "",
        f"- 原始地块数：{data_quality['raw_block_count']}。",
        f"- 可聚类地块数：{data_quality['clusterable_block_count']}；零primary向量：{data_quality['zero_primary_vector_count']}。",
        f"- 有效维度：{data_quality['feature_count']}；归一化后行和范围：{data_quality['row_sum_min']:.6f} - {data_quality['row_sum_max']:.6f}。",
        f"- `未映射`均值：{data_quality['unmapped_pct_mean']:.4f}%，最大值：{data_quality['unmapped_pct_max']:.4f}%。",
        f"- 单一主导度均值：{data_quality['dominant_pct_mean']:.2f}%，中位数：{data_quality['dominant_pct_median']:.2f}%。",
        f"- 主导占比 >=80% 的样本占比：{data_quality['dominant_pct_ge_80_share']:.2%}；>=95%：{data_quality['dominant_pct_ge_95_share']:.2%}。",
        "",
        "## 推荐方案",
        "",
        f"- 策略：`{best['strategy']}`。",
        f"- 组成数据变换：`{best['transform']}`。",
        f"- 聚类数：{int(best_metrics['cluster_count'])}。",
        f"- 健康判断：{best_metrics['health']}。",
        f"- Silhouette：{best_metrics['silhouette_score']:.4f}，越接近 1 越好。",
        f"- Davies-Bouldin：{best_metrics['davies_bouldin_score']:.4f}，越小越好。",
        f"- Calinski-Harabasz：{best_metrics['calinski_harabasz_score']:.4f}，越大越好。",
        f"- 稳定性 ARI：{best.get('stability_ari_mean', None)}，越接近 1 越稳定。",
        "",
        "## Top候选",
        "",
        "\n".join(markdown_rows),
        "",
        "## 图",
        "",
    ]
    for name, path in plot_paths.items():
        lines.append(f"- `{name}`：`{path}`。")
    lines.extend(
        [
            "",
            "## 判断口径",
            "",
            "- 如果 `silhouette >= 0.50`、`DB <= 1.00`、`ARI >= 0.75` 且小簇样本占比低，可视为较理想。",
            "- 如果 `silhouette` 在 `0.35-0.50`，通常是可解释但边界较软的城市用地混合数据，需要人工复核原型。",
            "- 如果小簇很多或稳定性低，不建议直接作为正式分类体系。",
        ]
    )
    return "\n".join(lines)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Optimize clustering for 10D primary land-use composition vectors.")
    parser.add_argument(
        "--input-file",
        type=pathlib.Path,
        default=DEFAULT_DATA_DIR / "block_cluster_feature_matrix_primary_merged_with_elements.xlsx",
    )
    parser.add_argument("--sheet-name", default="primary_merged_matrix")
    parser.add_argument(
        "--output-prefix",
        type=pathlib.Path,
        default=DEFAULT_DATA_DIR / "block_cluster_primary_composition_optimized",
    )
    parser.add_argument("--include-unmapped", action="store_true")
    parser.add_argument("--min-k", type=int, default=12)
    parser.add_argument("--max-k", type=int, default=52)
    parser.add_argument("--k-step", type=int, default=4)
    parser.add_argument("--transforms", default="raw_percent,hellinger,clr")
    parser.add_argument("--two-stage-divisors", default="30")
    parser.add_argument("--stability-top-n", type=int, default=8)
    parser.add_argument("--stability-runs", type=int, default=5)
    parser.add_argument("--random-state", type=int, default=42)
    args = parser.parse_args()

    result = optimize_primary_composition_clustering(
        input_file=args.input_file,
        output_prefix=args.output_prefix,
        sheet_name=args.sheet_name,
        include_unmapped=args.include_unmapped,
        min_k=args.min_k,
        max_k=args.max_k,
        k_step=args.k_step,
        transforms_to_try=tuple(v.strip() for v in args.transforms.split(",") if v.strip()),
        two_stage_divisors=tuple(int(v.strip()) for v in args.two_stage_divisors.split(",") if v.strip()),
        stability_top_n=args.stability_top_n,
        stability_runs=args.stability_runs,
        random_state=args.random_state,
    )
    print(json.dumps(json_safe(result), ensure_ascii=False, indent=2))
