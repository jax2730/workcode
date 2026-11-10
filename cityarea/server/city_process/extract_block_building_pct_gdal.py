from __future__ import annotations

"""按地块提取 OSM 标签组成。

最终矩阵只使用标签的原始 ``value`` 作为列名；``building``、``amenity`` 等
category 仅参与内部比例计算。同名 value 跨 category 合并，例如
``building=school`` 与 ``amenity=school`` 最终共同写入 ``*_school``。

当 block 图层包含 ``cat_pct_*`` 时，单个标签的组成比例为：

    val_pct(b, c, v) = area_pixels(b, c, v) / area_pixels(b, c, *) * effective_cat_pct(b, c)

``tagged_area_pct`` 表示标签几何在地块中的实际覆盖率；
``building_pct_total`` 表示标签组成比例总和，二者不再混用。

如果某些 ``cat_pct_*`` 类别没有对应标签要素，不能直接把这些类别的权重丢掉，
否则总和会系统性低于 100%。脚本会先对地块内实际出现的类别重新归一化；
如果类别权重字段不完整，则使用这些类别的栅格像元数作为类别权重。
"""

import argparse
import json
import math
import pathlib
import re
import sys
import tempfile
from dataclasses import dataclass
from typing import Any

import numpy as np
import pandas as pd


gpd = None
gdal = None
ogr = None


def ensure_gis_imports() -> None:
    global gpd, gdal, ogr
    if gpd is not None and gdal is not None and ogr is not None:
        return
    try:
        import geopandas as _gpd
        from osgeo import gdal as _gdal
        from osgeo import ogr as _ogr
    except Exception as exc:
        raise ImportError(
            "geopandas and GDAL Python bindings are required for raster extraction. "
            "On the server, use the same Python environment that can run smooth_terrain.py."
        ) from exc
    gpd = _gpd
    gdal = _gdal
    ogr = _ogr


SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
PROJECT_ROOT_CANDIDATES = [SCRIPT_DIR.parent, SCRIPT_DIR.parent.parent]
for candidate in PROJECT_ROOT_CANDIDATES:
    if str(candidate) not in sys.path:
        sys.path.append(str(candidate))

try:
    from constants import DATA_DIR, DEFAULT_CITY  # type: ignore
except Exception:
    DATA_DIR = pathlib.Path("/home/pixel/geo_tools/data")
    DEFAULT_CITY = "Beijing"


DEFAULT_BLOCK_FILENAMES = [
    "classify_streetId_block.gpkg",
    "classify_street_block.gpkg",
    "city_street_block.gpkg",
]
DEFAULT_BLOCK_FILENAME = "city_street_block.gpkg"
DEFAULT_BUILDING_FILENAMES = [
    # 增量版本包含 building、man_made；优先使用，避免只读到 landuse/amenity。
    "priority_blocks_incremental_man_made.gpkg",
    "priority_blocks_copy.gpkg",
    "priority_blocks.gpkg",
]
DEFAULT_BUILDING_FILENAME = "priority_blocks_incremental_man_made.gpkg"
DEFAULT_BLOCK_LAYER: str | None = None
DEFAULT_BUILDING_LAYER: str | None = "priority_blocks"
DEFAULT_BLOCK_ID_COLUMN: str | None = None
DEFAULT_TARGET_CRS: str | None = None
DEFAULT_PIXEL_SIZE = 2.0
DEFAULT_TILE_SIZE = 4096
DEFAULT_ALL_TOUCHED = False
DEFAULT_VALUE_COLUMNS = ["value", "building", "amenity", "landuse", "leisure", "tourism", "man_made"]
DEFAULT_CATEGORY_COLUMNS = ["source_category", "category", "tag_key", "key"]
DEFAULT_ALLOWED_CATEGORIES = ["building", "amenity", "landuse", "leisure", "tourism", "man_made"]


@dataclass
class RasterGrid:
    xmin: float
    ymin: float
    xmax: float
    ymax: float
    pixel_size: float
    width: int
    height: int
    projection_wkt: str


@dataclass(frozen=True)
class RuntimeConfig:
    city: str
    city_dir: pathlib.Path
    block_path: pathlib.Path
    building_path: pathlib.Path
    output_dir: pathlib.Path
    block_layer: str | None
    building_layer: str | None
    block_id_column: str | None
    target_crs: str | None
    pixel_size: float
    tile_size: int
    all_touched: bool


def resolve_runtime_config(
    city: str,
    blocks: pathlib.Path | None = None,
    buildings: pathlib.Path | None = None,
    output_dir: pathlib.Path | None = None,
    block_layer: str | None = DEFAULT_BLOCK_LAYER,
    building_layer: str | None = DEFAULT_BUILDING_LAYER,
    block_id_column: str | None = DEFAULT_BLOCK_ID_COLUMN,
    target_crs: str | None = DEFAULT_TARGET_CRS,
    pixel_size: float = DEFAULT_PIXEL_SIZE,
    tile_size: int = DEFAULT_TILE_SIZE,
    all_touched: bool = DEFAULT_ALL_TOUCHED,
) -> RuntimeConfig:
    city_dir = pathlib.Path(DATA_DIR) / "city" / str(city)
    default_block_path = next(
        (city_dir / filename for filename in DEFAULT_BLOCK_FILENAMES if (city_dir / filename).exists()),
        city_dir / DEFAULT_BLOCK_FILENAME,
    )
    default_building_path = next(
        (city_dir / filename for filename in DEFAULT_BUILDING_FILENAMES if (city_dir / filename).exists()),
        city_dir / DEFAULT_BUILDING_FILENAME,
    )
    return RuntimeConfig(
        city=str(city),
        city_dir=city_dir,
        block_path=blocks or default_block_path,
        building_path=buildings or default_building_path,
        output_dir=output_dir or city_dir,
        block_layer=block_layer,
        building_layer=building_layer,
        block_id_column=block_id_column,
        target_crs=target_crs,
        pixel_size=float(pixel_size),
        tile_size=int(tile_size),
        all_touched=bool(all_touched),
    )


