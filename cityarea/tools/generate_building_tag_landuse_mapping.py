import json
import pathlib
import re
from collections import Counter, defaultdict

import pandas as pd


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
WXWORK_DIR = pathlib.Path(r"C:\Users\Pixel\Documents\WXWork\1688857186721619\Cache\File\2026-04")
REPORT_DIR = REPO_ROOT / "reports" / "landuse_tag_mapping"


PLANNING_PRIMARY = {
    "01": "耕地",
    "02": "园地",
    "03": "林地",
    "04": "草地",
    "06": "农业设施建设用地",
    "07": "居住用地",
    "08": "公共管理与公共服务用地",
    "09": "商业服务业用地",
    "10": "工矿用地",
    "11": "仓储用地",
    "12": "交通运输用地",
    "13": "公用设施用地",
    "14": "绿地与开敞空间用地",
    "15": "特殊用地",
    "16": "留白/待开发用地",
}


TAG_MAPPING = {
    # 居住
    "residential": ("07", "0701", "城镇住宅用地", "住宅建筑/居住用地，强证据", 1.0),
    "apartments": ("07", "0701", "城镇住宅用地", "公寓住宅，强证据", 1.0),
    "garages": ("07", "0702", "城镇社区服务设施用地", "车库常作为居住配套，弱到中等证据", 0.6),
    # 公共管理与公共服务
    "school": ("08", "0804", "教育用地", "学校，强证据", 1.0),
    "kindergarten": ("08", "0804", "教育用地", "幼儿园，强证据", 1.0),
    "university": ("08", "0804", "教育用地", "高等教育，强证据", 1.0),
    "college": ("08", "0804", "教育用地", "教育/学院，强证据", 1.0),
    "education": ("08", "0804", "教育用地", "教育设施，强证据", 1.0),
    "hospital": ("08", "0806", "医疗卫生用地", "医院/医疗设施，强证据", 1.0),
    "research_institute": ("08", "0804", "科研教育用地", "科研机构，强证据", 1.0),
    "government": ("08", "0801", "机关团体用地", "政府机关，强证据", 1.0),
    "governmental": ("08", "0801", "机关团体用地", "政府/行政办公，强证据", 1.0),
    "civic_admin": ("08", "0801", "机关团体用地", "公共行政，强证据", 1.0),
    "community_centre": ("08", "0807", "社会福利/社区服务用地", "社区中心，强证据", 0.9),
    "museum": ("08", "0803", "文化用地", "博物馆，强证据", 1.0),
    "theatre": ("08", "0803", "文化用地", "剧院文化设施，强证据", 1.0),
    "arts_centre": ("08", "0803", "文化用地", "艺术中心，强证据", 1.0),
    "studio": ("08", "0803", "文化用地", "工作室/文化创作空间，中等证据", 0.7),
    "sports_centre": ("08", "0805", "体育用地", "体育中心，强证据", 1.0),
    "stadium": ("08", "0805", "体育用地", "体育场，强证据", 1.0),
    "sports_hall": ("08", "0805", "体育用地", "体育馆，强证据", 1.0),
    "pitch": ("08", "0805", "体育用地", "运动场，强证据", 0.9),
    "track": ("08", "0805", "体育用地", "运动跑道，强证据", 0.9),
    "fitness_centre": ("08", "0805", "体育用地", "健身设施，若商业运营可归09；此处按体育服务", 0.7),
    # 商业服务业
    "commercial": ("09", "0901", "商业用地", "商业建筑，强证据", 1.0),
    "retail": ("09", "0901", "商业用地", "零售商业，强证据", 1.0),
    "marketplace": ("09", "0901", "商业用地", "市场，强证据", 1.0),
    "mall": ("09", "0901", "商业用地", "购物中心，强证据", 1.0),
    "restaurant": ("09", "0901", "商业用地", "餐饮商业，强证据", 1.0),
    "hotel": ("09", "0901", "旅馆用地", "酒店旅馆，强证据", 1.0),
    "resort": ("09", "0901", "旅馆/旅游服务用地", "度假设施，强证据", 0.9),
    "office": ("09", "0902", "商务金融用地", "办公商务，强证据", 1.0),
    "conference_centre": ("09", "0902", "商务会议服务用地", "会议中心，中高证据", 0.8),
    "theme_park": ("09", "0904", "娱乐康体/旅游服务用地", "主题公园，多为商业娱乐", 0.8),
    "water_park": ("09", "0904", "娱乐康体/旅游服务用地", "水上乐园，多为商业娱乐", 0.8),
    "zoo": ("09", "0904", "旅游服务/游乐设施用地", "动物园，可属公共文化或旅游服务，此处按游乐服务", 0.7),
    "aquarium": ("09", "0904", "旅游服务/游乐设施用地", "水族馆，可属公共文化或旅游服务，此处按游乐服务", 0.7),
    "attraction": ("09", "0904", "旅游服务用地", "景点设施，语义较泛", 0.6),
    "car": ("09", "0901", "商业用地", "汽车销售/服务更接近商业", 0.7),
    # 工矿、仓储、交通、公用设施
    "industrial": ("10", "1001", "工业用地", "工业设施，强证据", 1.0),
    "warehouse": ("11", "1101", "物流仓储用地", "仓储设施，强证据", 1.0),
    "parking": ("12", "1208", "交通场站/停车设施用地", "停车设施，强证据", 0.9),
    "railway": ("12", "1201", "铁路用地", "铁路设施，强证据", 1.0),
    "train_station": ("12", "1201", "铁路场站用地", "火车站，强证据", 1.0),
    "bus_station": ("12", "1207", "公共交通场站用地", "公交场站，强证据", 1.0),
    "fuel": ("12", "1208", "交通服务设施用地", "加油站，常作为交通服务设施", 0.8),
    "driver_training": ("12", "1208", "交通服务设施用地", "驾校/训练场，交通服务", 0.8),
    "wastewater_plant": ("13", "1302", "排水用地", "污水处理设施，强证据", 1.0),
    "water_works": ("13", "1301", "供水用地", "供水设施，强证据", 1.0),
    "landfill": ("13", "1309", "环卫用地", "垃圾填埋/环卫设施，强证据", 1.0),
    "fire_station": ("13", "1310", "消防用地", "消防设施，强证据", 1.0),
    "monitoring_station": ("13", "1304", "环境/市政监测设施用地", "监测站，语义偏公用设施", 0.8),
    # 农林草、绿地开敞、待开发
    "farmland": ("01", "0103", "旱地/农用地", "农田，强证据", 1.0),
    "orchard": ("02", "0201", "果园", "果园，强证据", 1.0),
    "vineyard": ("02", "0204", "其他园地", "葡萄园，强证据", 1.0),
    "forest": ("03", "0301", "乔木林地", "森林/林地，强证据", 1.0),
    "grass": ("04", "0402", "人工草地/草地", "草地，强证据", 0.9),
    "meadow": ("04", "0401", "天然牧草地/草地", "草甸，强证据", 0.9),
    "farmyard": ("06", "0602", "设施农业用地", "农业院落/农场设施", 0.9),
    "greenhouse": ("06", "0602", "种植设施建设用地", "温室，强证据", 0.9),
    "greenhouse_horticulture": ("06", "0602", "种植设施建设用地", "园艺温室，强证据", 0.9),
    "plant_nursery": ("02", "0204", "苗圃/其他园地", "苗圃，强证据", 0.9),
    "park": ("14", "1401", "公园绿地", "公园绿地，强证据", 1.0),
    "garden": ("14", "1401", "公园绿地/附属绿地", "花园绿地，中高证据", 0.9),
    "golf_course": ("14", "1403", "广场/开敞活动用地", "高尔夫开敞场地，按绿地开敞处理", 0.8),
    "recreation_ground": ("14", "1403", "开敞活动用地", "休闲活动场地", 0.8),
    "common": ("14", "1403", "开敞空间用地", "公共开敞地，语义较泛", 0.6),
    "nature_reserve": ("03", "0301", "自然保护/林草生态空间", "自然保护地，不一定是城市建设用地", 0.7),
    "greenfield": ("16", "1601", "留白/待开发用地", "未开发绿地，待开发证据", 0.9),
    "brownfield": ("16", "1601", "留白/待开发用地", "棕地/再开发地块，强待开发证据", 1.0),
    # 特殊
    "military": ("15", "1501", "军事设施用地", "军事设施，强证据", 1.0),
    "prison": ("15", "1502", "监教场所用地", "监狱，强证据", 1.0),
    "cemetery": ("15", "1506", "殡葬用地", "墓地，强证据", 1.0),
    "place_of_worship": ("15", "1505", "宗教用地", "宗教场所，强证据", 1.0),
    "religious": ("15", "1505", "宗教用地", "宗教设施，强证据", 1.0),
    "horse_riding": ("14", "1403", "开敞活动/体育休闲用地", "骑马活动场地，偏开敞休闲", 0.6),
    # 弱语义/不建议强分类
    "yes": ("00", "0000", "通用建筑/未细分", "OSM building=yes，仅表示有建筑，不提供明确用地功能", 0.0),
    "roof": ("00", "0000", "构筑物/屋顶", "屋顶结构，不提供用地功能", 0.0),
}


