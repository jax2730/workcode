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
    normalize_primary_features,
    to_bestauto_prototypes,
)
from server.city_process.rule_then_cluster_primary_composition import (
    eligible_high_purity_names,
    split_high_purity_blocks,
)


def make_topk_signature(
    row: pd.Series,
    feature_cols: list[str],
    top_k: int = 3,
    min_component_pct: float = 5.0,
) -> str:
    pairs = []
    for col in feature_cols:
        value = float(row.get(col, 0.0))
        if value >= float(min_component_pct):
            pairs.append((_primary_name(col), value))
    pairs.sort(key=lambda item: item[1], reverse=True)
    return " + ".join(name for name, _ in pairs[: int(top_k)])


def _top_indices(values: np.ndarray, top_k: int) -> set[int]:
    positive = np.where(values > 0)[0]
    if len(positive) == 0:
        return set()
    ordered = positive[np.argsort(values[positive])[::-1]]
    return set(int(v) for v in ordered[: int(top_k)])


def evaluate_reconstruction(
    x_percent: pd.DataFrame,
    labels: pd.Series | np.ndarray,
    feature_cols: list[str],
) -> tuple[dict, pd.DataFrame]:
    x = x_percent[feature_cols].astype(float).reset_index(drop=True)
    label_series = pd.Series(labels).reset_index(drop=True).astype(str)

    prototypes = x.groupby(label_series).mean()
    tv_errors = []
    top1_hits = []
    top2_overlaps = []
    top3_overlaps = []
    prototype_top3_mass = []

    for idx, row in x.iterrows():
        label = label_series.iloc[idx]
        proto = prototypes.loc[label].to_numpy(dtype=float)
        values = row.to_numpy(dtype=float)

        # Total variation distance：0.5 * Σ|p_i - q_i|。
        # 对百分比组成来说，它表示用“类别原型”替代原始地块组成时，平均需要移动多少百分点。
        tv_errors.append(float(np.abs(values - proto).sum() / 2.0))

        row_top1 = int(np.argmax(values))
        proto_top1 = int(np.argmax(proto))
        top1_hits.append(1.0 if row_top1 == proto_top1 else 0.0)

        row_top2 = _top_indices(values, 2)
        proto_top2 = _top_indices(proto, 2)
        top2_overlaps.append(len(row_top2 & proto_top2) / max(1, min(2, len(row_top2))))

        row_top3 = _top_indices(values, 3)
        proto_top3 = _top_indices(proto, 3)
        top3_overlaps.append(len(row_top3 & proto_top3) / max(1, min(3, len(row_top3))))
        prototype_top3_mass.append(float(values[list(proto_top3)].sum()) if proto_top3 else 0.0)

    sizes = label_series.value_counts()
    per_rows = []
    for label, size in sizes.items():
        mask = label_series == label
        proto_row = prototypes.loc[label]
        text = build_classification_text(proto_row, feature_cols)
        per_rows.append(
            {
                "cluster_key": label,
                "block_count": int(size),
                "tv_error_mean": float(np.mean(np.asarray(tv_errors)[mask.to_numpy()])),
                "top1_accuracy": float(np.mean(np.asarray(top1_hits)[mask.to_numpy()])),
                "top2_overlap_mean": float(np.mean(np.asarray(top2_overlaps)[mask.to_numpy()])),
                "top3_overlap_mean": float(np.mean(np.asarray(top3_overlaps)[mask.to_numpy()])),
                "prototype_top3_mass_mean": float(np.mean(np.asarray(prototype_top3_mass)[mask.to_numpy()])),
                **text,
                **{col: float(proto_row[col]) for col in feature_cols},
            }
        )

    metrics = {
        "sample_count": int(len(x)),
        "cluster_count": int(label_series.nunique()),
        "min_cluster_size": int(sizes.min()),
        "small_cluster_count_lt_10": int((sizes < 10).sum()),
        "small_cluster_sample_share_lt_10": float(sizes[sizes < 10].sum() / sizes.sum()),
        "tv_error_mean": float(np.mean(tv_errors)),
        "tv_error_median": float(np.median(tv_errors)),
        "tv_error_p75": float(np.percentile(tv_errors, 75)),
        "tv_error_p90": float(np.percentile(tv_errors, 90)),
        "top1_accuracy": float(np.mean(top1_hits)),
        "top2_overlap_mean": float(np.mean(top2_overlaps)),
        "top3_overlap_mean": float(np.mean(top3_overlaps)),
        "prototype_top3_mass_mean": float(np.mean(prototype_top3_mass)),
    }
    return metrics, pd.DataFrame(per_rows).sort_values(["block_count", "cluster_key"], ascending=[False, True])