def sanitize_value_name(value: str) -> str:
    text = str(value).strip().lower()
    text = re.sub(r"[^0-9a-zA-Z_]+", "_", text)
    text = re.sub(r"_+", "_", text).strip("_")
    return text or "unknown"


def split_csv(text: str | None) -> list[str]:
    if text is None:
        return []
    return [part.strip() for part in str(text).split(",") if part.strip()]


def list_gpkg_layers(path: pathlib.Path) -> list[str]:
    ensure_gis_imports()
    ds = ogr.Open(str(path))
    if ds is None:
        return []
    try:
        return [ds.GetLayerByIndex(i).GetName() for i in range(ds.GetLayerCount())]
    finally:
        ds = None


def choose_layer(path: pathlib.Path, requested: str | None, preferred: list[str]) -> str | None:
    if requested:
        return requested
    layers = list_gpkg_layers(path)
    if not layers:
        return None
    for name in preferred:
        if name in layers:
            return name
    return layers[0]


def read_layer(path: pathlib.Path, layer: str | None) -> gpd.GeoDataFrame:
    ensure_gis_imports()
    if layer:
        return gpd.read_file(path, layer=layer)
    return gpd.read_file(path)


def choose_projected_crs(blocks: gpd.GeoDataFrame, buildings: gpd.GeoDataFrame, target_crs: str | None) -> Any:
    if target_crs:
        return target_crs
    if blocks.crs is not None and not blocks.crs.is_geographic:
        return blocks.crs
    if buildings.crs is not None and not buildings.crs.is_geographic:
        return buildings.crs
    try:
        return blocks.estimate_utm_crs()
    except Exception:
        return "EPSG:3857"


def make_valid_polygons(gdf: gpd.GeoDataFrame) -> gpd.GeoDataFrame:
    work = gdf.copy()
    work = work[work.geometry.notna()].copy()
    work["geometry"] = work.geometry.make_valid()
    work = work[~work.geometry.is_empty].copy()
    work = work[work.geometry.geom_type.isin(["Polygon", "MultiPolygon"])].copy()
    return work.reset_index(drop=True)


def first_existing_column(gdf: gpd.GeoDataFrame, candidates: list[str]) -> str | None:
    lower_to_actual = {str(col).lower(): str(col) for col in gdf.columns}
    for candidate in candidates:
        actual = lower_to_actual.get(candidate.lower())
        if actual is not None:
            return actual
    return None


def build_tag_value(row: pd.Series, value_columns: list[str]) -> str:
    for column in value_columns:
        if column not in row.index:
            continue
        value = row.get(column)
        if value is None or pd.isna(value):
            continue
        text = str(value).strip()
        if text and text.lower() not in {"nan", "none", "null", "no"}:
            return text
    return "unknown"


def is_meaningful_tag_value(value: Any) -> bool:
    if value is None or pd.isna(value):
        return False
    text = str(value).strip()
    return bool(text) and text.lower() not in {"nan", "none", "null", "no", "false", "0"}


def collect_tag_evidence(
    row: pd.Series,
    value_columns: list[str],
    category_columns: list[str],
    allowed_categories: set[str],
) -> list[tuple[str, str]]:
    allowed_lower = {str(category).strip().lower() for category in allowed_categories}
    category_col = next((column for column in category_columns if column in row.index), None)
    generic_category = ""
    if category_col and is_meaningful_tag_value(row.get(category_col)):
        generic_category = str(row.get(category_col)).strip().lower()

    evidence: list[tuple[str, str]] = []
    seen: set[tuple[str, str]] = set()

    if generic_category in allowed_lower and "value" in row.index and is_meaningful_tag_value(row.get("value")):
        raw_value = str(row.get("value")).strip()
        key = (generic_category, sanitize_value_name(raw_value))
        evidence.append((generic_category, raw_value))
        seen.add(key)

    for column in value_columns:
        column_category = str(column).strip().lower()
        if column_category == "value" or column_category not in allowed_lower:
            continue
        if column not in row.index or not is_meaningful_tag_value(row.get(column)):
            continue
        raw_value = str(row.get(column)).strip()
        key = (column_category, sanitize_value_name(raw_value))
        if key in seen:
            continue
        evidence.append((column_category, raw_value))
        seen.add(key)

    return evidence


def filter_building_evidence(
    gdf: gpd.GeoDataFrame,
    value_columns: list[str],
    category_columns: list[str],
    allowed_categories: set[str],
) -> gpd.GeoDataFrame:
    work = gdf.copy()
    expanded_rows = []
    for _, row in work.iterrows():
        for source_category, raw_value in collect_tag_evidence(row, value_columns, category_columns, allowed_categories):
            new_row = row.copy()
            safe_category = sanitize_value_name(source_category)
            safe_value = sanitize_value_name(raw_value)
            new_row["_source_category"] = safe_category
            new_row["_raw_tag_value"] = safe_value
            # category 只用于内部计算；最终输出按原始 value 合并，避免列名出现
            # building/amenity/landuse 等分类前缀。
            new_row["_tag_value"] = safe_value
            new_row["_tag_key"] = f"{safe_category}__{safe_value}"
            expanded_rows.append(new_row)

    if not expanded_rows:
        return work.iloc[0:0].copy()

    expanded = pd.DataFrame(expanded_rows)
    if gpd is not None and hasattr(gpd, "GeoDataFrame") and "geometry" in expanded.columns:
        geometry_name = getattr(getattr(work, "geometry", None), "name", "geometry")
        expanded = gpd.GeoDataFrame(expanded, geometry=geometry_name, crs=getattr(work, "crs", None))
    return expanded.reset_index(drop=True)


