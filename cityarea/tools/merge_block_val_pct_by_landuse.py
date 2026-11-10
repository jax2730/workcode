from __future__ import annotations

import argparse
import importlib.util
import json
import pathlib
import sys
from typing import Any

import pandas as pd


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_MAPPING_CSV = REPO_ROOT / "data" / "tag_classify" / "multi_city_priority_tag_values_20260623_210813_unique_values_landuse_mapping.csv"
CLASSIFIER_PATH = REPO_ROOT / "tools" / "classify_priority_tag_landuse.py"


SUPPLEMENTAL_BLOCK_TAG_MAPPING: dict[str, dict[str, str | bool]] = {
    "alcohol": {"primary_code": "09", "secondary_code": "0901", "tertiary_code": "090101", "use": True, "source": "block_supplemental_shop"},
    "apartments_residential": {"primary_code": "07", "secondary_code": "0701", "tertiary_code": "070102", "use": True, "source": "block_supplemental_residential"},
    "architect": {"primary_code": "09", "secondary_code": "0902", "tertiary_code": "", "use": True, "source": "block_supplemental_business_office"},
    "barracks": {"primary_code": "15", "secondary_code": "1501", "tertiary_code": "", "use": True, "source": "block_supplemental_military"},
    "beacon": {"primary_code": "12", "secondary_code": "1209", "tertiary_code": "", "use": True, "source": "block_supplemental_transport_aid"},
    "biergarten": {"primary_code": "09", "secondary_code": "0901", "tertiary_code": "090103", "use": True, "source": "block_supplemental_dining"},
    "boathouse": {"primary_code": "12", "secondary_code": "1204", "tertiary_code": "", "use": True, "source": "block_supplemental_harbor_storage"},
    "bridge": {"primary_code": "12", "secondary_code": "1207", "tertiary_code": "", "use": True, "source": "block_supplemental_transport_structure"},
    "bureau_de_change": {"primary_code": "09", "secondary_code": "0902", "tertiary_code": "", "use": True, "source": "block_supplemental_finance"},
    "commercial_yes": {"primary_code": "09", "secondary_code": "0901", "tertiary_code": "", "use": True, "source": "block_supplemental_commercial"},
    "communications_tower": {"primary_code": "13", "secondary_code": "1306", "tertiary_code": "", "use": True, "source": "block_supplemental_telecom"},
    "computer": {"primary_code": "09", "secondary_code": "0901", "tertiary_code": "090101", "use": True, "source": "block_supplemental_shop"},
    "courtyard": {"primary_code": "00", "secondary_code": "0000", "tertiary_code": "", "use": False, "source": "block_supplemental_not_direct_landuse"},
    "dancing_school": {"primary_code": "08", "secondary_code": "0804", "tertiary_code": "080405", "use": True, "source": "block_supplemental_education"},
    "data_center": {"primary_code": "13", "secondary_code": "1306", "tertiary_code": "", "use": True, "source": "block_supplemental_telecom"},
    "electricity": {"primary_code": "13", "secondary_code": "1303", "tertiary_code": "", "use": True, "source": "block_supplemental_power"},
    "employment_agency": {"primary_code": "09", "secondary_code": "0902", "tertiary_code": "", "use": True, "source": "block_supplemental_business_office"},
    "exercise_point": {"primary_code": "08", "secondary_code": "0805", "tertiary_code": "080501", "use": True, "source": "block_supplemental_sport"},
    "fabric": {"primary_code": "09", "secondary_code": "0901", "tertiary_code": "090101", "use": True, "source": "block_supplemental_shop"},
    "financial": {"primary_code": "09", "secondary_code": "0902", "tertiary_code": "", "use": True, "source": "block_supplemental_finance"},
    "hardware": {"primary_code": "09", "secondary_code": "0901", "tertiary_code": "090101", "use": True, "source": "block_supplemental_shop"},
    "industrial_yes": {"primary_code": "10", "secondary_code": "1001", "tertiary_code": "", "use": True, "source": "block_supplemental_industrial"},
    "insurance": {"primary_code": "09", "secondary_code": "0902", "tertiary_code": "", "use": True, "source": "block_supplemental_finance"},
    "karaoke_box": {"primary_code": "09", "secondary_code": "0903", "tertiary_code": "", "use": True, "source": "block_supplemental_entertainment"},
    "notary": {"primary_code": "09", "secondary_code": "0902", "tertiary_code": "", "use": True, "source": "block_supplemental_business_office"},
    "satellite_dish": {"primary_code": "00", "secondary_code": "0000", "tertiary_code": "", "use": False, "source": "block_supplemental_not_direct_landuse"},
    "skyscraper": {"primary_code": "00", "secondary_code": "0000", "tertiary_code": "", "use": False, "source": "block_supplemental_building_form"},
    "stage": {"primary_code": "00", "secondary_code": "0000", "tertiary_code": "", "use": False, "source": "block_supplemental_not_direct_landuse"},
    "static_caravan": {"primary_code": "09", "secondary_code": "0901", "tertiary_code": "090104", "use": True, "source": "block_supplemental_lodging"},
    "stock_exchange": {"primary_code": "09", "secondary_code": "0902", "tertiary_code": "", "use": True, "source": "block_supplemental_finance"},
    "subway": {"primary_code": "12", "secondary_code": "1206", "tertiary_code": "", "use": True, "source": "block_supplemental_urban_rail"},
    "terrace": {"primary_code": "07", "secondary_code": "0701", "tertiary_code": "070102", "use": True, "source": "block_supplemental_residential"},
    "therapist": {"primary_code": "08", "secondary_code": "0806", "tertiary_code": "080602", "use": True, "source": "block_supplemental_clinic"},
    "trade": {"primary_code": "09", "secondary_code": "0901", "tertiary_code": "090101", "use": True, "source": "block_supplemental_shop"},
    "ventilation_shaft": {"primary_code": "00", "secondary_code": "0000", "tertiary_code": "", "use": False, "source": "block_supplemental_not_direct_landuse"},
    "yes_roof": {"primary_code": "00", "secondary_code": "0000", "tertiary_code": "", "use": False, "source": "block_supplemental_not_direct_landuse"},
}