SCENE_ASSET_MAPPING = {
    "Buildings": {"primary_code": "00", "primary": "建筑资产/需结合语义标签判定", "comment": "scene中建筑mesh只说明形态，不足以判定用地"},
    "Vegetation": {"primary_code": "14", "primary": "绿地与开敞空间用地", "comment": "植被资产可作为绿地景观弱证据"},
    "Rocks": {"primary_code": "14", "primary": "绿地与开敞空间用地", "comment": "岩石景观资产可作为开敞景观弱证据"},
    "Landscape": {"primary_code": "14", "primary": "绿地与开敞空间用地", "comment": "山体/景观背景资产"},
}


def parse_signature(text: str) -> list[tuple[str, float]]:
    out = []
    for part in str(text).split("|"):
        match = re.match(r"\s*([^:]+):\s*([0-9.]+)%", part.strip())
        if match:
            out.append((match.group(1).strip(), float(match.group(2))))
    return out


def find_excel() -> pathlib.Path:
    files = [p for p in WXWORK_DIR.glob("*.xlsx") if not p.name.startswith("~$") and p.stat().st_size > 1000]
    if not files:
        raise FileNotFoundError("No usable xlsx found in WXWork cache.")
    return files[0]


def build_mapping_rows(tag_counts: Counter) -> list[dict]:
    rows = []
    for tag, count in sorted(tag_counts.items()):
        code, secondary, tertiary, comment, weight = TAG_MAPPING.get(
            tag,
            ("00", "0000", "未映射", "未出现在人工映射表，需复核", 0.0),
        )
        rows.append(
            {
                "tag": tag,
                "occurrence_in_cluster_signatures": int(count),
                "primary_code": code,
                "primary": PLANNING_PRIMARY.get(code, "未映射/弱语义"),
                "secondary_code": secondary,
                "secondary_or_tertiary_hint": tertiary,
                "classification_weight": float(weight),
                "use_for_landuse_classification": bool(weight > 0),
                "comment": comment,
            }
        )
    return rows