def prepare_inputs(
    block_path: pathlib.Path,
    building_path: pathlib.Path,
    block_layer: str | None,
    building_layer: str | None,
    block_id_column: str | None,
    value_columns: list[str],
    category_columns: list[str],
    allowed_categories: set[str],
    target_crs: str | None,
) -> tuple[
    gpd.GeoDataFrame,
    gpd.GeoDataFrame,
    dict[int, str],
    dict[int, tuple[str, str]],
    dict[int, str],
]:
    blocks = read_layer(block_path, block_layer)
    buildings = read_layer(building_path, building_layer)
    if blocks.empty:
        raise ValueError(f"No block features found: {block_path}")
    if buildings.empty:
        raise ValueError(f"No building/tag features found: {building_path}")

    crs = choose_projected_crs(blocks, buildings, target_crs)
    blocks = make_valid_polygons(blocks.to_crs(crs))
    buildings = make_valid_polygons(buildings.to_crs(crs))
    buildings = filter_building_evidence(buildings, value_columns, category_columns, allowed_categories)
    if buildings.empty:
        raise ValueError("No usable building/tag polygons after filtering and value extraction.")

    if block_id_column and block_id_column in blocks.columns:
        blocks["_block_id"] = blocks[block_id_column].astype(str)
    elif "_block_id" in blocks.columns:
        blocks["_block_id"] = blocks["_block_id"].astype(str)
    elif "block_id" in blocks.columns:
        blocks["_block_id"] = blocks["block_id"].astype(str)
    else:
        blocks["_block_id"] = blocks.index.astype(str)

    if "sector_id" not in blocks.columns:
        blocks["sector_id"] = blocks["_block_id"]
    else:
        blocks["sector_id"] = blocks["sector_id"].astype(str)

    blocks["_burn_block_id"] = np.arange(1, len(blocks) + 1, dtype=np.int32)
    category_values = sorted(buildings["_source_category"].dropna().astype(str).unique().tolist())
    category_to_id = {category: idx + 1 for idx, category in enumerate(category_values)}
    buildings["_category_id"] = buildings["_source_category"].map(category_to_id).astype(np.int32)

    # 同一个 value 在不同 category 中必须分别统计；否则同名标签会在栅格化时
    # 共用同一个像元编号，无法计算各类别内部比例。
    tag_keys = sorted(buildings["_tag_key"].dropna().astype(str).unique().tolist())
    tag_to_id = {tag: idx + 1 for idx, tag in enumerate(tag_keys)}
    buildings["_burn_tag_id"] = buildings["_tag_key"].map(tag_to_id).astype(np.int32)

    block_id_by_burn = dict(zip(blocks["_burn_block_id"].astype(int), blocks["_block_id"].astype(str)))
    tag_by_burn = {
        idx: (
            str(buildings.loc[buildings["_tag_key"] == tag, "_source_category"].iloc[0]),
            str(buildings.loc[buildings["_tag_key"] == tag, "_tag_value"].iloc[0]),
        )
        for tag, idx in tag_to_id.items()
    }
    category_by_id = {idx: category for category, idx in category_to_id.items()}
    return blocks, buildings, block_id_by_burn, tag_by_burn, category_by_id


def write_temp_gpkg(blocks: gpd.GeoDataFrame, buildings: gpd.GeoDataFrame, temp_dir: pathlib.Path) -> tuple[pathlib.Path, pathlib.Path]:
    block_path = temp_dir / "blocks_for_raster.gpkg"
    building_path = temp_dir / "buildings_for_raster.gpkg"

    block_cols = ["_block_id", "sector_id", "_burn_block_id", "geometry"]
    building_cols = ["_tag_value", "_tag_key", "_source_category", "_category_id", "_burn_tag_id", "geometry"]
    blocks[block_cols].to_file(block_path, layer="blocks", driver="GPKG")
    buildings[building_cols].to_file(building_path, layer="buildings", driver="GPKG")
    return block_path, building_path


def build_grid(bounds: tuple[float, float, float, float], pixel_size: float, projection_wkt: str) -> RasterGrid:
    xmin, ymin, xmax, ymax = bounds
    width = int(math.ceil((xmax - xmin) / pixel_size))
    height = int(math.ceil((ymax - ymin) / pixel_size))
    xmax = xmin + width * pixel_size
    ymin = ymax - height * pixel_size
    return RasterGrid(xmin=xmin, ymin=ymin, xmax=xmax, ymax=ymax, pixel_size=pixel_size, width=width, height=height, projection_wkt=projection_wkt)