def assign_signature_classes(
    x_percent: pd.DataFrame,
    feature_cols: list[str],
    high_purity_threshold: float,
    min_high_purity_class_count: int,
    top_k: int,
    min_component_pct: float,
    min_group_size: int,
) -> pd.DataFrame:
    x = x_percent[feature_cols].astype(float).reset_index(drop=True)
    split = split_high_purity_blocks(x, feature_cols, threshold=high_purity_threshold)
    eligible_names = eligible_high_purity_names(split, min_count=min_high_purity_class_count)
    out = split.copy()
    out["class_key"] = ""
    out["class_source"] = ""

    pure_mask = out["is_high_purity"] & out["dominant_primary"].isin(eligible_names)
    out.loc[pure_mask, "class_key"] = "PURE|" + out.loc[pure_mask, "dominant_primary"]
    out.loc[pure_mask, "class_source"] = "rule_high_purity"

    remaining = ~pure_mask
    for current_k in range(int(top_k), 0, -1):
        unassigned = remaining & (out["class_key"] == "")
        if not unassigned.any():
            break
        sigs = x.loc[unassigned].apply(
            lambda row: make_topk_signature(row, feature_cols, top_k=current_k, min_component_pct=min_component_pct),
            axis=1,
        )
        sigs = sigs.replace("", "其他")
        counts = sigs.value_counts()
        eligible_sigs = set(counts[counts >= int(min_group_size)].index.astype(str))
        assign_mask = unassigned.copy()
        assign_mask.loc[unassigned] = sigs.isin(eligible_sigs).to_numpy()
        out.loc[assign_mask, "class_key"] = [f"TOP{current_k}|{value}" for value in sigs[sigs.isin(eligible_sigs)]]
        out.loc[assign_mask, "class_source"] = f"top{current_k}_signature"

    still_unassigned = out["class_key"] == ""
    if still_unassigned.any():
        fallback = out.loc[still_unassigned, "dominant_primary"].astype(str)
        out.loc[still_unassigned, "class_key"] = "FALLBACK|" + fallback
        out.loc[still_unassigned, "class_source"] = "fallback_dominant_primary"

    return out


def _candidate_score(metrics: dict, target_min: int = 20, target_max: int = 45) -> float:
    cluster_count = int(metrics["cluster_count"])
    if target_min <= cluster_count <= target_max:
        count_factor = 1.0
    else:
        distance = min(abs(cluster_count - target_min), abs(cluster_count - target_max))
        count_factor = max(0.55, 1.0 - distance / 50.0)

    tv_factor = max(0.0, 1.0 - float(metrics["tv_error_mean"]) / 50.0)
    small_factor = max(0.0, 1.0 - float(metrics["small_cluster_sample_share_lt_10"]) * 3.0)
    return float(
        (
            0.25 * float(metrics["top1_accuracy"])
            + 0.25 * float(metrics["top2_overlap_mean"])
            + 0.25 * float(metrics["top3_overlap_mean"])
            + 0.20 * tv_factor
            + 0.05 * min(float(metrics["prototype_top3_mass_mean"]) / 100.0, 1.0)
        )
        * count_factor
        * small_factor
    )