def load_classifier_module():
    spec = importlib.util.spec_from_file_location("classify_priority_tag_landuse", CLASSIFIER_PATH)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def normalize_code(value: Any, width: int) -> str:
    text = "" if pd.isna(value) else str(value).strip()
    if not text:
        return ""
    if text.endswith(".0"):
        text = text[:-2]
    return text.zfill(width)


def parse_bool(value: Any) -> bool:
    return str(value).strip().lower() in {"true", "1", "yes", "y"}


def build_tag_mapping(block_tags: list[str], mapping_csv: pathlib.Path, classifier) -> pd.DataFrame:
    rows: list[dict[str, Any]] = []
    source_by_tag: dict[str, dict[str, Any]] = {}

    if mapping_csv.exists():
        mapping_df = pd.read_csv(mapping_csv, dtype=str, encoding="utf-8-sig")
        for _, row in mapping_df.iterrows():
            tag = str(row.get("tag", "")).strip()
            if not tag:
                continue
            source_by_tag[tag] = {
                "tag": tag,
                "tag_zh": row.get("tag_zh", classifier.tag_to_chinese_name(tag)),
                "primary_code": normalize_code(row.get("primary_code", ""), 2),
                "secondary_code": normalize_code(row.get("secondary_code", ""), 4),
                "tertiary_code": normalize_code(row.get("tertiary_code", ""), 6),
                "use_for_landuse_classification": parse_bool(row.get("use_for_landuse_classification", False)),
                "mapping_source": str(row.get("matched_by", "tag_landuse_mapping")),
                "comment": str(row.get("comment", "")),
            }

    for tag in block_tags:
        if tag in source_by_tag:
            item = source_by_tag[tag]
        elif tag in SUPPLEMENTAL_BLOCK_TAG_MAPPING:
            supplement = SUPPLEMENTAL_BLOCK_TAG_MAPPING[tag]
            item = {
                "tag": tag,
                "tag_zh": classifier.tag_to_chinese_name(tag),
                "primary_code": str(supplement["primary_code"]),
                "secondary_code": str(supplement["secondary_code"]),
                "tertiary_code": str(supplement["tertiary_code"]),
                "use_for_landuse_classification": bool(supplement["use"]),
                "mapping_source": str(supplement["source"]),
                "comment": "block矩阵补充映射：该tag不在原始标签分类表中，按OSM常见语义补充。",
            }
        else:
            item = {
                "tag": tag,
                "tag_zh": classifier.tag_to_chinese_name(tag),
                "primary_code": "00",
                "secondary_code": "0000",
                "tertiary_code": "",
                "use_for_landuse_classification": False,
                "mapping_source": "unknown_unmapped",
                "comment": "未在标签分类表或block补充映射中找到，保留为未映射。",
            }

        primary_code = item["primary_code"] if item["primary_code"] in classifier.PLANNING_PRIMARY else "00"
        secondary_code = item["secondary_code"] if item["secondary_code"] in classifier.PLANNING_SECONDARY else ("0000" if primary_code == "00" else "")
        tertiary_code = item["tertiary_code"] if item["tertiary_code"] in classifier.PLANNING_TERTIARY else ""
        rows.append(
            {
                "tag": tag,
                "tag_zh": item["tag_zh"],
                "source_column": f"val_pct_{tag}",
                "primary_code": primary_code,
                "primary": classifier.PLANNING_PRIMARY.get(primary_code, ""),
                "secondary_code": secondary_code,
                "secondary": classifier.PLANNING_SECONDARY.get(secondary_code, "") if secondary_code else "",
                "tertiary_code": tertiary_code,
                "tertiary": classifier.PLANNING_TERTIARY.get(tertiary_code, "") if tertiary_code else "",
                "use_for_landuse_classification": bool(item["use_for_landuse_classification"]),
                "mapping_source": item["mapping_source"],
                "comment": item["comment"],
            }
        )

    return pd.DataFrame(rows)


