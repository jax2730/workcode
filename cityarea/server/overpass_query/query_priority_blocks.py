if __name__ == "__main__" or __package__ is None:
    from osm_util import *
else:
    from .osm_util import *

import sys
import os
import argparse
import pathlib
import sqlite3
from concurrent.futures import ThreadPoolExecutor
import osmium
import osmium.osm.types as osm_types
import osmium.geom
import geopandas as gpd
import pandas as pd
import shapely.wkb as wkblib
import shapely.geometry as geomlib
import xml.etree.ElementTree as ET

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from constants import *

# Reduce noisy Qt network warnings in headless/server runs
os.environ.setdefault('QT_LOGGING_RULES', 'qt.network.*=false')
# Keep geospatial stack offline and deterministic on server
os.environ.setdefault('PROJ_NETWORK', 'OFF')


CITY_DIR = DATA_DIR / "city"

# Higher priority first
PRIORITY_ORDER = ["landuse", "amenity", "leisure", "tourism", "building", "man_made"]
ONLINE_QUERY_KEYS = ["amenity", "leisure", "tourism", "man_made"]


class PrioritySurfaceHandler(osmium.SimpleHandler):
    def __init__(self, allowed_keys: list[str]):
        super().__init__()
        self.features = []
        self.wkbfab = osmium.geom.WKBFactory()
        self.allowed_keys = allowed_keys

    def _pick_source_key(self, tags) -> tuple[str | None, str | None]:
        for key in self.allowed_keys:
            if key in tags:
                value = tags.get(key)
                if value is not None and str(value).strip() != "":
                    return key, value
        return None, None

    def _extract_name_fields(self, tags) -> tuple[str | None, str | None, str | None]:
        name_local = tags.get('name')
        name_en = tags.get('name:en') if 'name:en' in tags else None
        name_zh = tags.get('name:zh') if 'name:zh' in tags else (
            tags.get('name:ko-Hani') if 'name:ko-Hani' in tags else None
        )
        return name_local, name_en, name_zh

    def _append_feature(self, feature_id: int, osm_type: str, tags, geometry):
        source_key, source_value = self._pick_source_key(tags)
        if source_key is None:
            return

        # Exclude bridge from man_made category
        if source_key == "man_made" and str(source_value).lower() == "bridge":
            return

        # Filter out building parts (too fragmented)
        if source_key == "building":
            if 'building:part' in tags and str(tags.get('building:part')).lower() not in ("", "no", "false", "0"):
                return

        name_local, name_en, name_zh = self._extract_name_fields(tags)

        self.features.append({
            'id': feature_id,
            'osm_type': osm_type,
            'category': source_key,
            'value': source_value,
            'name:local': name_local,
            'name:en': name_en,
            'name:zh': name_zh,
            'geometry': geometry,
        })

    def way(self, w: osm_types.Way):
        if not w.is_closed():
            return
        if not w.tags:
            return

        try:
            if hasattr(self.wkbfab, 'create_polygon'):
                wkb = self.wkbfab.create_polygon(w)
                geometry = wkblib.loads(wkb, hex=True)
            else:
                wkb = self.wkbfab.create_linestring(w)
                line = wkblib.loads(wkb, hex=True)
                coords = list(line.coords)
                if len(coords) < 4:
                    return
                if coords[0] != coords[-1]:
                    coords.append(coords[0])
                geometry = geomlib.Polygon(coords)
                if not geometry.is_valid:
                    geometry = geometry.buffer(0)
                if geometry.is_empty:
                    return

            if geometry.geom_type not in ['Polygon', 'MultiPolygon']:
                return

            self._append_feature(w.id, 'way', w.tags, geometry)
        except Exception:
            return

    def area(self, a: osm_types.Area):
        if not a.tags:
            return

        try:
            wkb = self.wkbfab.create_multipolygon(a)
            geometry = wkblib.loads(wkb, hex=True)
            if geometry.geom_type not in ['Polygon', 'MultiPolygon']:
                return
            self._append_feature(a.id, 'area', a.tags, geometry)
        except Exception:
            return


