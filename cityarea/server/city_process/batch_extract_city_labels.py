from __future__ import annotations

import argparse
import importlib.util
import json
import os
import pathlib
import sys
import time
from dataclasses import asdict, dataclass
from datetime import datetime
from typing import Callable

import pandas as pd


# Fill city names here. The server can also override with --cities.
CITY_LIST: list[str] = [
    "Beijing",
    "Tianjin",
    "Shijiazhuang",
    "Tangshan",
    "Zhangjiakou",
    "Baoding",
    "Chengde",
    "Hengshui",
    "Langfang",
    "Qinhuangdao",
    "Xingtai",
]

PRIORITY_SURFACE_KEYS = ("landuse", "amenity", "leisure", "tourism", "building", "man_made")
PROJECT_ROOT_ENV = "CITYAREA_PROJECT_ROOT"
DATA_DIR_ENV = "CITYAREA_DATA_DIR"
SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
OUTPUT_PREFIX = "multi_city_priority_tag_values"


def _find_priority_builder_paths(root: pathlib.Path) -> tuple[pathlib.Path, pathlib.Path] | None:
    """Return (overpass_dir, constants_path) for supported layouts."""
    candidates = [
        # Server screenshot layout:
        # root/city_process, root/overpass_query, root/constants.py
        (root / "overpass_query", root / "constants.py"),
        # Local repository layout:
        # root/server/city_process, root/server/overpass_query, root/server/constants.py
        (root / "server" / "overpass_query", root / "server" / "constants.py"),
    ]
    for overpass_dir, constants_path in candidates:
        if (
            (overpass_dir / "query_priority_blocks.py").exists()
            and (overpass_dir / "osm_util.py").exists()
            and constants_path.exists()
        ):
            return overpass_dir, constants_path
    return None


def _has_priority_builder(root: pathlib.Path) -> bool:
    return _find_priority_builder_paths(root) is not None


def get_priority_builder_paths(root: pathlib.Path) -> tuple[pathlib.Path, pathlib.Path]:
    paths = _find_priority_builder_paths(root)
    if paths is None:
        raise FileNotFoundError(
            "Cannot find priority builder. Expected either "
            "overpass_query/query_priority_blocks.py + constants.py, or "
            "server/overpass_query/query_priority_blocks.py + server/constants.py"
        )
    return paths


def find_project_root(start: pathlib.Path | None = None) -> pathlib.Path:
    """Find project root for both the flat server layout and local repo layout."""
    env_root = os.environ.get(PROJECT_ROOT_ENV)
    if env_root:
        root = pathlib.Path(env_root).expanduser().resolve()
        if _has_priority_builder(root):
            return root
        raise FileNotFoundError(
            f"{PROJECT_ROOT_ENV} is set but project files are missing under: {root}"
        )

    search_start = (start or SCRIPT_DIR).resolve()
    candidates = [
        search_start,
        *search_start.parents,
        pathlib.Path.cwd().resolve(),
        *pathlib.Path.cwd().resolve().parents,
    ]
    seen: set[pathlib.Path] = set()
    for candidate in candidates:
        if candidate in seen:
            continue
        seen.add(candidate)
        if _has_priority_builder(candidate):
            return candidate

    raise FileNotFoundError(
        "Cannot find project root. Set CITYAREA_PROJECT_ROOT to the directory containing "
        "overpass_query/query_priority_blocks.py or server/overpass_query/query_priority_blocks.py"
    )


PROJECT_ROOT = find_project_root()
OVERPASS_DIR, CONSTANTS_PATH = get_priority_builder_paths(PROJECT_ROOT)


def _load_module_from_file(module_name: str, module_path: pathlib.Path, package: str | None = None):
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    if spec is None or spec.loader is None:
        raise ImportError(f"Cannot load module from file: {module_path}")
    module = importlib.util.module_from_spec(spec)
    module.__package__ = package
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return module


def _load_constants_module():
    return _load_module_from_file("cityarea_constants", CONSTANTS_PATH, package=None)