def build_outputs(block_df: pd.DataFrame, tag_mapping: pd.DataFrame, classifier) -> tuple[pd.DataFrame, pd.DataFrame, pd.DataFrame]:
    val_cols = [column for column in block_df.columns if column.startswith("val_pct_")]
    id_cols = [column for column in block_df.columns if not column.startswith("val_pct_")]
    col_to_tag = {f"val_pct_{row.tag}": row for row in tag_mapping.itertuples(index=False)}

    mapping = tag_mapping.copy()
    mapping["total_val_pct_sum"] = [block_df[f"val_pct_{tag}"].sum() for tag in mapping["tag"]]
    mapping["nonzero_block_count"] = [(block_df[f"val_pct_{tag}"] > 0).sum() for tag in mapping["tag"]]

    long_rows: list[dict[str, Any]] = []
    merged = block_df[id_cols].copy()

    primary_codes = sorted(set(mapping["primary_code"]) | {"00"})
    for primary_code in primary_codes:
        primary_tag_cols = [
            f"val_pct_{row.tag}"
            for row in mapping[mapping["primary_code"] == primary_code].itertuples(index=False)
            if f"val_pct_{row.tag}" in block_df.columns
        ]
        merged[f"landuse_pct_{primary_code}"] = block_df[primary_tag_cols].sum(axis=1) if primary_tag_cols else 0.0

    merged["total_val_pct_sum"] = block_df[val_cols].sum(axis=1)
    merged["mapped_val_pct_sum"] = sum(
        merged[f"landuse_pct_{code}"] for code in primary_codes if code != "00"
    ) if primary_codes else 0.0
    merged["unmapped_val_pct_sum"] = merged.get("landuse_pct_00", 0.0)

    landuse_cols = [f"landuse_pct_{code}" for code in primary_codes]
    dominant_codes: list[str] = []
    dominant_names: list[str] = []
    dominant_pcts: list[float] = []
    primary_json_values: list[str] = []
    tag_json_values: list[str] = []

    for index, row in block_df.iterrows():
        primary_parts = []
        for code in primary_codes:
            pct = float(merged.at[index, f"landuse_pct_{code}"])
            if pct > 0:
                primary_parts.append({"primary_code": code, "primary": classifier.PLANNING_PRIMARY.get(code, ""), "pct": round(pct, 6)})
        primary_parts = sorted(primary_parts, key=lambda item: item["pct"], reverse=True)
        primary_json_values.append(json.dumps(primary_parts, ensure_ascii=False))

        if primary_parts:
            dominant_codes.append(primary_parts[0]["primary_code"])
            dominant_names.append(primary_parts[0]["primary"])
            dominant_pcts.append(primary_parts[0]["pct"])
        else:
            dominant_codes.append("00")
            dominant_names.append(classifier.PLANNING_PRIMARY["00"])
            dominant_pcts.append(0.0)

        tag_parts = []
        for column in val_cols:
            pct = float(row[column])
            if pct <= 0:
                continue
            mapped = col_to_tag[column]
            tag_item = {
                "tag": mapped.tag,
                "tag_zh": mapped.tag_zh,
                "val_pct": round(pct, 6),
                "primary_code": mapped.primary_code,
                "primary": mapped.primary,
                "secondary_code": mapped.secondary_code,
                "secondary": mapped.secondary,
                "tertiary_code": mapped.tertiary_code,
                "tertiary": mapped.tertiary,
                "mapping_source": mapped.mapping_source,
            }
            tag_parts.append(tag_item)
            long_rows.append({**{col: row[col] for col in id_cols}, **tag_item})
        tag_parts = sorted(tag_parts, key=lambda item: item["val_pct"], reverse=True)
        tag_json_values.append(json.dumps(tag_parts, ensure_ascii=False))

    merged.insert(len(id_cols), "dominant_landuse_code", dominant_codes)
    merged.insert(len(id_cols) + 1, "dominant_landuse", dominant_names)
    merged.insert(len(id_cols) + 2, "dominant_landuse_pct", dominant_pcts)
    merged["landuse_pct_json"] = primary_json_values
    merged["val_pct_elements_json"] = tag_json_values

    long_df = pd.DataFrame(long_rows)
    return merged, long_df, mapping.sort_values(["primary_code", "tag"]).reset_index(drop=True)