def aggregate_primary(signature: str) -> tuple[str, str, str]:
    sums = defaultdict(float)
    evidence = []
    ignored = []
    for tag, pct in parse_signature(signature):
        code, _secondary, _tertiary, _comment, weight = TAG_MAPPING.get(tag, ("00", "0000", "未映射", "未映射", 0.0))
        if weight > 0 and code != "00":
            primary = PLANNING_PRIMARY[code]
            sums[primary] += pct * weight
            evidence.append(f"{tag}->{primary}:{pct:.2f}%*{weight:g}")
        else:
            ignored.append(f"{tag}:{pct:.2f}%")
    if not sums:
        return "未映射", "", "ignored=" + " | ".join(ignored[:8])
    total = sum(sums.values())
    ordered = sorted(sums.items(), key=lambda item: item[1], reverse=True)
    signature_text = " | ".join(f"{name}:{value / total * 100:.2f}%" for name, value in ordered)
    return ordered[0][0], signature_text, "evidence=" + " | ".join(evidence[:10]) + ("; ignored=" + " | ".join(ignored[:8]) if ignored else "")


def main() -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    excel_path = find_excel()
    df = pd.read_excel(excel_path)

    tag_counts = Counter()
    for signature in df["top_signature"].dropna().astype(str):
        for tag, _pct in parse_signature(signature):
            tag_counts[tag] += 1

    mapping_rows = build_mapping_rows(tag_counts)
    mapping_df = pd.DataFrame(mapping_rows).sort_values(
        ["use_for_landuse_classification", "primary_code", "tag"],
        ascending=[False, True, True],
    )

    enriched = df.copy()
    aggregates = enriched["top_signature"].fillna("").map(aggregate_primary)
    enriched["mapped_primary"] = [item[0] for item in aggregates]
    enriched["mapped_primary_signature"] = [item[1] for item in aggregates]
    enriched["mapping_evidence"] = [item[2] for item in aggregates]

    mapping_json = {
        "planning_basis": {
            "source_doc": r"C:\Users\Pixel\Documents\WXWork\1688857186721619\Cache\File\2026-06\P020231127391946850971.doc",
            "note": "依据《国土空间调查、规划、用途管制用地用海分类指南》一级类框架，并结合城市地块分类需求补充“待开发用地”。",
            "primary_classes": PLANNING_PRIMARY,
        },
        "scene_asset_mapping": SCENE_ASSET_MAPPING,
        "tag_mapping": {row["tag"]: {k: v for k, v in row.items() if k != "tag"} for row in mapping_rows},
    }

    json_path = REPORT_DIR / "building_tag_landuse_mapping.json"
    csv_path = REPORT_DIR / "building_tag_landuse_mapping.csv"
    enriched_path = REPORT_DIR / "beijing_block_clusters_with_landuse_mapping.xlsx"
    summary_path = REPORT_DIR / "building_tag_landuse_mapping_summary.md"

    json_path.write_text(json.dumps(mapping_json, ensure_ascii=False, indent=2), encoding="utf-8")
    mapping_df.to_csv(csv_path, index=False, encoding="utf-8-sig")
    with pd.ExcelWriter(enriched_path, engine="openpyxl") as writer:
        enriched.to_excel(writer, sheet_name="clusters_with_landuse", index=False)
        mapping_df.to_excel(writer, sheet_name="tag_mapping", index=False)

    primary_counts = mapping_df[mapping_df["use_for_landuse_classification"]]["primary"].value_counts()
    weak_tags = mapping_df[~mapping_df["use_for_landuse_classification"]]["tag"].tolist()
    summary_path.write_text(
        "\n".join(
            [
                "# 建筑标签到规划用地分类映射说明",
                "",
                "## 方法",
                "",
                "- 从 Excel 的 `top_signature` 提取英文建筑/场景标签。",
                "- 依据 P020231127391946850971.doc 中的国土空间用地分类一级类框架进行映射。",
                "- `scene.json` 只提供资产前缀参考：Buildings/Vegetation/Rocks/Landscape，不能直接替代用地功能分类。",
                "- `yes`、`roof` 这类弱语义标签不参与强分类，避免把“有建筑”误判成具体用地。",
                "",
                "## 映射统计",
                "",
                f"- 标签总数：{len(mapping_df)}。",
                f"- 可用于用地分类的标签：{int(mapping_df['use_for_landuse_classification'].sum())}。",
                f"- 弱语义/未映射标签：{len(weak_tags)}，包括：{', '.join(weak_tags)}。",
                "",
                "## 一级类覆盖",
                "",
                *[f"- {name}: {count} 个标签" for name, count in primary_counts.items()],
                "",
                "## 输出",
                "",
                f"- `{json_path}`",
                f"- `{csv_path}`",
                f"- `{enriched_path}`",
            ]
        ),
        encoding="utf-8",
    )
    print(json.dumps({"json": str(json_path), "csv": str(csv_path), "xlsx": str(enriched_path), "summary": str(summary_path)}, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