def _normalize_keys(keys: list[str]) -> list[str]:
    seen = set()
    normalized = []
    for key in keys:
        text = str(key).strip()
        if text == "":
            continue
        if text not in seen:
            normalized.append(text)
            seen.add(text)
    return normalized


def _build_query_by_keys(city_name: str, radius: int, keys: list[str]) -> str:
    clauses = []
    for key in keys:
        clauses.append(f"way(around.city_center:{radius})[{key}];")
        clauses.append(f"relation(around.city_center:{radius})[{key}];")
    clause_text = "\n            ".join(clauses)

    return f"""
    [out:xml][timeout:1800][maxsize:2147483648];
    (
      node[place~"city|town"]["name:en"="{city_name}"];
      node[place~"city|town"]["name"="{city_name}"];
      node[place~"city|town"]["name:zh"="{city_name}"];
    )->.city_center;
    (
            {clause_text}
    );
    out body;
    >;
    out skel qt;
    """


def _build_query(city_name: str, radius: int) -> str:
    return _build_query_by_keys(city_name, radius, ONLINE_QUERY_KEYS)


def _build_query_fallback_by_keys(city_name: str, radius: int, keys: list[str]) -> str:
        clauses = []
        for key in keys:
            clauses.append(f"way(around:{radius})[{key}];")
            clauses.append(f"relation(around:{radius})[{key}];")
        clause_text = "\n            ".join(clauses)

        """Fallback local query template using strict city anchor style."""
        return f"""
        [out:xml][timeout:1800][maxsize:2147483648];
        node[place="city"]["name:en"="{city_name}"];
        (
            {clause_text}
        );
        out body;
        >;
        out skel qt;
        """


def _build_query_fallback(city_name: str, radius: int) -> str:
    return _build_query_fallback_by_keys(city_name, radius, ONLINE_QUERY_KEYS)


def _query_osm(city_name: str, radius: int, osm_file: pathlib.Path, log: bool = False) -> bool:
    """Local-db only query. No remote fallback."""
    return _query_osm_by_keys(city_name, radius, ONLINE_QUERY_KEYS, osm_file, log=log)


def _query_osm_by_keys(
    city_name: str,
    radius: int,
    keys: list[str],
    osm_file: pathlib.Path,
    log: bool = False,
) -> bool:
    keys = _normalize_keys(keys)
    if len(keys) == 0:
        if log:
            print("❌ Empty key list for OSM query")
        return False

    primary_query = _build_query_by_keys(city_name, radius, keys)
    ret = query_osm_data_local(primary_query, osm_file, log)
    if ret == 0 and osm_file.exists() and (not osm_result_empty(osm_file)):
        return True

    if log:
        print("⚠️  Primary local query empty, trying fallback local template...")

    fallback_query = _build_query_fallback_by_keys(city_name, radius, keys)
    ret_fb = query_osm_data_local(fallback_query, osm_file, log)
    if ret_fb == 0 and osm_file.exists() and (not osm_result_empty(osm_file)):
        return True

    if log:
        print(f"❌ Local database query returned empty for keys={keys} (both templates).")
    return False


def _union_all_safe(geo_series: gpd.GeoSeries):
    if len(geo_series) == 0:
        return None
    try:
        return geo_series.union_all()
    except Exception:
        return geo_series.unary_union


def _difference_in_chunks(gdf: gpd.GeoDataFrame, covered_union, chunk_size: int = 20000) -> gpd.GeoDataFrame:
    if len(gdf) == 0:
        return gdf

    chunks = []
    total = len(gdf)
    for start in range(0, total, chunk_size):
        end = min(start + chunk_size, total)
        part = gdf.iloc[start:end].copy()
        part['geometry'] = part.geometry.difference(covered_union)
        part = part[~part.geometry.is_empty].copy()
        part = part[part.geometry.notna()].copy()
        if len(part) > 0:
            chunks.append(part)

    if len(chunks) == 0:
        return gpd.GeoDataFrame(columns=gdf.columns, geometry='geometry', crs=gdf.crs)

    out = pd.concat(chunks, ignore_index=True)
    return gpd.GeoDataFrame(out, geometry='geometry', crs=gdf.crs)


