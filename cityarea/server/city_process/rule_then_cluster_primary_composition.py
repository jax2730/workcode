import argparse
import json
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
from server.city_process.optimize_primary_composition_clustering import transform_composition


def split_high_purity_blocks(
    x_percent: pd.DataFrame,
    feature_cols: list[str],
    threshold: float = 90.0,
) -> pd.DataFrame:
    dominant_cols = x_percent[feature_cols].idxmax(axis=1)
    dominant_pct = x_percent[feature_cols].max(axis=1)
    return pd.DataFrame(
        {
            "dominant_primary": dominant_cols.map(_primary_name),
            "dominant_pct": dominant_pct.astype(float),
            # 严格使用 > threshold：90.00% 正好仍进入混合部分，避免边界样本被硬塞进纯类。
            "is_high_purity": dominant_pct > float(threshold),
        },
        index=x_percent.index,
    )


def eligible_high_purity_names(split: pd.DataFrame, min_count: int = 2) -> set[str]:
    counts = split.loc[split["is_high_purity"], "dominant_primary"].value_counts()
    return set(counts[counts >= int(min_count)].index.astype(str))


def _fit_kmeans(x: pd.DataFrame, k: int, random_state: int) -> np.ndarray:
    from sklearn.cluster import MiniBatchKMeans

    km = MiniBatchKMeans(n_clusters=int(k), random_state=int(random_state), n_init=8, max_iter=200, batch_size=1024)
    return km.fit_predict(x.to_numpy(dtype=float))


def _mean_stability_ari(x: pd.DataFrame, k: int, seeds: list[int]) -> float:
    from sklearn.metrics import adjusted_rand_score

    labels = [_fit_kmeans(x, k=k, random_state=seed) for seed in seeds]
    scores = []
    for i in range(len(labels)):
        for j in range(i + 1, len(labels)):
            scores.append(adjusted_rand_score(labels[i], labels[j]))
    return float(np.mean(scores)) if scores else 0.0


def _evaluate_mixed_k(
    x: pd.DataFrame,
    labels: np.ndarray,
    min_cluster_size: int,
    random_state: int,
) -> dict:
    from sklearn.metrics import calinski_harabasz_score, davies_bouldin_score, silhouette_score

    x_arr = x.to_numpy(dtype=float)
    sizes = pd.Series(labels).value_counts()
    return {
        "mixed_k": int(len(set(labels.tolist()))),
        "mixed_silhouette": float(silhouette_score(x_arr, labels, metric="euclidean", sample_size=min(800, len(labels)), random_state=random_state)),
        "mixed_davies_bouldin": float(davies_bouldin_score(x_arr, labels)),
        "mixed_calinski_harabasz": float(calinski_harabasz_score(x_arr, labels)),
        "mixed_min_cluster_size": int(sizes.min()),
        "mixed_max_cluster_size": int(sizes.max()),
        "mixed_small_cluster_share": float(sizes[sizes < int(min_cluster_size)].sum() / sizes.sum()),
    }


def _candidate_score(row: dict) -> float:
    sil = float(row["mixed_silhouette"])
    db = float(row["mixed_davies_bouldin"])
    ch = float(row["mixed_calinski_harabasz"])
    ari = float(row["mixed_stability_ari"])
    small_share = float(row["mixed_small_cluster_share"])
    return float(
        (
            0.35 * max(0.0, (sil + 1.0) / 2.0)
            + 0.25 * (1.0 / (1.0 + max(db, 0.0)))
            + 0.15 * min(np.log1p(max(ch, 0.0)) / 7.0, 1.0)
            + 0.25 * ari
        )
        * (1.0 - small_share)
    )


def select_mixed_k(
    mixed_x: pd.DataFrame,
    k_values: list[int],
    min_cluster_size: int,
    random_state: int,
    stability_seeds: list[int],
) -> pd.DataFrame:
    rows = []
    for k in k_values:
        labels = _fit_kmeans(mixed_x, k=k, random_state=random_state)
        row = _evaluate_mixed_k(mixed_x, labels, min_cluster_size=min_cluster_size, random_state=random_state)
        row["mixed_stability_ari"] = _mean_stability_ari(mixed_x, k=k, seeds=stability_seeds)
        row["candidate_score"] = _candidate_score(row)
        rows.append(row)
    return pd.DataFrame(rows).sort_values("candidate_score", ascending=False).reset_index(drop=True)