def tile_windows(grid: RasterGrid, tile_size: int):
    for yoff in range(0, grid.height, tile_size):
        rows = min(tile_size, grid.height - yoff)
        tile_ymax = grid.ymax - yoff * grid.pixel_size
        tile_ymin = tile_ymax - rows * grid.pixel_size
        for xoff in range(0, grid.width, tile_size):
            cols = min(tile_size, grid.width - xoff)
            tile_xmin = grid.xmin + xoff * grid.pixel_size
            tile_xmax = tile_xmin + cols * grid.pixel_size
            yield xoff, yoff, cols, rows, tile_xmin, tile_ymin, tile_xmax, tile_ymax


def create_mem_raster(cols: int, rows: int, xmin: float, ymax: float, pixel_size: float, projection_wkt: str, dtype: int):
    ensure_gis_imports()
    ds = gdal.GetDriverByName("MEM").Create("", cols, rows, 1, dtype)
    ds.SetGeoTransform((xmin, pixel_size, 0.0, ymax, 0.0, -pixel_size))
    ds.SetProjection(projection_wkt)
    band = ds.GetRasterBand(1)
    band.Fill(0)
    band.SetNoDataValue(0)
    return ds


def rasterize_layer_to_array(
    layer: ogr.Layer,
    cols: int,
    rows: int,
    xmin: float,
    ymin: float,
    xmax: float,
    ymax: float,
    pixel_size: float,
    projection_wkt: str,
    attribute: str,
    all_touched: bool,
) -> np.ndarray:
    layer.SetSpatialFilterRect(xmin, ymin, xmax, ymax)
    ds = create_mem_raster(cols, rows, xmin, ymax, pixel_size, projection_wkt, gdal.GDT_Int32)
    options = [f"ATTRIBUTE={attribute}"]
    if all_touched:
        options.append("ALL_TOUCHED=TRUE")
    gdal.RasterizeLayer(ds, [1], layer, options=options)
    band = ds.GetRasterBand(1)
    # Use ReadRaster instead of ReadAsArray: some server GDAL installs lack osgeo._gdal_array.
    data = band.ReadRaster(0, 0, cols, rows, buf_type=gdal.GDT_Int32)
    array = np.frombuffer(data, dtype=np.int32).reshape(rows, cols)
    ds = None
    layer.SetSpatialFilter(None)
    return array


