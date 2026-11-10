if __name__ == "__main__" or __package__ is None:
    from osm_util import *
else:
    from .osm_util import *

import sys
import os
import argparse
import pathlib
import geopandas as gpd
import pandas as pd
import shapely.wkb as wkblib
import shapely.geometry as geomlib
import osmium
import osmium.osm.types as osm_types

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from constants import *


CITY_DIR = DATA_DIR / "city"

# 只需要改这里：
# - 一个类别： ["amenity"]
# - 两个类别： ["amenity", "leisure"]
# - 三个类别： ["amenity", "leisure", "tourism"]
DEFAULT_CATEGORIES = ["amenity", "leisure", "tourism"]


class CategorySurfaceHandler(osmium.SimpleHandler):
    def __init__(self, allowed_keys: list[str]):
        super().__init__()
        self.features = []
        self.allowed_keys = allowed_keys
        self.wkbfab = osmium.geom.WKBFactory()

    def _pick_category(self, tags) -> tuple[str | None, str | None]:
        for key in self.allowed_keys:
            if key in tags:
                value = tags.get(key)
                if value is not None and str(value).strip() != "":
                    return key, value
        return None, None

    def _extract_names(self, tags) -> tuple[str | None, str | None, str | None]:
        name_local = tags.get('name')
        name_en = tags.get('name:en') if 'name:en' in tags else None
        name_zh = tags.get('name:zh') if 'name:zh' in tags else (
            tags.get('name:ko-Hani') if 'name:ko-Hani' in tags else None
        )
        return name_local, name_en, name_zh

    def _append_feature(self, feature_id: int, osm_type: str, tags, geometry):
        category, value = self._pick_category(tags)
        if category is None:
            return

        # building:part 一般过碎，默认排除
        if category == 'building':
            part_value = tags.get('building:part') if 'building:part' in tags else None
            if part_value is not None and str(part_value).lower() not in ("", "no", "false", "0"):
                return

        name_local, name_en, name_zh = self._extract_names(tags)

        self.features.append({
            'id': feature_id,
            'osm_type': osm_type,
            'category': category,
            'value': value,
            'name:local': name_local,
            'name:en': name_en,
            'name:zh': name_zh,
            'geometry': geometry,
        })

    def way(self, w: osm_types.Way):
        if not w.tags or not w.is_closed():
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


def _normalize_categories(categories: list[str]) -> list[str]:
    normalized = []
    seen = set()
    for cat in categories:
        key = str(cat).strip()
        if key == "":
            continue
        if key not in seen:
            normalized.append(key)
            seen.add(key)
    return normalized


def _build_query(city_name: str, radius: int, categories: list[str]) -> str:
    clauses = []
    for key in categories:
        clauses.append(f"way(around.city_center:{radius})[{key}];")
        clauses.append(f"relation(around.city_center:{radius})[{key}];")

    clause_text = "\n        ".join(clauses)

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


def _build_query_fallback(city_name: str, radius: int, categories: list[str]) -> str:
    clauses = []
    for key in categories:
        clauses.append(f"way(around:{radius})[{key}];")
        clauses.append(f"relation(around:{radius})[{key}];")

    clause_text = "\n            ".join(clauses)

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


def _query_osm(city_name: str, radius: int, categories: list[str], osm_file: pathlib.Path, log: bool = False) -> bool:
    query = _build_query(city_name, radius, categories)

    local_code = query_osm_data_local(query, osm_file, log)
    if local_code == 0 and osm_file.exists() and (not osm_result_empty(osm_file)):
        return True

    if log:
        print("⚠️  Primary local query returned empty, trying fallback local query...")

    fallback = _build_query_fallback(city_name, radius, categories)
    fallback_code = query_osm_data_local(fallback, osm_file, log)
    if fallback_code == 0 and osm_file.exists() and (not osm_result_empty(osm_file)):
        return True

    if log:
        print("⚠️  Local query failed/empty, trying remote Overpass...")

    remote_code = query_osm_data_remote(query, osm_file, log)
    if remote_code == 200 and osm_file.exists() and (not osm_result_empty(osm_file)):
        return True

    if log:
        print("❌ Query failed for all paths (local primary, local fallback, remote)")
    return False