def _signature(row: pd.Series, feature_cols: list[str], top_n: int = 5) -> str:
    pairs = [(_primary_name(col), float(row.get(col, 0.0))) for col in feature_cols]
    pairs = [(name, value) for name, value in pairs if value > 0]
    pairs.sort(key=lambda item: item[1], reverse=True)
    return " | ".join(f"{name}:{value:.2f}%" for name, value in pairs[:top_n])


def _build_catalog(
    x_raw: pd.DataFrame,
    feature_cols: list[str],
    labels: np.ndarray,
    cluster_sources: dict[int, str],
) -> pd.DataFrame:
    rows = []
    for cluster_id in sorted(set(int(v) for v in labels)):
        mask = labels == cluster_id
        centroid = x_raw.loc[mask, feature_cols].mean()
        text = build_classification_text(centroid, feature_cols)
        rows.append(
            {
                "primary_cluster_id": int(cluster_id),
                "macro_primary": text["macro_class"],
                "cluster_source": cluster_sources.get(int(cluster_id), "mixed_kmeans"),
                "block_count": int(mask.sum()),
                **text,
                **{col: float(centroid[col]) for col in feature_cols},
            }
        )
    return pd.DataFrame(rows)


def run_rule_then_mixed_clustering(
    input_file: pathlib.Path,
    output_prefix: pathlib.Path,
    sheet_name: str = "primary_merged_matrix",
    purity_threshold: float = 90.0,
    min_high_purity_class_count: int = 2,
    mixed_transform: str = "raw_percent",
    mixed_k_values: tuple[int, ...] = tuple(range(4, 21, 2)),
    min_cluster_size: int = 8,
    random_state: int = 42,
) -> dict:
    if input_file.suffix.lower() in {".xlsx", ".xls", ".xlsm"}:
        df = pd.read_excel(input_file, sheet_name=sheet_name)
    else:
        df = pd.read_csv(input_file)

    norm, feature_cols = normalize_primary_features(df, include_unmapped=False)
    work = norm[~norm["is_zero_primary_vector"]].copy().reset_index(drop=True)
    x_raw = work[feature_cols].astype(float).reset_index(drop=True)
    split = split_high_purity_blocks(x_raw, feature_cols, threshold=purity_threshold)
    eligible_names = eligible_high_purity_names(split, min_count=min_high_purity_class_count)

    pure_mask = (split["is_high_purity"] & split["dominant_primary"].isin(eligible_names)).to_numpy()
    mixed_mask = ~pure_mask
    pure_names = [_primary_name(col) for col in feature_cols if _primary_name(col) in eligible_names]
    pure_id_by_name = {name: idx for idx, name in enumerate(pure_names)}

    labels = np.full(len(x_raw), -1, dtype=int)
    cluster_sources = {}
    for name, cluster_id in pure_id_by_name.items():
        mask = pure_mask & (split["dominant_primary"].to_numpy() == name)
        if mask.any():
            labels[mask] = cluster_id
            cluster_sources[cluster_id] = f"rule_high_purity_gt_{purity_threshold:g}"

    mixed_x_raw = x_raw.loc[mixed_mask].reset_index(drop=True)
    mixed_x = transform_composition(mixed_x_raw, method=mixed_transform)
    candidate_scores = select_mixed_k(
        mixed_x=mixed_x,
        k_values=list(mixed_k_values),
        min_cluster_size=min_cluster_size,
        random_state=random_state,
        stability_seeds=[random_state, random_state + 101, random_state + 202],
    )
    best_k = int(candidate_scores.iloc[0]["mixed_k"])
    mixed_labels = _fit_kmeans(mixed_x, k=best_k, random_state=random_state)
    mixed_offset = len(pure_id_by_name)
    labels[mixed_mask] = mixed_labels + mixed_offset
    for local_id in sorted(set(int(v) for v in mixed_labels)):
        cluster_sources[mixed_offset + local_id] = f"mixed_kmeans_{mixed_transform}"

    catalog = _build_catalog(x_raw, feature_cols, labels, cluster_sources)
    prototypes = to_bestauto_prototypes(catalog)
    prototypes.insert(
        5,
        "cluster_source",
        prototypes["cluster_id"].map(catalog.set_index("primary_cluster_id")["cluster_source"]).fillna(""),
    )

    id_cols = [col for col in ["_block_id", "sector_id"] if col in work.columns]
    assignments = work[id_cols].copy()
    assignments["cluster_id"] = labels.astype(int)
    assignments["cluster_source"] = [cluster_sources[int(label)] for label in labels]
    assignments["dominant_primary"] = split["dominant_primary"].to_numpy()
    assignments["dominant_pct"] = split["dominant_pct"].to_numpy()
    assignments["is_high_purity"] = pure_mask
    assignments["primary_signature"] = x_raw.apply(lambda row: _signature(row, feature_cols), axis=1)

    mixed_metrics, mixed_per_cluster = compute_clustering_metrics(mixed_x, mixed_labels)
    final_metrics, final_per_cluster = compute_clustering_metrics(x_raw, labels)
    pca_coords, pca_summary = build_pca_projection(x_raw, labels)
    pca_coords = pd.concat(
        [assignments[[col for col in ["_block_id", "sector_id", "dominant_primary"] if col in assignments.columns]].reset_index(drop=True), pca_coords],
        axis=1,
    )
    pca_coords["macro_primary"] = assignments["dominant_primary"].to_numpy()

    output_prefix.parent.mkdir(parents=True, exist_ok=True)
    assignments_path = output_prefix.with_name(output_prefix.name + "_assignments.csv")
    catalog_path = output_prefix.with_name(output_prefix.name + "_catalog.csv")
    prototypes_path = output_prefix.with_name(output_prefix.name + "_prototypes_bestauto.csv")
    candidates_path = output_prefix.with_name(output_prefix.name + "_mixed_k_candidates.csv")
    metrics_path = output_prefix.with_name(output_prefix.name + "_metrics.json")
    pca_path = output_prefix.with_name(output_prefix.name + "_pca_coordinates.csv")
    xlsx_path = output_prefix.with_name(output_prefix.name + "_classification.xlsx")
    notes_path = output_prefix.with_name(output_prefix.name + "_diagnostics.md")

    assignments.to_csv(assignments_path, index=False, encoding="utf-8-sig")
    catalog.to_csv(catalog_path, index=False, encoding="utf-8-sig")
    prototypes.to_csv(prototypes_path, index=False, encoding="utf-8-sig")
    candidate_scores.to_csv(candidates_path, index=False, encoding="utf-8-sig")
    pca_coords.to_csv(pca_path, index=False, encoding="utf-8-sig")

    plot_paths = write_clustering_plots(
        pca_coords=pca_coords,
        catalog=catalog,
        feature_cols=feature_cols,
        output_prefix=output_prefix,
        pca_summary=pca_summary,
    )

    mixed_only_catalog = _build_catalog(
        mixed_x_raw,
        feature_cols,
        mixed_labels,
        {int(label): f"mixed_kmeans_{mixed_transform}" for label in sorted(set(int(v) for v in mixed_labels))},
    )
    mixed_only_pca_coords, mixed_only_pca_summary = build_pca_projection(mixed_x, mixed_labels)
    mixed_only_source = assignments.loc[mixed_mask, [col for col in ["_block_id", "sector_id", "dominant_primary"] if col in assignments.columns]].reset_index(drop=True)
    mixed_only_pca_coords = pd.concat([mixed_only_source, mixed_only_pca_coords], axis=1)
    mixed_only_pca_coords["macro_primary"] = mixed_only_pca_coords["dominant_primary"].to_numpy()
    mixed_only_prefix = output_prefix.with_name(output_prefix.name + "_mixed_only")
    mixed_only_pca_path = output_prefix.with_name(output_prefix.name + "_mixed_only_pca_coordinates.csv")
    mixed_only_catalog_path = output_prefix.with_name(output_prefix.name + "_mixed_only_catalog.csv")
    mixed_only_prototypes_path = output_prefix.with_name(output_prefix.name + "_mixed_only_prototypes_bestauto.csv")
    mixed_only_pca_coords.to_csv(mixed_only_pca_path, index=False, encoding="utf-8-sig")
    mixed_only_catalog.to_csv(mixed_only_catalog_path, index=False, encoding="utf-8-sig")
    to_bestauto_prototypes(mixed_only_catalog).to_csv(mixed_only_prototypes_path, index=False, encoding="utf-8-sig")
    mixed_only_plot_paths = write_clustering_plots(
        pca_coords=mixed_only_pca_coords,
        catalog=mixed_only_catalog,
        feature_cols=feature_cols,
        output_prefix=mixed_only_prefix,
        pca_summary=mixed_only_pca_summary,
    )

    pure_counts = split.loc[pure_mask, "dominant_primary"].value_counts().to_dict()
    mixed_counts = pd.Series(mixed_labels).value_counts().sort_index().to_dict()
    metrics = {
        "method": "rule_high_purity_then_mixed_kmeans",
        "purity_threshold": purity_threshold,
        "min_high_purity_class_count": int(min_high_purity_class_count),
        "high_purity_rule": "dominant_pct > purity_threshold",
        "raw_block_count": int(len(df)),
        "clusterable_block_count": int(len(work)),
        "zero_primary_vector_count": int(len(df) - len(work)),
        "high_purity_block_count": int(pure_mask.sum()),
        "mixed_block_count": int(mixed_mask.sum()),
        "high_purity_class_count": int(len(pure_counts)),
        "mixed_k": best_k,
        "final_cluster_count": int(len(set(labels.tolist()))),
        "mixed_transform": mixed_transform,
        "mixed_metrics": mixed_metrics,
        "final_reference_metrics": final_metrics,
        "pca_summary": pca_summary,
        "mixed_only_pca_summary": mixed_only_pca_summary,
        "high_purity_counts": {str(k): int(v) for k, v in pure_counts.items()},
        "excluded_high_purity_counts": {
            str(k): int(v)
            for k, v in split.loc[split["is_high_purity"] & ~split["dominant_primary"].isin(eligible_names), "dominant_primary"]
            .value_counts()
            .to_dict()
            .items()
        },
        "mixed_cluster_counts": {str(k + mixed_offset): int(v) for k, v in mixed_counts.items()},
        "outputs": {
            "assignments": str(assignments_path),
            "catalog": str(catalog_path),
            "prototypes_bestauto": str(prototypes_path),
            "mixed_k_candidates": str(candidates_path),
            "pca_coordinates": str(pca_path),
            "classification_xlsx": str(xlsx_path),
            "diagnostics": str(notes_path),
            "mixed_only_pca_coordinates": str(mixed_only_pca_path),
            "mixed_only_catalog": str(mixed_only_catalog_path),
            "mixed_only_prototypes_bestauto": str(mixed_only_prototypes_path),
            **plot_paths,
            **{f"mixed_only_{key}": value for key, value in mixed_only_plot_paths.items()},
        },
    }
    metrics_path.write_text(json.dumps(metrics, ensure_ascii=False, indent=2), encoding="utf-8")

    with pd.ExcelWriter(xlsx_path, engine="openpyxl") as writer:
        prototypes.to_excel(writer, sheet_name="prototypes_bestauto", index=False)
        catalog.to_excel(writer, sheet_name="catalog", index=False)
        assignments.to_excel(writer, sheet_name="assignments", index=False)
        candidate_scores.to_excel(writer, sheet_name="mixed_k_candidates", index=False)
        mixed_per_cluster.to_excel(writer, sheet_name="mixed_per_cluster_metrics", index=False)
        final_per_cluster.to_excel(writer, sheet_name="final_reference_metrics", index=False)
        pca_coords.to_excel(writer, sheet_name="pca_coordinates", index=False)
        mixed_only_catalog.to_excel(writer, sheet_name="mixed_only_catalog", index=False)
        mixed_only_pca_coords.to_excel(writer, sheet_name="mixed_only_pca", index=False)

    notes_path.write_text(
        "\n".join(
            [
                "# 90%高纯规则 + 混合地块聚类诊断",
                "",
                "## 方法",
                "",
                f"- 若某地块最大 primary 占比 `dominant_pct > {purity_threshold:g}%`，直接归入对应高纯类。",
                f"- 但高纯类必须至少有 {min_high_purity_class_count} 个样本；不足者并回混合地块。",
                "- 这一步是规则分类，不是统计聚类；它表达“几乎单一用地”的业务事实。",
                f"- 剩余 `dominant_pct <= {purity_threshold:g}%` 的混合地块，使用 `{mixed_transform}` 变换后做 KMeans。",
                f"- 混合部分自动比较 k 候选：{list(mixed_k_values)}，推荐 k={best_k}。",
                "",
                "## 可行性",
                "",
                f"- 高纯地块：{int(pure_mask.sum())}，覆盖 {len(pure_counts)} 个 primary 类。",
                f"- 混合地块：{int(mixed_mask.sum())}，主导占比最高 {float(split.loc[mixed_mask, 'dominant_pct'].max()):.2f}%。",
                "- 高纯样本过少的 primary 不单独作为 macro_bucket，而是进入混合聚类。",
                "",
                "## 混合聚类评分",
                "",
                f"- mixed_k：{best_k}。",
                f"- mixed silhouette：{mixed_metrics['silhouette_score']:.4f}。",
                f"- mixed Davies-Bouldin：{mixed_metrics['davies_bouldin_score']:.4f}。",
                f"- mixed Calinski-Harabasz：{mixed_metrics['calinski_harabasz_score']:.4f}。",
                "",
                "## 图的坐标轴",
                "",
                f"- 横轴 pca_x：PC1，解释方差 {pca_summary['pca_x_explained_variance_ratio']:.2%}。",
                f"- 纵轴 pca_y：PC2，解释方差 {pca_summary['pca_y_explained_variance_ratio']:.2%}。",
                "- PCA 只用于画图；规则分类和混合聚类都发生在 10 维 primary 组成比例空间。",
                "",
                "## 只看剩余混合地块的PCA",
                "",
                f"- 混合地块横轴 PC1 解释方差：{mixed_only_pca_summary['pca_x_explained_variance_ratio']:.2%}。",
                f"- 混合地块纵轴 PC2 解释方差：{mixed_only_pca_summary['pca_y_explained_variance_ratio']:.2%}。",
                f"- 混合地块 PC1+PC2 合计解释方差：{mixed_only_pca_summary['pca_total_explained_variance_ratio']:.2%}。",
            ]
        ),
        encoding="utf-8",
    )
    return metrics


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Assign >90% high-purity primary blocks, then cluster remaining mixed blocks.")
    parser.add_argument(
        "--input-file",
        type=pathlib.Path,
        default=DEFAULT_DATA_DIR / "block_cluster_feature_matrix_primary_merged_with_elements.xlsx",
    )
    parser.add_argument("--sheet-name", default="primary_merged_matrix")
    parser.add_argument(
        "--output-prefix",
        type=pathlib.Path,
        default=DEFAULT_DATA_DIR / "block_cluster_primary_rule90_mixed",
    )
    parser.add_argument("--purity-threshold", type=float, default=90.0)
    parser.add_argument("--min-high-purity-class-count", type=int, default=2)
    parser.add_argument("--mixed-transform", default="raw_percent")
    parser.add_argument("--mixed-k-values", default="4,6,8,10,12,14,16,18,20")
    parser.add_argument("--min-cluster-size", type=int, default=8)
    parser.add_argument("--random-state", type=int, default=42)
    args = parser.parse_args()

    result = run_rule_then_mixed_clustering(
        input_file=args.input_file,
        output_prefix=args.output_prefix,
        sheet_name=args.sheet_name,
        purity_threshold=args.purity_threshold,
        min_high_purity_class_count=args.min_high_purity_class_count,
        mixed_transform=args.mixed_transform,
        mixed_k_values=tuple(int(v.strip()) for v in args.mixed_k_values.split(",") if v.strip()),
        min_cluster_size=args.min_cluster_size,
        random_state=args.random_state,
    )
    print(json.dumps(result, ensure_ascii=False, indent=2))