def _difference_tile_worker(tile_gdf: gpd.GeoDataFrame, covered_union) -> gpd.GeoDataFrame:
    if len(tile_gdf) == 0:
        return tile_gdf

    tile = tile_gdf.copy()
    tile['geometry'] = tile.geometry.difference(covered_union)
    tile = tile[~tile.geometry.is_empty].copy()
    tile = tile[tile.geometry.notna()].copy()
    return tile


def _difference_by_tiles_parallel(
    gdf: gpd.GeoDataFrame,
    covered_union,
    tile_size_m: float = 4000.0,
    workers: int = 0,
    log: bool = False,
) -> gpd.GeoDataFrame:
    if len(gdf) == 0:
        return gdf

    if tile_size_m <= 0:
        return _difference_in_chunks(gdf, covered_union, chunk_size=20000)

    bounds = gdf.total_bounds  # minx, miny, maxx, maxy
    minx, miny = bounds[0], bounds[1]

    temp = gdf.copy()
    centroids = temp.geometry.centroid
    temp['_tile_x'] = ((centroids.x - minx) // tile_size_m).astype(int)
    temp['_tile_y'] = ((centroids.y - miny) // tile_size_m).astype(int)
    temp['_tile_id'] = temp['_tile_x'].astype(str) + '_' + temp['_tile_y'].astype(str)

    tile_groups = [tile_df.drop(columns=['_tile_x', '_tile_y', '_tile_id']) for _, tile_df in temp.groupby('_tile_id')]
    if len(tile_groups) == 0:
        return gpd.GeoDataFrame(columns=gdf.columns, geometry='geometry', crs=gdf.crs)

    if workers <= 0:
        workers = os.cpu_count() or 4
    workers = max(1, min(workers, len(tile_groups)))

    if log:
        print(f"    building tile-parallel: tiles={len(tile_groups)}, workers={workers}, tile_size={tile_size_m}m")

    if workers == 1 or len(tile_groups) == 1:
        out_frames = [_difference_tile_worker(tile_df, covered_union) for tile_df in tile_groups]
    else:
        with ThreadPoolExecutor(max_workers=workers) as executor:
            futures = [executor.submit(_difference_tile_worker, tile_df, covered_union) for tile_df in tile_groups]
            out_frames = [f.result() for f in futures]

    out_frames = [df for df in out_frames if len(df) > 0]
    if len(out_frames) == 0:
        return gpd.GeoDataFrame(columns=gdf.columns, geometry='geometry', crs=gdf.crs)

    out = pd.concat(out_frames, ignore_index=True)
    return gpd.GeoDataFrame(out, geometry='geometry', crs=gdf.crs)


def _clip_category_with_union(
    cat: gpd.GeoDataFrame,
    covered_union,
    category: str,
    building_chunk_size: int = 20000,
    building_tile_size: float = 4000.0,
    building_workers: int = 0,
    log: bool = False,
) -> gpd.GeoDataFrame:
    """Clip category by covered union with fast path for large building layer."""
    if covered_union is None or len(cat) == 0:
        return cat

    if category != 'building':
        cat = cat.copy()
        cat['geometry'] = cat.geometry.difference(covered_union)
        cat = cat[~cat.geometry.is_empty].copy()
        cat = cat[cat.geometry.notna()].copy()
        return cat

    # Building fast path:
    # 1) keep non-overlap buildings directly
    # 2) run expensive difference only on overlap subset
    minx, miny, maxx, maxy = covered_union.bounds
    bounds = cat.bounds
    maybe_overlap_mask = ~(
        (bounds.maxx < minx) |
        (bounds.minx > maxx) |
        (bounds.maxy < miny) |
        (bounds.miny > maxy)
    )

    maybe_overlap = cat[maybe_overlap_mask].copy()
    no_overlap = cat[~maybe_overlap_mask].copy()

    if len(maybe_overlap) > 0:
        real_overlap_mask = maybe_overlap.geometry.intersects(covered_union)
        overlap = maybe_overlap[real_overlap_mask].copy()
        no_overlap = pd.concat([no_overlap, maybe_overlap[~real_overlap_mask]], ignore_index=True)
    else:
        overlap = maybe_overlap

    if len(overlap) > 0:
        try:
            fully_covered_mask = overlap.geometry.covered_by(covered_union)
        except Exception:
            fully_covered_mask = overlap.geometry.apply(lambda geom: geom.covered_by(covered_union))

        partial_overlap = overlap[~fully_covered_mask].copy()
        if len(partial_overlap) > 0:
            overlap = _difference_by_tiles_parallel(
                partial_overlap,
                covered_union,
                tile_size_m=building_tile_size,
                workers=building_workers,
                log=log,
            )
        else:
            overlap = gpd.GeoDataFrame(columns=cat.columns, geometry='geometry', crs=cat.crs)

    out = pd.concat([no_overlap, overlap], ignore_index=True)
    out = gpd.GeoDataFrame(out, geometry='geometry', crs=cat.crs)

    if log:
        print(
            f"    building fast-clip: total={len(cat)}, "
            f"bbox-candidates={len(maybe_overlap)}, overlap-kept={len(overlap)}, kept={len(out)}"
        )

    return out


def _clip_by_priority(
    gdf: gpd.GeoDataFrame,
    skip_building: bool = False,
    building_chunk_size: int = 20000,
    building_tile_size: float = 4000.0,
    building_workers: int = 0,
    log: bool = False,
) -> gpd.GeoDataFrame:
    if len(gdf) == 0:
        return gdf

    work = gdf.to_crs(epsg=3857).copy()
    result_frames = []
    covered_union = None

    for category in PRIORITY_ORDER:
        if skip_building and category == 'building':
            if log:
                print("  Skip building category by option")
            continue

        cat = work[work['category'] == category].copy()
        if len(cat) == 0:
            continue

        if covered_union is not None:
            cat = _clip_category_with_union(
                cat,
                covered_union,
                category=category,
                building_chunk_size=building_chunk_size,
                building_tile_size=building_tile_size,
                building_workers=building_workers,
                log=log,
            )

        cat = cat[cat.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
        if len(cat) == 0:
            continue

        result_frames.append(cat)

        cat_union = _union_all_safe(cat.geometry)
        if cat_union is None:
            continue

        if covered_union is None:
            covered_union = cat_union
        else:
            try:
                covered_union = covered_union.union(cat_union)
            except Exception:
                covered_union = gpd.GeoSeries([covered_union, cat_union]).union_all()

        if log:
            print(f"  Kept {len(cat)} polygons for {category}")

    if len(result_frames) == 0:
        return gpd.GeoDataFrame(columns=gdf.columns, geometry='geometry', crs=work.crs)

    out = pd.concat(result_frames, ignore_index=True)
    out = gpd.GeoDataFrame(out, geometry='geometry', crs=work.crs)
    out['area_m2'] = out.geometry.area
    return out.to_crs(epsg=4326)


def _sanitize_layer_name(name: str) -> str:
    cleaned = ''.join(ch if ch.isalnum() or ch == '_' else '_' for ch in name)
    while '__' in cleaned:
        cleaned = cleaned.replace('__', '_')
    cleaned = cleaned.strip('_')
    return cleaned[:50] if cleaned else 'layer'


def _empty_block_gdf() -> gpd.GeoDataFrame:
    return gpd.GeoDataFrame(
        columns=['id', 'osm_type', 'category', 'value', 'name:local', 'name:en', 'name:zh', 'geometry'],
        geometry='geometry',
        crs='EPSG:4326',
    )


def _list_gpkg_layers(gpkg_path: pathlib.Path) -> list[str]:
    if not gpkg_path.exists():
        return []
    conn = sqlite3.connect(str(gpkg_path))
    try:
        rows = conn.execute("SELECT table_name FROM gpkg_contents WHERE data_type = 'features'").fetchall()
        return [row[0] for row in rows]
    finally:
        conn.close()


def _standardize_block_columns(gdf: gpd.GeoDataFrame, category: str, value_source: str) -> gpd.GeoDataFrame:
    if len(gdf) == 0:
        return _empty_block_gdf()

    out = gdf.copy()
    if out.crs is None:
        out.set_crs(epsg=4326, inplace=True)

    # Names
    if 'name:local' not in out.columns:
        out['name:local'] = out['name'] if 'name' in out.columns else None
    if 'name:en' not in out.columns:
        out['name:en'] = None
    if 'name:zh' not in out.columns:
        out['name:zh'] = None

    # ID / OSM type
    if 'id' not in out.columns:
        out['id'] = range(1, len(out) + 1)
    if 'osm_type' not in out.columns:
        out['osm_type'] = 'gpkg'

    # Category/value
    out['category'] = category
    if value_source in out.columns:
        out['value'] = out[value_source].astype(str)
    else:
        out['value'] = value_source

    out = out[out.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
    if len(out) == 0:
        return _empty_block_gdf()

    return out[['id', 'osm_type', 'category', 'value', 'name:local', 'name:en', 'name:zh', 'geometry']]


def _load_landuse_blocks(save_dir: pathlib.Path, log: bool = False) -> gpd.GeoDataFrame:
    landuse_gpkg = save_dir / 'landuse.gpkg'
    if not landuse_gpkg.exists():
        if log:
            print(f"⚠️  Missing landuse.gpkg, skip landuse source: {landuse_gpkg}")
        return _empty_block_gdf()

    try:
        gdf = gpd.read_file(str(landuse_gpkg), layer='landuse')
    except Exception as error:
        if log:
            print(f"⚠️  Failed reading landuse.gpkg: {error}")
        return _empty_block_gdf()

    return _standardize_block_columns(gdf, category='landuse', value_source='landuse')


def _load_building_blocks(save_dir: pathlib.Path, log: bool = False) -> gpd.GeoDataFrame:
    building_gpkg = save_dir / 'All_Building.gpkg'
    if not building_gpkg.exists():
        if log:
            print(f"⚠️  Missing All_Building.gpkg, skip building source: {building_gpkg}")
        return _empty_block_gdf()

    layers = _list_gpkg_layers(building_gpkg)
    if len(layers) == 0:
        if log:
            print(f"⚠️  All_Building.gpkg has no feature layers: {building_gpkg}")
        return _empty_block_gdf()

    frames = []
    for layer in layers:
        try:
            gdf = gpd.read_file(str(building_gpkg), layer=layer)
        except Exception:
            continue
        if len(gdf) == 0:
            continue

        standardized = _standardize_block_columns(gdf, category='building', value_source='value')
        if len(standardized) == 0:
            continue

        # If no value column in source, use layer name as value
        if 'value' not in gdf.columns:
            standardized['value'] = layer
        frames.append(standardized)

    if len(frames) == 0:
        return _empty_block_gdf()

    out = pd.concat(frames, ignore_index=True)
    out = gpd.GeoDataFrame(out, geometry='geometry', crs=frames[0].crs)
    return out


def _load_online_blocks(
    city_name: str,
    radius: int,
    save_dir: pathlib.Path,
    keys: list[str] | None = None,
    log: bool = False,
) -> gpd.GeoDataFrame:
    if keys is None:
        keys = ONLINE_QUERY_KEYS
    keys = _normalize_keys(keys)

    osm_file = save_dir / 'priority_blocks_online.osm'
    ok = _query_osm_by_keys(city_name, radius, keys, osm_file, log=log)
    if not ok:
        if log:
            print(f"⚠️  Online query failed for keys={keys}; continuing with cached sources")
        return _empty_block_gdf()

    handler = PrioritySurfaceHandler(allowed_keys=keys)
    handler.apply_file(str(osm_file), locations=True)
    if len(handler.features) == 0:
        if log:
            try:
                node_count = 0
                way_count = 0
                relation_count = 0
                root = ET.parse(str(osm_file)).getroot()
                for child in root:
                    if child.tag == 'node':
                        node_count += 1
                    elif child.tag == 'way':
                        way_count += 1
                    elif child.tag == 'relation':
                        relation_count += 1
                print("⚠️  Online query parsed 0 polygon features.")
                print(f"    OSM content counts -> node: {node_count}, way: {way_count}, relation: {relation_count}")
            except Exception:
                print("⚠️  Online query parsed 0 polygon features (failed to inspect OSM file)")
        return _empty_block_gdf()

    gdf = gpd.GeoDataFrame(handler.features, crs='EPSG:4326')
    gdf = gdf[gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
    if len(gdf) == 0:
        return _empty_block_gdf()
    return gdf


def _ensure_priority_columns(gdf: gpd.GeoDataFrame) -> gpd.GeoDataFrame:
    if len(gdf) == 0:
        return _empty_block_gdf()

    out = gdf.copy()
    if out.crs is None:
        out.set_crs(epsg=4326, inplace=True)

    if 'id' not in out.columns:
        out['id'] = range(1, len(out) + 1)
    if 'osm_type' not in out.columns:
        out['osm_type'] = 'gpkg'
    if 'category' not in out.columns:
        out['category'] = 'unknown'
    if 'value' not in out.columns:
        out['value'] = out['category'].astype(str)

    if 'name:local' not in out.columns:
        out['name:local'] = out['name'] if 'name' in out.columns else None
    if 'name:en' not in out.columns:
        out['name:en'] = None
    if 'name:zh' not in out.columns:
        out['name:zh'] = None

    out = out[out.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
    if len(out) == 0:
        return _empty_block_gdf()

    return out[['id', 'osm_type', 'category', 'value', 'name:local', 'name:en', 'name:zh', 'geometry']]


def _category_sort_key(category: str) -> tuple[int, str]:
    if category in PRIORITY_ORDER:
        return PRIORITY_ORDER.index(category), category
    return len(PRIORITY_ORDER) + 999, category


def incremental_add_categories_to_priority_blocks(
    city_name: str,
    add_categories: list[str],
    base_gpkg_name: str = 'priority_blocks.gpkg',
    output_name: str | None = None,
    radius: int = 50000,
    log: bool = False,
):
    save_dir = CITY_DIR / city_name
    save_dir.mkdir(parents=True, exist_ok=True)

    add_categories = _normalize_keys(add_categories)
    if len(add_categories) == 0:
        print("❌ add_categories is empty")
        return

    base_file = save_dir / base_gpkg_name
    if not base_file.exists():
        print(f"❌ Base priority gpkg not found: {base_file}")
        return

    try:
        base_gdf_raw = gpd.read_file(str(base_file), layer='priority_blocks')
    except Exception as error:
        print(f"❌ Failed to read layer 'priority_blocks' from {base_file}: {error}")
        return

    base_gdf = _ensure_priority_columns(base_gdf_raw)
    if len(base_gdf) == 0:
        print("❌ Base priority layer has no polygon features")
        return

    if log:
        print(f"Incremental mode: load existing blocks from {base_file}")
        print(f"  existing polygons: {len(base_gdf)}")
        print(f"  add categories: {', '.join(add_categories)}")

    key_suffix = '_'.join([_sanitize_layer_name(k) for k in add_categories])
    inc_osm_file = save_dir / f'priority_blocks_incremental_{key_suffix}.osm'

    ok = _query_osm_by_keys(city_name, radius, add_categories, inc_osm_file, log=log)
    if not ok:
        print(f"❌ Query failed for incremental categories={add_categories}")
        return

    handler = PrioritySurfaceHandler(allowed_keys=add_categories)
    handler.apply_file(str(inc_osm_file), locations=True)
    if len(handler.features) == 0:
        print("❌ Incremental query parsed 0 polygon features")
        return

    add_gdf = gpd.GeoDataFrame(handler.features, crs='EPSG:4326')
    add_gdf = add_gdf[add_gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
    if len(add_gdf) == 0:
        print("❌ Incremental features contain no polygon geometries")
        return

    base_3857 = base_gdf.to_crs(epsg=3857)
    covered_union = _union_all_safe(base_3857.geometry)

    add_3857 = add_gdf.to_crs(epsg=3857)
    if covered_union is not None:
        add_3857['geometry'] = add_3857.geometry.difference(covered_union)
        add_3857 = add_3857[~add_3857.geometry.is_empty].copy()
        add_3857 = add_3857[add_3857.geometry.notna()].copy()

    if len(add_3857) == 0:
        print("✅ No new uncovered polygons after clipping with existing priority blocks")
        return

    add_3857['area_m2'] = add_3857.geometry.area
    add_out = add_3857.to_crs(epsg=4326)

    base_out = base_gdf.copy()
    if 'area_m2' not in base_out.columns:
        base_out['area_m2'] = base_out.to_crs(epsg=3857).geometry.area

    keep_cols = ['id', 'osm_type', 'category', 'value', 'name:local', 'name:en', 'name:zh', 'geometry', 'area_m2']
    for col in keep_cols:
        if col not in base_out.columns:
            base_out[col] = None
        if col not in add_out.columns:
            add_out[col] = None

    combined = pd.concat([
        base_out[keep_cols],
        add_out[keep_cols],
    ], ignore_index=True)
    combined = gpd.GeoDataFrame(combined, geometry='geometry', crs='EPSG:4326')

    if output_name is None or str(output_name).strip() == '':
        output_name = f'priority_blocks_incremental_{key_suffix}.gpkg'
    output_file = save_dir / output_name
    if output_file.exists():
        output_file.unlink()

    combined.to_file(str(output_file), layer='priority_blocks', driver='GPKG')

    categories_sorted = sorted(combined['category'].dropna().astype(str).unique().tolist(), key=_category_sort_key)
    for category in categories_sorted:
        cat = combined[combined['category'] == category].copy()
        if len(cat) == 0:
            continue
        layer = _sanitize_layer_name(f"cat_{category}")
        cat.to_file(str(output_file), layer=layer, driver='GPKG', mode='a')

    print(f"✅ Incremental priority blocks saved: {output_file}")
    print(f"   Base polygons: {len(base_gdf)}")
    print(f"   Added polygons: {len(add_out)}")
    print(f"   Total polygons: {len(combined)}")


def query_priority_blocks(
    city_name: str,
    radius: int = 50000,
    skip_online: bool = False,
    skip_building: bool = False,
    min_building_area: float = 0.0,
    building_chunk_size: int = 20000,
    building_tile_size: float = 4000.0,
    building_workers: int = 0,
    log: bool = False,
):
    save_dir = CITY_DIR / city_name
    save_dir.mkdir(parents=True, exist_ok=True)

    if log:
        print(f"Building priority blocks for {city_name} using cached + online sources...")
        print("Priority (high -> low): landuse > amenity > leisure > tourism > building > man_made")

    landuse_gdf = _load_landuse_blocks(save_dir, log=log)
    building_gdf = _load_building_blocks(save_dir, log=log)

    online_keys = ONLINE_QUERY_KEYS.copy()
    if len(landuse_gdf) == 0 and 'landuse' not in online_keys:
        online_keys = ['landuse'] + online_keys
        if log:
            print("⚠️  landuse.gpkg missing; add 'landuse' to online query keys")

    online_gdf = _empty_block_gdf() if skip_online else _load_online_blocks(
        city_name,
        radius,
        save_dir,
        keys=online_keys,
        log=log,
    )

    if len(building_gdf) > 0 and min_building_area > 0:
        b3857 = building_gdf.to_crs(epsg=3857)
        area_mask = b3857.geometry.area >= min_building_area
        before = len(building_gdf)
        building_gdf = building_gdf[area_mask.values].copy()
        if log:
            print(f"  building area filter >= {min_building_area} m²: {before} -> {len(building_gdf)}")

    frames = [df for df in [landuse_gdf, online_gdf, building_gdf] if len(df) > 0]
    if len(frames) == 0:
        print("❌ No input polygons available. Ensure landuse.gpkg / All_Building.gpkg exist, or online query works.")
        return

    gdf = pd.concat(frames, ignore_index=True)
    gdf = gpd.GeoDataFrame(gdf, geometry='geometry', crs='EPSG:4326')
    gdf = gdf[gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()

    if len(gdf) == 0:
        print("❌ Parsed features contain no polygon geometries")
        return

    if log:
        print(f"Loaded {len(gdf)} polygon features before priority clipping")
        print(f"  landuse: {len(landuse_gdf)}")
        print(f"  online({','.join(online_keys)}): {len(online_gdf)}")
        print(f"  building: {len(building_gdf)}")

    dedup = _clip_by_priority(
        gdf,
        skip_building=skip_building,
        building_chunk_size=building_chunk_size,
        building_tile_size=building_tile_size,
        building_workers=building_workers,
        log=log,
    )
    if len(dedup) == 0:
        print("❌ No polygons left after priority clipping")
        return

    output_file = save_dir / 'priority_blocks.gpkg'
    if output_file.exists():
        output_file.unlink()

    # Combined layer
    dedup.to_file(str(output_file), layer='priority_blocks', driver='GPKG')

    # Category sublayers for easy QGIS usage
    first_extra = True
    for category in PRIORITY_ORDER:
        cat = dedup[dedup['category'] == category].copy()
        if len(cat) == 0:
            continue
        layer = _sanitize_layer_name(f"cat_{category}")
        if first_extra:
            cat.to_file(str(output_file), layer=layer, driver='GPKG', mode='a')
            first_extra = False
        else:
            cat.to_file(str(output_file), layer=layer, driver='GPKG', mode='a')

    print(f"✅ Saved priority blocks: {output_file}")
    print(f"   Total polygons: {len(dedup)}")
    print("   Priority order: landuse > amenity > leisure > tourism > building > man_made")


def main():
    parser = argparse.ArgumentParser(
        description='Generate area blocks for landuse/amenity/leisure/tourism/building with priority clipping'
    )
    parser.add_argument('city', nargs='?', default=DEFAULT_CITY, help=f'City name (default: {DEFAULT_CITY})')  # type: ignore
    parser.add_argument('--radius', type=int, default=50000, help='Search radius in meters (default: 50000)')
    parser.add_argument('--skip-online', action='store_true', help='Skip online amenity/leisure/tourism query and use cached files only')
    parser.add_argument('--skip-building', action='store_true', help='Skip building category to speed up processing')
    parser.add_argument('--min-building-area', type=float, default=0.0, help='Filter out buildings smaller than this area (m²) before clipping')
    parser.add_argument('--building-chunk-size', type=int, default=20000, help='Chunk size for building difference operation (default: 20000)')
    parser.add_argument('--building-tile-size', type=float, default=4000.0, help='Tile size in meters for parallel building clipping (default: 4000)')
    parser.add_argument('--building-workers', type=int, default=0, help='Worker count for tile parallel clipping (0 = CPU count)')
    parser.add_argument('--incremental', action='store_true', help='Incrementally add categories based on existing priority_blocks.gpkg')
    parser.add_argument('--base-gpkg', type=str, default='priority_blocks.gpkg', help='Base gpkg filename used in incremental mode (default: priority_blocks.gpkg)')
    parser.add_argument('--add-categories', nargs='+', default=['man_made'], help='Categories to append in incremental mode (default: man_made)')
    parser.add_argument('--output', type=str, default=None, help='Output gpkg filename; default auto name in incremental mode')
    parser.add_argument('--log', action='store_true', help='Enable detailed logging')
    args = parser.parse_args()

    if args.incremental:
        incremental_add_categories_to_priority_blocks(
            city_name=args.city,
            add_categories=args.add_categories,
            base_gpkg_name=args.base_gpkg,
            output_name=args.output,
            radius=args.radius,
            log=args.log,
        )
        return

    query_priority_blocks(
        args.city,
        radius=args.radius,
        skip_online=args.skip_online,
        skip_building=args.skip_building,
        min_building_area=args.min_building_area,
        building_chunk_size=args.building_chunk_size,
        building_tile_size=args.building_tile_size,
        building_workers=args.building_workers,
        log=args.log,
    )


if __name__ == '__main__':
    main()