def write_outputs(merged: pd.DataFrame, long_df: pd.DataFrame, mapping: pd.DataFrame, output_prefix: pathlib.Path) -> dict[str, str]:
    output_prefix.parent.mkdir(parents=True, exist_ok=True)
    merged_csv = output_prefix.with_name(f"{output_prefix.name}_landuse_merged.csv")
    long_csv = output_prefix.with_name(f"{output_prefix.name}_val_pct_composition_long.csv")
    mapping_csv = output_prefix.with_name(f"{output_prefix.name}_val_pct_tag_mapping.csv")
    xlsx_path = output_prefix.with_name(f"{output_prefix.name}_landuse_merged.xlsx")

    merged.to_csv(merged_csv, index=False, encoding="utf-8-sig")
    long_df.to_csv(long_csv, index=False, encoding="utf-8-sig")
    mapping.to_csv(mapping_csv, index=False, encoding="utf-8-sig")
    with pd.ExcelWriter(xlsx_path, engine="openpyxl") as writer:
        merged.to_excel(writer, sheet_name="block_landuse_merged", index=False)
        long_df.to_excel(writer, sheet_name="val_pct_composition_long", index=False)
        mapping.to_excel(writer, sheet_name="val_pct_tag_mapping", index=False)

    return {
        "merged_csv": str(merged_csv),
        "long_csv": str(long_csv),
        "mapping_csv": str(mapping_csv),
        "xlsx": str(xlsx_path),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Merge block val_pct tag columns into planning land-use percentages.")
    parser.add_argument("input_csv", type=pathlib.Path)
    parser.add_argument("--mapping-csv", type=pathlib.Path, default=DEFAULT_MAPPING_CSV)
    parser.add_argument("--output-prefix", type=pathlib.Path, default=None)
    args = parser.parse_args()

    classifier = load_classifier_module()
    block_df = pd.read_csv(args.input_csv)
    val_cols = [column for column in block_df.columns if column.startswith("val_pct_")]
    block_tags = [column.removeprefix("val_pct_") for column in val_cols]
    output_prefix = args.output_prefix or args.input_csv.with_suffix("")

    tag_mapping = build_tag_mapping(block_tags, args.mapping_csv, classifier)
    merged, long_df, mapping = build_outputs(block_df, tag_mapping, classifier)
    paths = write_outputs(merged, long_df, mapping, output_prefix)

    result = {
        **paths,
        "rows": int(len(block_df)),
        "val_pct_columns": int(len(val_cols)),
        "composition_rows": int(len(long_df)),
        "unmapped_tag_count": int((mapping["primary_code"] == "00").sum()),
        "zero_val_pct_block_count": int((merged["total_val_pct_sum"] == 0).sum()),
    }
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