def resolve_data_dir() -> pathlib.Path:
    env_data_dir = os.environ.get(DATA_DIR_ENV)
    if env_data_dir:
        return pathlib.Path(env_data_dir).expanduser().resolve()

    constants_module = _load_constants_module()
    data_dir = getattr(constants_module, "DATA_DIR", pathlib.Path("/home/pixel/geo_tools/data"))
    return pathlib.Path(data_dir).expanduser().resolve()


DATA_DIR = resolve_data_dir()
CITY_ROOT = DATA_DIR / "city"
OUTPUT_DIR = DATA_DIR / "csv_classify"


@dataclass
class PriorityBuildOptions:
    city_root: pathlib.Path = CITY_ROOT
    radius: int = 50000
    skip_online: bool = False
    skip_building: bool = False
    min_building_area: float = 0.0
    building_chunk_size: int = 20000
    building_tile_size: float = 4000.0
    building_workers: int = 0
    log: bool = False


@dataclass
class CityProcessResult:
    city: str
    status: str
    priority_status: str
    priority_path: str
    value_count: int
    unique_value_count: int
    message: str
    duration_seconds: float


def priority_blocks_path(city_name: str, city_root: pathlib.Path = CITY_ROOT) -> pathlib.Path:
    return city_root / city_name / "priority_blocks.gpkg"


def _load_query_priority_blocks_module():
    """Load project query_priority_blocks.py by exact file path, avoiding global package collisions."""
    constants_module = _load_constants_module()
    query_priority_path = OVERPASS_DIR / "query_priority_blocks.py"
    osm_util_path = OVERPASS_DIR / "osm_util.py"

    old_constants = sys.modules.get("constants")
    old_osm_util = sys.modules.get("osm_util")
    old_path = list(sys.path)
    try:
        sys.modules["constants"] = constants_module
        overpass_path = str(OVERPASS_DIR)
        if overpass_path not in sys.path:
            sys.path.insert(0, overpass_path)

        osm_util = _load_module_from_file("osm_util", osm_util_path, package=None)
        sys.modules["osm_util"] = osm_util

        return _load_module_from_file(
            "cityarea_query_priority_blocks",
            query_priority_path,
            package=None,
        )
    finally:
        sys.path[:] = old_path
        if old_constants is None:
            sys.modules.pop("constants", None)
        else:
            sys.modules["constants"] = old_constants

        if old_osm_util is None:
            sys.modules.pop("osm_util", None)
        else:
            sys.modules["osm_util"] = old_osm_util


def _generate_priority_blocks(city_name: str, build_options: PriorityBuildOptions) -> None:
    module = _load_query_priority_blocks_module()
    module.CITY_DIR = build_options.city_root
    module.query_priority_blocks(
        city_name=city_name,
        radius=build_options.radius,
        skip_online=build_options.skip_online,
        skip_building=build_options.skip_building,
        min_building_area=build_options.min_building_area,
        building_chunk_size=build_options.building_chunk_size,
        building_tile_size=build_options.building_tile_size,
        building_workers=build_options.building_workers,
        log=build_options.log,
    )


def ensure_priority_blocks(
    city_name: str,
    build_options: PriorityBuildOptions,
    generator: Callable[[str, PriorityBuildOptions], None] | None = None,
) -> tuple[pathlib.Path, str]:
    path = priority_blocks_path(city_name, build_options.city_root)
    if path.exists():
        return path, "existing"

    path.parent.mkdir(parents=True, exist_ok=True)
    build = generator or _generate_priority_blocks
    build(city_name, build_options)

    if not path.exists():
        raise FileNotFoundError(f"priority_blocks.gpkg not generated: {path}")
    return path, "generated"


def read_priority_blocks(priority_path: pathlib.Path) -> pd.DataFrame:
    import geopandas as gpd

    return gpd.read_file(priority_path, layer="priority_blocks")


def _clean_value_series(series: pd.Series) -> pd.Series:
    return series.astype(str).str.strip()