def count_block_tag_pixels(
    block_path: pathlib.Path,
    building_path: pathlib.Path,
    blocks: gpd.GeoDataFrame,
    grid: RasterGrid,
    tile_size: int,
    all_touched: bool,
    category_by_id: dict[int, str],
    verbose: bool,
) -> tuple[dict[tuple[int, int], int], dict[int, int], dict[int, int]]:
    ensure_gis_imports()
    block_ds = ogr.Open(str(block_path))
    building_ds = ogr.Open(str(building_path))
    if block_ds is None or building_ds is None:
        raise RuntimeError("Failed to open temporary GPKG layers for GDAL rasterization.")
    block_layer = block_ds.GetLayerByName("blocks")
    building_layer = building_ds.GetLayerByName("buildings")

    pair_counts: dict[tuple[int, int], int] = {}
    block_pixel_counts: dict[int, int] = {}
    tagged_pixel_counts: dict[int, int] = {}
    total_tiles = math.ceil(grid.width / tile_size) * math.ceil(grid.height / tile_size)

    try:
        for tile_index, (_xoff, _yoff, cols, rows, xmin, ymin, xmax, ymax) in enumerate(tile_windows(grid, tile_size), start=1):
            if verbose and (tile_index == 1 or tile_index % 25 == 0 or tile_index == total_tiles):
                print(f"tile {tile_index}/{total_tiles}: cols={cols}, rows={rows}")

            block_arr = rasterize_layer_to_array(
                block_layer, cols, rows, xmin, ymin, xmax, ymax, grid.pixel_size, grid.projection_wkt, "_burn_block_id", all_touched
            )
            block_mask = block_arr > 0
            if not block_mask.any():
                continue

            block_ids, block_counts = np.unique(block_arr[block_mask], return_counts=True)
            for block_id, count in zip(block_ids.astype(int), block_counts.astype(int)):
                block_pixel_counts[block_id] = block_pixel_counts.get(block_id, 0) + int(count)

            # 不同 category 不能共用一个栅格：同一像元可能同时属于 amenity、
            # landuse、leisure 等类别。逐类别栅格化后，每个类别内部独立统计，
            # 才能得到“value 在 category 内的比例”。
            tile_tagged_mask = np.zeros(block_arr.shape, dtype=bool)
            for category_id in sorted(category_by_id):
                building_layer.SetAttributeFilter(f"_category_id = {int(category_id)}")
                try:
                    building_arr = rasterize_layer_to_array(
                        building_layer,
                        cols,
                        rows,
                        xmin,
                        ymin,
                        xmax,
                        ymax,
                        grid.pixel_size,
                        grid.projection_wkt,
                        "_burn_tag_id",
                        all_touched,
                    )
                finally:
                    building_layer.SetAttributeFilter(None)

                valid = block_mask & (building_arr > 0)
                if not valid.any():
                    continue
                tile_tagged_mask |= valid

                # 编码 block/tag 对，避免 Python 逐像元循环；1_000_000 大于
                # 单个临时栅格中的 block/tag 编号上限，保证可逆解码。
                combined = block_arr[valid].astype(np.int64) * 1_000_000 + building_arr[valid].astype(np.int64)
                unique, counts = np.unique(combined, return_counts=True)
                for encoded, count in zip(unique.astype(np.int64), counts.astype(int)):
                    block_id = int(encoded // 1_000_000)
                    tag_id = int(encoded % 1_000_000)
                    key = (block_id, tag_id)
                    pair_counts[key] = pair_counts.get(key, 0) + int(count)

            if tile_tagged_mask.any():
                tagged_ids, tagged_counts = np.unique(block_arr[tile_tagged_mask], return_counts=True)
                for block_id, count in zip(tagged_ids.astype(int), tagged_counts.astype(int)):
                    tagged_pixel_counts[block_id] = tagged_pixel_counts.get(block_id, 0) + int(count)
    finally:
        block_ds = None
        building_ds = None

    return pair_counts, block_pixel_counts, tagged_pixel_counts


def add_building_share_columns(wide_df: pd.DataFrame) -> pd.DataFrame:
    val_cols = [col for col in wide_df.columns if col.startswith("val_pct_")]
    if not val_cols:
        return wide_df.copy()

    total = pd.to_numeric(wide_df["building_pct_total"], errors="coerce").fillna(0.0)
    denominator = total.replace(0.0, np.nan)
    value_df = wide_df[val_cols].apply(pd.to_numeric, errors="coerce").fillna(0.0)
    share_df = value_df.div(denominator, axis=0).mul(100.0).fillna(0.0).round(8)
    share_df.columns = [col.replace("val_pct_", "building_share_", 1) for col in share_df.columns]
    return pd.concat([wide_df.reset_index(drop=True), share_df.reset_index(drop=True)], axis=1).copy()


def split_feature_matrix_tables(wide_df: pd.DataFrame) -> tuple[pd.DataFrame, pd.DataFrame]:
    val_cols = [col for col in wide_df.columns if col.startswith("val_pct_")]
    share_cols = [col for col in wide_df.columns if col.startswith("building_share_")]
    meta_cols = [
        col
        for col in wide_df.columns
        if not col.startswith("val_pct_") and not col.startswith("building_share_") and col != "building_pct_total"
    ]
    total_cols = ["building_pct_total"] if "building_pct_total" in wide_df.columns else []

    val_pct_df = wide_df[meta_cols + val_cols + total_cols].copy()
    building_share_df = wide_df[meta_cols + total_cols + share_cols].copy()
    return val_pct_df, building_share_df


def compute_value_composition(
    pixel_count: int,
    category_total_pixels: int,
    block_total_pixels: int,
    category_pct: float | None,
) -> tuple[float, float]:
    """计算单个 value 的类别内比例与最终组成比例。

    有分类地块时：

    ``ratio_in_category = value_pixels / category_value_pixels``

    ``val_pct = ratio_in_category * cat_pct``

    没有 ``cat_pct_*`` 时，退化为所有标签像元中的组成比例。
    """
    ratio_in_category = (
        float(pixel_count) / float(category_total_pixels)
        if category_total_pixels > 0
        else 0.0
    )
    if category_pct is None:
        val_pct = (
            float(pixel_count) / float(block_total_pixels) * 100.0
            if block_total_pixels > 0
            else 0.0
        )
    else:
        val_pct = ratio_in_category * float(category_pct)
    return ratio_in_category, val_pct


def resolve_effective_category_weights(
    category_pixel_totals: dict[str, int],
    raw_category_weights: dict[str, float | None],
) -> dict[str, float]:
    """为一个 block 计算实际参与组成分配的类别权重。

    数学处理：

    1. 只保留当前 block 中确实存在栅格像元的类别，避免把没有标签的类别
       当成已观测类别。
    2. 如果每个已观测类别都有有效 ``cat_pct_*``，使用

       ``w_c = raw_w_c / sum(raw_w_j) * 100``。

       这样缺少某一类数据时，其余已观测类别仍能构成完整的 100% 组成向量。
    3. 如果类别权重字段缺失或不完整，使用类别像元数近似类别权重：

       ``w_c = pixels_c / sum(pixels_j) * 100``。

    返回值总和为 100；空 block 返回空字典。
    """
    observed = {
        str(category): int(pixel_count)
        for category, pixel_count in category_pixel_totals.items()
        if int(pixel_count) > 0
    }
    if not observed:
        return {}

    raw_values: dict[str, float] = {}
    for category in observed:
        raw_value = raw_category_weights.get(category)
        numeric = pd.to_numeric(raw_value, errors="coerce")
        if pd.notna(numeric) and float(numeric) >= 0.0:
            raw_values[category] = float(numeric)

    raw_is_complete = len(raw_values) == len(observed)
    raw_total = sum(raw_values.values())
    if raw_is_complete and raw_total > 0.0:
        return {
            category: raw_values[category] / raw_total * 100.0
            for category in observed
        }

    pixel_total = sum(observed.values())
    if pixel_total <= 0:
        return {}
    return {
        category: pixel_count / pixel_total * 100.0
        for category, pixel_count in observed.items()
    }


def build_output_tables(
    blocks: gpd.GeoDataFrame,
    pair_counts: dict[tuple[int, int], int],
    block_pixel_counts: dict[int, int],
    tagged_pixel_counts: dict[int, int],
    block_id_by_burn: dict[int, str],
    tag_by_burn: dict[int, tuple[str, str]],
    pixel_area: float,
) -> tuple[pd.DataFrame, pd.DataFrame]:
    category_columns = {
        sanitize_value_name(str(column)[len("cat_pct_") :]): column
        for column in blocks.columns
        if str(column).startswith("cat_pct_")
    }
    block_meta_columns = ["_burn_block_id", "_block_id", "sector_id", "geometry"] + list(category_columns.values())
    block_meta = blocks[block_meta_columns].copy()
    block_meta["block_area_m2"] = block_meta.geometry.area.astype(float)
    block_meta["block_pixel_area_m2"] = block_meta["_burn_block_id"].map(lambda i: block_pixel_counts.get(int(i), 0) * pixel_area)
    block_meta["tagged_area_m2"] = block_meta["_burn_block_id"].map(lambda i: tagged_pixel_counts.get(int(i), 0) * pixel_area)
    # 标签覆盖率使用同一栅格中的 block 像元作为分母，避免栅格近似面积与
    # 矢量几何面积不同导致比例出现 100% 以上。
    block_meta["tagged_area_pct"] = np.where(
        block_meta["block_pixel_area_m2"] > 0,
        block_meta["tagged_area_m2"] / block_meta["block_pixel_area_m2"] * 100.0,
        0.0,
    )
    block_meta = block_meta.drop(columns=["geometry"])

    block_meta_by_id = block_meta.set_index("_burn_block_id", drop=False)
    # 类别内部比例的分母必须是该 block/category 下全部标签像元数，
    # 不能直接使用 block 面积。否则得到的是覆盖率，不是组成比例。
    category_pixel_totals: dict[tuple[int, str], int] = {}
    block_pixel_totals: dict[int, int] = {}
    for (burn_block_id, burn_tag_id), pixel_count in pair_counts.items():
        tag_info = tag_by_burn.get(burn_tag_id)
        if tag_info is None:
            continue
        source_category, _tag_value = tag_info
        category_key = (int(burn_block_id), source_category)
        category_pixel_totals[category_key] = category_pixel_totals.get(category_key, 0) + int(pixel_count)
        block_pixel_totals[burn_block_id] = block_pixel_totals.get(burn_block_id, 0) + int(pixel_count)

    long_rows: list[dict[str, Any]] = []
    effective_category_weights_by_block: dict[int, dict[str, float]] = {}
    raw_category_weights_by_block: dict[int, dict[str, float | None]] = {}
    category_totals_by_block: dict[int, dict[str, int]] = {}
    for (burn_block_id, source_category), pixel_count in category_pixel_totals.items():
        category_totals_by_block.setdefault(int(burn_block_id), {})[source_category] = int(pixel_count)

    for burn_block_id, block_category_totals in category_totals_by_block.items():
        block_row = block_meta_by_id.loc[burn_block_id]
        raw_weights: dict[str, float | None] = {}
        for source_category in block_category_totals:
            category_column = category_columns.get(source_category)
            raw_value = block_row.get(category_column) if category_column is not None else None
            numeric = pd.to_numeric(raw_value, errors="coerce")
            raw_weights[source_category] = float(numeric) if pd.notna(numeric) else None
        raw_category_weights_by_block[burn_block_id] = raw_weights
        effective_category_weights_by_block[burn_block_id] = resolve_effective_category_weights(
            block_category_totals,
            raw_weights,
        )

    for (burn_block_id, burn_tag_id), pixel_count in pair_counts.items():
        block_id = block_id_by_burn.get(burn_block_id)
        tag_info = tag_by_burn.get(burn_tag_id)
        if block_id is None or tag_info is None:
            continue
        source_category, tag_value = tag_info
        block_row = block_meta_by_id.loc[burn_block_id]
        block_area = float(block_row["block_area_m2"])
        area_m2 = float(pixel_count * pixel_area)

        category_total_pixels = category_pixel_totals.get((burn_block_id, source_category), 0)
        block_total_pixels = block_pixel_totals.get(burn_block_id, 0)
        raw_category_pct = raw_category_weights_by_block.get(burn_block_id, {}).get(source_category)
        effective_category_pct = effective_category_weights_by_block.get(burn_block_id, {}).get(source_category)
        if effective_category_pct is not None:
            # val_pct = ratio_in_category * effective_category_pct。
            # effective_category_pct 已在当前 block 的已观测类别内归一化为百分数。
            ratio_in_category, val_pct = compute_value_composition(
                pixel_count,
                category_total_pixels,
                block_total_pixels,
                effective_category_pct,
            )
        else:
            # 没有可用类别权重时，退化为所有标签像元中的组成比例。
            ratio_in_category, val_pct = compute_value_composition(
                pixel_count,
                category_total_pixels,
                block_total_pixels,
                None,
            )

        tagged_pct = float(block_row["tagged_area_pct"])
        long_rows.append(
            {
                "_burn_block_id": burn_block_id,
                "_block_id": block_id,
                "source_category": source_category,
                "tag_value": tag_value,
                "value_col": sanitize_value_name(tag_value),
                "pixel_count": int(pixel_count),
                "building_area_m2": round(area_m2, 3),
                "block_area_m2": round(block_area, 3),
                "category_pixel_count": int(category_total_pixels),
                "category_pct": round(float(raw_category_pct), 8) if raw_category_pct is not None else 0.0,
                "effective_category_pct": round(float(effective_category_pct), 8) if effective_category_pct is not None else 0.0,
                "ratio_in_category": round(ratio_in_category, 8),
                "building_pct": round(val_pct, 8),
                "tagged_area_pct": round(tagged_pct, 8),
            }
        )

    long_df = pd.DataFrame(long_rows)
    if long_df.empty:
        long_df = pd.DataFrame(
            columns=[
                "_burn_block_id",
                "_block_id",
                "source_category",
                "tag_value",
                "value_col",
                "pixel_count",
                "building_area_m2",
                "block_area_m2",
                "category_pixel_count",
                "category_pct",
                "effective_category_pct",
                "ratio_in_category",
                "building_pct",
                "tagged_area_pct",
            ]
        )

    final_meta = block_meta.drop(columns=list(category_columns.values()), errors="ignore")
    long_df = long_df.merge(
        final_meta.drop(columns=["block_area_m2", "tagged_area_pct"]),
        on=["_burn_block_id", "_block_id"],
        how="left",
    )
    long_df = long_df.sort_values(["_burn_block_id", "building_pct", "tag_value"], ascending=[True, False, True]).reset_index(drop=True)

    pivot = long_df.pivot_table(
        index=["_block_id"],
        columns="value_col",
        values="building_pct",
        aggfunc="sum",
        fill_value=0.0,
    )
    pivot.columns = [f"val_pct_{col}" for col in pivot.columns]
    pivot = pivot.reset_index()

    wide_meta = final_meta.drop(columns=["_burn_block_id"]).copy()
    wide_meta["block_area_m2"] = wide_meta["block_area_m2"].round(3)
    wide_meta["block_pixel_area_m2"] = wide_meta["block_pixel_area_m2"].round(3)
    wide_meta["tagged_area_m2"] = wide_meta["tagged_area_m2"].round(3)
    wide_meta["tagged_area_pct"] = wide_meta["tagged_area_pct"].round(8)

    wide_joined = wide_meta[["_block_id"]].merge(pivot, on="_block_id", how="left")
    val_cols = [col for col in wide_joined.columns if col.startswith("val_pct_")]
    if val_cols:
        value_df = wide_joined[val_cols].fillna(0.0).astype(float)
        # 最后再按行校正一次，消除浮点误差，并保证非空 block 的组成向量
        # 严格落在 100% 附近。实际标签几何覆盖率由 tagged_area_pct 表示。
        raw_total = value_df.sum(axis=1)
        nonzero = raw_total > 0.0
        value_df.loc[nonzero] = value_df.loc[nonzero].div(raw_total[nonzero], axis=0).mul(100.0)
        value_df = value_df.round(8)
        total = value_df.sum(axis=1).round(8).rename("building_pct_total")
        wide_df = pd.concat([wide_meta.reset_index(drop=True), value_df.reset_index(drop=True), total.reset_index(drop=True)], axis=1).copy()
    else:
        wide_df = pd.concat([wide_meta.reset_index(drop=True), pd.Series(0.0, name="building_pct_total")], axis=1).copy()

    wide_df = add_building_share_columns(wide_df)
    return long_df, wide_df


def extract_block_building_pct(
    block_path: pathlib.Path,
    building_path: pathlib.Path,
    output_dir: pathlib.Path,
    block_layer: str | None,
    building_layer: str | None,
    block_id_column: str | None,
    value_columns: list[str],
    category_columns: list[str],
    allowed_categories: set[str],
    target_crs: str | None,
    pixel_size: float,
    tile_size: int,
    all_touched: bool,
    verbose: bool,
) -> dict[str, Any]:
    block_layer = choose_layer(
        block_path,
        block_layer,
        ["classify_streetId_block", "classify_street_block", "blocks", "street_blocks", "city_street_block", "block", "DEFAULT"],
    )
    building_layer = choose_layer(building_path, building_layer, ["priority_blocks", "buildings", "building", "DEFAULT"])

    print(f"blocks: {block_path} layer={block_layer or 'DEFAULT'}")
    print(f"buildings: {building_path} layer={building_layer or 'DEFAULT'}")
    blocks, buildings, block_id_by_burn, tag_by_burn, category_by_id = prepare_inputs(
        block_path=block_path,
        building_path=building_path,
        block_layer=block_layer,
        building_layer=building_layer,
        block_id_column=block_id_column,
        value_columns=value_columns,
        category_columns=category_columns,
        allowed_categories=allowed_categories,
        target_crs=target_crs,
    )

    bounds = tuple(blocks.total_bounds)
    grid = build_grid(bounds, pixel_size, blocks.crs.to_wkt())
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"prepared blocks={len(blocks)}, building/tag polygons={len(buildings)}, tags={len(tag_by_burn)}")
    print(f"grid width={grid.width}, height={grid.height}, pixel_size={pixel_size}, tile_size={tile_size}")

    with tempfile.TemporaryDirectory(prefix="block_building_pct_") as temp_name:
        temp_dir = pathlib.Path(temp_name)
        temp_block_path, temp_building_path = write_temp_gpkg(blocks, buildings, temp_dir)
        pair_counts, block_pixel_counts, tagged_pixel_counts = count_block_tag_pixels(
            block_path=temp_block_path,
            building_path=temp_building_path,
            blocks=blocks,
            grid=grid,
            tile_size=tile_size,
            all_touched=all_touched,
            category_by_id=category_by_id,
            verbose=verbose,
        )

    long_df, wide_df = build_output_tables(
        blocks=blocks,
        pair_counts=pair_counts,
        block_pixel_counts=block_pixel_counts,
        tagged_pixel_counts=tagged_pixel_counts,
        block_id_by_burn=block_id_by_burn,
        tag_by_burn=tag_by_burn,
        pixel_area=pixel_size * pixel_size,
    )

    val_pct_df, building_share_df = split_feature_matrix_tables(wide_df)

    long_path = output_dir / "block_building_value_features_long.csv"
    val_pct_path = output_dir / "block_building_feature_matrix_val_pct.csv"
    building_share_path = output_dir / "block_building_feature_matrix_building_share.csv"
    summary_path = output_dir / "block_building_feature_summary.json"
    xlsx_path = output_dir / "block_building_feature_matrix.xlsx"

    long_df.to_csv(long_path, index=False, encoding="utf-8-sig")
    val_pct_df.to_csv(val_pct_path, index=False, encoding="utf-8-sig")
    building_share_df.to_csv(building_share_path, index=False, encoding="utf-8-sig")
    try:
        with pd.ExcelWriter(xlsx_path, engine="openpyxl") as writer:
            val_pct_df.to_excel(writer, sheet_name="val_pct_matrix", index=False)
            building_share_df.to_excel(writer, sheet_name="building_share_matrix", index=False)
            long_df.to_excel(writer, sheet_name="block_building_long", index=False)
    except Exception as exc:
        print(f"skip xlsx export: {exc}")
        xlsx_path = pathlib.Path("")

    summary = {
        "blocks": int(len(blocks)),
        "building_polygons": int(len(buildings)),
        "tags": int(len(tag_by_burn)),
        "categories": int(len(category_by_id)),
        "uses_category_weights": bool(any(str(column).startswith("cat_pct_") for column in blocks.columns)),
        "nonzero_block_tag_pairs": int(len(long_df)),
        "blocks_with_building_pct": int((wide_df["building_pct_total"] > 0).sum()),
        "zero_building_blocks": int((wide_df["building_pct_total"] <= 0).sum()),
        "building_pct_total_median": float(pd.to_numeric(wide_df["building_pct_total"], errors="coerce").fillna(0.0).median()),
        "tagged_area_pct_median": float(pd.to_numeric(wide_df["tagged_area_pct"], errors="coerce").fillna(0.0).median()),
        "building_pct_total_definition": "observed tag composition sum; non-empty blocks normalized to 100",
        "tagged_area_pct_definition": "union raster coverage of tag geometries inside block",
        "pixel_size": float(pixel_size),
        "tile_size": int(tile_size),
        "all_touched": bool(all_touched),
        "long_csv": str(long_path),
        "val_pct_csv": str(val_pct_path),
        "building_share_csv": str(building_share_path),
        "xlsx": str(xlsx_path) if str(xlsx_path) not in {"", "."} else "",
    }
    summary_path.write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    return summary


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Extract per-block building/tag area percentages with GDAL tiled rasterization."
    )
    parser.add_argument("city", nargs="?", default=DEFAULT_CITY, help=f"city name under DATA_DIR/city, default={DEFAULT_CITY}")
    parser.add_argument("--blocks", type=pathlib.Path, default=None, help="block polygon GPKG/SHP/GeoJSON path")
    parser.add_argument("--buildings", type=pathlib.Path, default=None, help="priority/building polygon GPKG/SHP/GeoJSON path")
    parser.add_argument("--block-layer", default=DEFAULT_BLOCK_LAYER, help="block layer name; auto-detect when omitted")
    parser.add_argument("--building-layer", default=DEFAULT_BUILDING_LAYER, help="building/priority layer name")
    parser.add_argument("--block-id-column", default=DEFAULT_BLOCK_ID_COLUMN, help="existing block id column; default _block_id/block_id/index")
    parser.add_argument("--value-columns", default=",".join(DEFAULT_VALUE_COLUMNS), help="columns used to choose tag value")
    parser.add_argument("--category-columns", default=",".join(DEFAULT_CATEGORY_COLUMNS), help="columns used to filter OSM key/category")
    parser.add_argument("--allowed-categories", default=",".join(DEFAULT_ALLOWED_CATEGORIES), help="category values to keep if category column exists")
    parser.add_argument("--target-crs", default=DEFAULT_TARGET_CRS, help="projected CRS, e.g. EPSG:3857 or EPSG:32650; auto when omitted")
    parser.add_argument("--pixel-size", type=float, default=DEFAULT_PIXEL_SIZE, help="raster pixel size in projected CRS units, usually meters")
    parser.add_argument("--tile-size", type=int, default=DEFAULT_TILE_SIZE, help="tile width/height in pixels")
    parser.add_argument("--all-touched", dest="all_touched", action="store_true", default=DEFAULT_ALL_TOUCHED, help="GDAL ALL_TOUCHED=TRUE")
    parser.add_argument("--no-all-touched", dest="all_touched", action="store_false", help="disable GDAL ALL_TOUCHED")
    parser.add_argument("--output-dir", type=pathlib.Path, default=None, help="output directory")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    config = resolve_runtime_config(
        city=args.city,
        blocks=args.blocks,
        buildings=args.buildings,
        output_dir=args.output_dir,
        block_layer=args.block_layer,
        building_layer=args.building_layer,
        block_id_column=args.block_id_column,
        target_crs=args.target_crs,
        pixel_size=args.pixel_size,
        tile_size=args.tile_size,
        all_touched=args.all_touched,
    )

    extract_block_building_pct(
        block_path=config.block_path,
        building_path=config.building_path,
        output_dir=config.output_dir,
        block_layer=config.block_layer,
        building_layer=config.building_layer,
        block_id_column=config.block_id_column,
        value_columns=split_csv(args.value_columns) or DEFAULT_VALUE_COLUMNS,
        category_columns=split_csv(args.category_columns) or DEFAULT_CATEGORY_COLUMNS,
        allowed_categories={item.lower() for item in split_csv(args.allowed_categories)},
        target_crs=config.target_crs,
        pixel_size=config.pixel_size,
        tile_size=config.tile_size,
        all_touched=config.all_touched,
        verbose=args.verbose,
    )


if __name__ == "__main__":
    main()