def optimize_topk_signature_classes(
    input_file: pathlib.Path,
    output_prefix: pathlib.Path,
    sheet_name: str = "primary_merged_matrix",
    high_purity_threshold: float = 90.0,
    min_high_purity_class_count: int = 2,
    top_k_values: tuple[int, ...] = (2, 3),
    min_component_pct_values: tuple[float, ...] = (1.0, 3.0, 5.0, 8.0, 10.0),
    min_group_size_values: tuple[int, ...] = (10, 15, 20, 30),
) -> dict:
    if input_file.suffix.lower() in {".xlsx", ".xls", ".xlsm"}:
        df = pd.read_excel(input_file, sheet_name=sheet_name)
    else:
        df = pd.read_csv(input_file)

    norm, feature_cols = normalize_primary_features(df, include_unmapped=False)
    work = norm[~norm["is_zero_primary_vector"]].copy().reset_index(drop=True)
    x = work[feature_cols].astype(float).reset_index(drop=True)

    candidate_rows = []
    assignment_by_key = {}
    for top_k in top_k_values:
        for min_pct in min_component_pct_values:
            for min_group_size in min_group_size_values:
                assignments = assign_signature_classes(
                    x,
                    feature_cols,
                    high_purity_threshold=high_purity_threshold,
                    min_high_purity_class_count=min_high_purity_class_count,
                    top_k=top_k,
                    min_component_pct=min_pct,
                    min_group_size=min_group_size,
                )
                metrics, _ = evaluate_reconstruction(x, assignments["class_key"], feature_cols)
                score = _candidate_score(metrics)
                row = {
                    "top_k": int(top_k),
                    "min_component_pct": float(min_pct),
                    "min_group_size": int(min_group_size),
                    "candidate_score": score,
                    **metrics,
                }
                candidate_rows.append(row)
                assignment_by_key[(top_k, min_pct, min_group_size)] = assignments

    candidates = pd.DataFrame(candidate_rows).sort_values("candidate_score", ascending=False).reset_index(drop=True)
    best = candidates.iloc[0]
    best_key = (int(best["top_k"]), float(best["min_component_pct"]), int(best["min_group_size"]))
    assignments = assignment_by_key[best_key].copy()
    metrics, per_cluster = evaluate_reconstruction(x, assignments["class_key"], feature_cols)

    class_keys = sorted(assignments["class_key"].unique())
    id_by_key = {key: idx for idx, key in enumerate(class_keys)}
    assignments["cluster_id"] = assignments["class_key"].map(id_by_key).astype(int)

    id_cols = [col for col in ["_block_id", "sector_id"] if col in work.columns]
    output_assignments = work[id_cols].copy()
    output_assignments["cluster_id"] = assignments["cluster_id"]
    output_assignments["class_key"] = assignments["class_key"]
    output_assignments["class_source"] = assignments["class_source"]
    output_assignments["dominant_primary"] = assignments["dominant_primary"]
    output_assignments["dominant_pct"] = assignments["dominant_pct"]

    per_cluster["cluster_id"] = per_cluster["cluster_key"].map(id_by_key).astype(int)
    catalog = per_cluster.rename(
        columns={
            "cluster_key": "signature_key",
            "block_count": "block_count",
        }
    ).copy()
    catalog["primary_cluster_id"] = catalog["cluster_id"]
    catalog["macro_primary"] = catalog["macro_class"]
    catalog["cluster_source"] = catalog["signature_key"].str.split("|", n=1).str[0]
    prototypes = to_bestauto_prototypes(catalog)
    prototypes.insert(
        5,
        "signature_key",
        prototypes["cluster_id"].map(catalog.set_index("cluster_id")["signature_key"]).fillna(""),
    )
    prototypes.insert(
        6,
        "cluster_source",
        prototypes["cluster_id"].map(catalog.set_index("cluster_id")["cluster_source"]).fillna(""),
    )
    for col in ["tv_error_mean", "top1_accuracy", "top2_overlap_mean", "top3_overlap_mean", "prototype_top3_mass_mean"]:
        prototypes[col] = prototypes["cluster_id"].map(catalog.set_index("cluster_id")[col])

    output_prefix.parent.mkdir(parents=True, exist_ok=True)
    candidates_path = output_prefix.with_name(output_prefix.name + "_candidates.csv")
    assignments_path = output_prefix.with_name(output_prefix.name + "_assignments.csv")
    catalog_path = output_prefix.with_name(output_prefix.name + "_catalog.csv")
    prototypes_path = output_prefix.with_name(output_prefix.name + "_prototypes_bestauto.csv")
    metrics_path = output_prefix.with_name(output_prefix.name + "_metrics.json")
    xlsx_path = output_prefix.with_name(output_prefix.name + "_classification.xlsx")
    notes_path = output_prefix.with_name(output_prefix.name + "_diagnostics.md")

    candidates.to_csv(candidates_path, index=False, encoding="utf-8-sig")
    output_assignments.to_csv(assignments_path, index=False, encoding="utf-8-sig")
    catalog.to_csv(catalog_path, index=False, encoding="utf-8-sig")
    prototypes.to_csv(prototypes_path, index=False, encoding="utf-8-sig")

    result = {
        "method": "rule_topk_signature_reconstruction",
        "raw_block_count": int(len(df)),
        "clusterable_block_count": int(len(work)),
        "zero_primary_vector_count": int(len(df) - len(work)),
        "best_parameters": {
            "high_purity_threshold": high_purity_threshold,
            "min_high_purity_class_count": min_high_purity_class_count,
            "top_k": best_key[0],
            "min_component_pct": best_key[1],
            "min_group_size": best_key[2],
        },
        "metrics": metrics,
        "outputs": {
            "candidates": str(candidates_path),
            "assignments": str(assignments_path),
            "catalog": str(catalog_path),
            "prototypes_bestauto": str(prototypes_path),
            "classification_xlsx": str(xlsx_path),
            "diagnostics": str(notes_path),
        },
    }
    metrics_path.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")

    with pd.ExcelWriter(xlsx_path, engine="openpyxl") as writer:
        candidates.to_excel(writer, sheet_name="candidate_scores", index=False)
        prototypes.to_excel(writer, sheet_name="prototypes_bestauto", index=False)
        catalog.to_excel(writer, sheet_name="catalog", index=False)
        output_assignments.to_excel(writer, sheet_name="assignments", index=False)

    notes_path.write_text(
        "\n".join(
            [
                "# top-k组成签名分类与原型重构诊断",
                "",
                "## 自优化流程",
                "",
                "1. 先用 `dominant_pct > 90%` 识别高纯规则类；样本数不足的高纯类不单独成类。",
                "2. 对剩余地块尝试 top-k 组成签名，参数包括 `top_k`、`min_component_pct`、`min_group_size`。",
                "3. 小样本签名自动回退：top3 -> top2 -> top1 -> dominant fallback。",
                "4. 每个候选方案计算原型重构指标，并选择综合分最高者。",
                "",
                "## 最佳参数",
                "",
                f"- top_k：{best_key[0]}。",
                f"- min_component_pct：{best_key[1]}%。",
                f"- min_group_size：{best_key[2]}。",
                "",
                "## 重构评分",
                "",
                f"- cluster_count：{metrics['cluster_count']}。",
                f"- TV error mean：{metrics['tv_error_mean']:.4f}，越小越好。",
                f"- TV error p75：{metrics['tv_error_p75']:.4f}。",
                f"- top1 accuracy：{metrics['top1_accuracy']:.4f}。",
                f"- top2 overlap：{metrics['top2_overlap_mean']:.4f}。",
                f"- top3 overlap：{metrics['top3_overlap_mean']:.4f}。",
                f"- prototype top3 mass recall：{metrics['prototype_top3_mass_mean']:.4f}%。",
                "",
                "## 评判标准",
                "",
                "- `top1_accuracy >= 0.90`：主导用地还原较可靠。",
                "- `top2_overlap >= 0.75`：前两个主要组成较可靠。",
                "- `top3_overlap >= 0.70`：前三组成有可解释性。",
                "- `TV error mean <= 15`：用类别原型替代单个地块时，平均偏差较小。",
                "- 类别数建议保持在 20-45，过多会碎片化，过少会损失还原能力。",
            ]
        ),
        encoding="utf-8",
    )
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Classify blocks by high-purity rules and top-k composition signatures.")
    parser.add_argument(
        "--input-file",
        type=pathlib.Path,
        default=DEFAULT_DATA_DIR / "block_cluster_feature_matrix_primary_merged_with_elements.xlsx",
    )
    parser.add_argument("--sheet-name", default="primary_merged_matrix")
    parser.add_argument(
        "--output-prefix",
        type=pathlib.Path,
        default=DEFAULT_DATA_DIR / "block_cluster_primary_topk_reconstruction",
    )
    args = parser.parse_args()
    output = optimize_topk_signature_classes(
        input_file=args.input_file,
        output_prefix=args.output_prefix,
        sheet_name=args.sheet_name,
    )
    print(json.dumps(output, ensure_ascii=False, indent=2))