def extract_value_rows(city_name: str, priority_blocks: pd.DataFrame) -> pd.DataFrame:
    columns = ["city", "source_category", "value", "area_m2"]
    if len(priority_blocks) == 0:
        return pd.DataFrame(columns=columns)

    if "category" not in priority_blocks.columns or "value" not in priority_blocks.columns:
        raise ValueError("priority_blocks layer must contain category and value columns")

    work = priority_blocks.copy()
    work["category"] = _clean_value_series(work["category"])
    value_not_empty = work["value"].notna() & _clean_value_series(work["value"]).ne("")
    category_allowed = work["category"].isin(PRIORITY_SURFACE_KEYS)
    work = work[category_allowed & value_not_empty].copy()

    if len(work) == 0:
        return pd.DataFrame(columns=columns)

    if "area_m2" in work.columns:
        area = pd.to_numeric(work["area_m2"], errors="coerce").fillna(0.0)
    else:
        area = pd.Series([0.0] * len(work), index=work.index)
        if "geometry" in work.columns and hasattr(work, "to_crs"):
            try:
                area = work.to_crs(epsg=3857).geometry.area
            except Exception:
                area = pd.Series([0.0] * len(work), index=work.index)

    rows = pd.DataFrame(
        {
            "city": city_name,
            "source_category": work["category"].astype(str),
            "value": _clean_value_series(work["value"]),
            "area_m2": area.astype(float),
        }
    )
    return rows.reset_index(drop=True)


def summarize_value_rows(rows: pd.DataFrame) -> pd.DataFrame:
    columns = ["city", "source_category", "value", "feature_count", "area_m2_sum"]
    if len(rows) == 0:
        return pd.DataFrame(columns=columns)

    summary = (
        rows.groupby(["city", "source_category", "value"], as_index=False)
        .agg(feature_count=("value", "size"), area_m2_sum=("area_m2", "sum"))
        .sort_values(["city", "source_category", "feature_count", "value"], ascending=[True, True, False, True])
        .reset_index(drop=True)
    )
    summary["area_m2_sum"] = summary["area_m2_sum"].round(3)
    return summary[columns]


def _join_unique(values: pd.Series) -> str:
    return "|".join(sorted({str(value) for value in values if str(value).strip()}))


def build_unique_value_catalog(summary: pd.DataFrame) -> pd.DataFrame:
    columns = ["value", "source_categories", "cities", "total_count", "area_m2_sum"]
    if len(summary) == 0:
        return pd.DataFrame(columns=columns)

    catalog = (
        summary.groupby("value", as_index=False)
        .agg(
            source_categories=("source_category", _join_unique),
            cities=("city", _join_unique),
            total_count=("feature_count", "sum"),
            area_m2_sum=("area_m2_sum", "sum"),
        )
        .sort_values(["total_count", "value"], ascending=[False, True])
        .reset_index(drop=True)
    )
    catalog["area_m2_sum"] = catalog["area_m2_sum"].round(3)
    return catalog[columns]


def process_city(
    city_name: str,
    build_options: PriorityBuildOptions,
    generator: Callable[[str, PriorityBuildOptions], None] | None = None,
) -> tuple[pd.DataFrame, CityProcessResult]:
    started = time.perf_counter()
    priority_path = ""
    priority_status = ""

    try:
        path, priority_status = ensure_priority_blocks(city_name, build_options, generator=generator)
        priority_path = str(path)
        priority_blocks = read_priority_blocks(path)
        rows = extract_value_rows(city_name, priority_blocks)
        result = CityProcessResult(
            city=city_name,
            status="ok",
            priority_status=priority_status,
            priority_path=priority_path,
            value_count=len(rows),
            unique_value_count=int(rows["value"].nunique()) if len(rows) else 0,
            message="completed",
            duration_seconds=round(time.perf_counter() - started, 3),
        )
        return rows, result
    except Exception as exc:
        result = CityProcessResult(
            city=city_name,
            status="failed",
            priority_status=priority_status,
            priority_path=priority_path,
            value_count=0,
            unique_value_count=0,
            message=f"{type(exc).__name__}: {exc}",
            duration_seconds=round(time.perf_counter() - started, 3),
        )
        return pd.DataFrame(columns=["city", "source_category", "value", "area_m2"]), result