def _sanitize_layer_name(name: str) -> str:
    cleaned = ''.join(ch if ch.isalnum() or ch == '_' else '_' for ch in name)
    while '__' in cleaned:
        cleaned = cleaned.replace('__', '_')
    cleaned = cleaned.strip('_')
    return cleaned[:50] if cleaned else 'layer'


def _build_output_filename(categories: list[str]) -> str:
    if len(categories) == 0:
        return 'categories_selection.gpkg'

    safe_parts = [_sanitize_layer_name(cat) for cat in categories]
    base_name = 'categories_' + '_'.join(safe_parts)
    if len(base_name) > 180:
        base_name = base_name[:180].rstrip('_')
    return f'{base_name}.gpkg'


def query_city_categories(
    city_name: str,
    categories: list[str] | None = None,
    radius: int = 50000,
    output_name: str | None = None,
    log: bool = False,
):
    categories = _normalize_categories(categories or DEFAULT_CATEGORIES)
    if len(categories) == 0:
        print("❌ categories 为空，请至少提供一个类别键（例如 amenity）")
        return

    save_dir = CITY_DIR / city_name
    save_dir.mkdir(parents=True, exist_ok=True)

    osm_file = save_dir / 'categories_selection.osm'
    ok = _query_osm(city_name, radius, categories, osm_file, log=log)
    if not ok:
        print(f"❌ No OSM data found for {city_name}, categories={categories}")
        return

    handler = CategorySurfaceHandler(allowed_keys=categories)
    handler.apply_file(str(osm_file), locations=True)

    if len(handler.features) == 0:
        print(f"❌ Parsed 0 polygon features for categories={categories}")
        return

    gdf = gpd.GeoDataFrame(handler.features, crs='EPSG:4326')
    gdf = gdf[gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()

    if len(gdf) == 0:
        print(f"❌ No polygon features left after geometry filtering for categories={categories}")
        return

    gdf = gdf.reset_index(drop=True)
    gdf['area_m2'] = gdf.to_crs(epsg=3857).geometry.area

    if output_name is None or str(output_name).strip() == '':
        output_name = _build_output_filename(categories)

    output_file = save_dir / output_name
    if output_file.exists():
        output_file.unlink()

    # 1) 合并图层（QGIS 一次加载即可）
    gdf.to_file(str(output_file), layer='categories_all', driver='GPKG')

    # 2) 每个 category 的子图层（QGIS 可按图层单独开关）
    for category in categories:
        part = gdf[gdf['category'] == category].copy()
        if len(part) == 0:
            continue
        layer_name = _sanitize_layer_name(f'cat_{category}')
        part.to_file(str(output_file), layer=layer_name, driver='GPKG', mode='a')

    print(f"✅ Saved category polygons: {output_file}")
    print(f"   Categories: {', '.join(categories)}")
    print(f"   Total polygons: {len(gdf)}")


def main():
    parser = argparse.ArgumentParser(
        description='Query one or more OSM categories and export polygon layers for QGIS'
    )
    parser.add_argument('city', nargs='?', default=DEFAULT_CITY, help=f'City name (default: {DEFAULT_CITY})')  # type: ignore
    parser.add_argument('--categories', nargs='+', default=None, help='Category keys, e.g. --categories amenity leisure tourism')
    parser.add_argument('--radius', type=int, default=50000, help='Search radius in meters (default: 50000)')
    parser.add_argument('--output', type=str, default=None, help='Output gpkg filename (default: auto by categories, e.g. categories_amenity_leisure.gpkg)')
    parser.add_argument('--log', action='store_true', help='Enable detailed logging')
    args = parser.parse_args()

    query_city_categories(
        city_name=args.city,
        categories=args.categories,
        radius=args.radius,
        output_name=args.output,
        log=args.log,
    )


if __name__ == '__main__':
    main()