def write_outputs(
    all_rows: pd.DataFrame,
    results: list[CityProcessResult],
    output_dir: pathlib.Path = OUTPUT_DIR,
    output_prefix: str = OUTPUT_PREFIX,
) -> dict[str, pathlib.Path]:
    output_dir.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    base = output_dir / f"{output_prefix}_{timestamp}"

    raw_path = base.with_name(base.name + "_raw.csv")
    summary_path = base.with_name(base.name + "_summary.csv")
    catalog_path = base.with_name(base.name + "_unique_values.csv")
    report_path = base.with_name(base.name + "_report.json")

    summary = summarize_value_rows(all_rows)
    catalog = build_unique_value_catalog(summary)

    all_rows.to_csv(raw_path, index=False, encoding="utf-8-sig")
    summary.to_csv(summary_path, index=False, encoding="utf-8-sig")
    catalog.to_csv(catalog_path, index=False, encoding="utf-8-sig")
    report_path.write_text(json.dumps([asdict(result) for result in results], ensure_ascii=False, indent=2), encoding="utf-8")

    return {
        "raw": raw_path,
        "summary": summary_path,
        "unique_values": catalog_path,
        "report": report_path,
    }


def _split_cities(text: str) -> list[str]:
    return [item.strip() for item in text.replace(";", ",").split(",") if item.strip()]


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Extract only OSM value labels from multi-city priority_blocks.gpkg.")
    parser.add_argument("--cities", type=str, default=None, help="Optional override, e.g. Beijing,Shanghai. If omitted, use CITY_LIST in code.")
    parser.add_argument("--city-root", type=pathlib.Path, default=CITY_ROOT, help="Directory containing city folders")
    parser.add_argument("--output-dir", type=pathlib.Path, default=OUTPUT_DIR, help="CSV output directory")
    parser.add_argument("--radius", type=int, default=50000, help="Radius for generating missing priority_blocks.gpkg")
    parser.add_argument("--skip-online", action="store_true", help="When generating missing priority_blocks.gpkg, skip online/local Overpass extra query")
    parser.add_argument("--skip-building", action="store_true", help="When generating missing priority_blocks.gpkg, skip building polygons")
    parser.add_argument("--min-building-area", type=float, default=0.0, help="Minimum building area in m2 before priority clipping")
    parser.add_argument("--building-workers", type=int, default=0, help="Workers for building clipping; 0 means CPU count")
    parser.add_argument("--log", action="store_true", help="Print detailed generation logs")
    return parser


def main() -> int:
    args = build_arg_parser().parse_args()
    cities = _split_cities(args.cities) if args.cities else CITY_LIST
    cities = [city for city in cities if city.strip()]
    if not cities:
        raise ValueError("CITY_LIST is empty. Fill CITY_LIST = ['Beijing', ...] in this file, or pass --cities.")

    build_options = PriorityBuildOptions(
        city_root=args.city_root,
        radius=args.radius,
        skip_online=args.skip_online,
        skip_building=args.skip_building,
        min_building_area=args.min_building_area,
        building_workers=args.building_workers,
        log=args.log,
    )

    all_frames: list[pd.DataFrame] = []
    results: list[CityProcessResult] = []
    for index, city_name in enumerate(cities, start=1):
        print(f"[{index}/{len(cities)}] {city_name}")
        rows, result = process_city(city_name, build_options)
        all_frames.append(rows)
        results.append(result)
        print(f"  {result.status}: priority={result.priority_status}, values={result.value_count}, unique={result.unique_value_count}")
        if result.status == "failed":
            print(f"  {result.message}")

    all_rows = pd.concat(all_frames, ignore_index=True) if all_frames else pd.DataFrame(columns=["city", "source_category", "value", "area_m2"])
    output_paths = write_outputs(all_rows, results, output_dir=args.output_dir)

    for name, path in output_paths.items():
        print(f"{name}: {path}")

    failed = [result for result in results if result.status == "failed"]
    unique_count = all_rows["value"].nunique() if len(all_rows) else 0
    print(f"Summary: cities={len(results)}, failed={len(failed)}, value_rows={len(all_rows)}, unique_values={unique_count}")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
