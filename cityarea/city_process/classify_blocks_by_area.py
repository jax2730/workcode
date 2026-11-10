import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
import geopandas as gpd
import pandas as pd
import osmium
import osmium.geom
import shapely.wkb as wkblib

CITY_DIR = DATA_DIR / 'city'


class POIPolygonHandler(osmium.SimpleHandler):
    """从 OSM 文件中解析 polygon/multipolygon 几何"""

    def __init__(self):
        super().__init__()
        self.polygons = []
        self.wkbfab = osmium.geom.WKBFactory()

    def way(self, w):
        if w.tags and w.is_closed():
            try:
                wkb = self.wkbfab.create_polygon(w)
                self.polygons.append(wkblib.loads(wkb, hex=True))
            except Exception:
                pass

    def area(self, a):
        if a.tags:
            try:
                wkb = self.wkbfab.create_multipolygon(a)
                self.polygons.append(wkblib.loads(wkb, hex=True))
            except Exception:
                pass


def load_poi_polygons(osm_dir: pathlib.Path) -> gpd.GeoDataFrame:
    """
    读取 osm/ 目录下所有 .osm 文件，
    返回含 poi_type 列的 polygon GeoDataFrame。
    """
    rows = []
    for osm_file in sorted(osm_dir.glob('*.osm')):
        poi_type = osm_file.stem
        handler = POIPolygonHandler()
        handler.apply_file(str(osm_file), locations=True)
        for poly in handler.polygons:
            rows.append({'poi_type': poi_type, 'geometry': poly})

    if not rows:
        return gpd.GeoDataFrame(columns=['poi_type', 'geometry'])
    return gpd.GeoDataFrame(rows, crs='EPSG:4326')


def classify_street_blocks_by_area(city_name: str):
    """
    对每个街区，计算各 POI 类型 polygon 与街区的交集面积总和，
    面积最大的类型作为该街区的 type 字段。
    数据来源：data/city/<city>/osm/*.osm（原始 OSM 文件，保留 polygon 几何）
    结果另存为 city_street_block_filled_classify.gpkg（新增 type、type_area 列）。
    """
    city_dir = CITY_DIR / city_name
    street_block_path = city_dir / 'city_street_block_filled.gpkg'
    output_path = city_dir / 'city_street_block_filled_classify.gpkg'
    osm_dir = city_dir / 'osm'

    street_block_gdf = gpd.read_file(street_block_path)
    street_block_gdf = street_block_gdf.to_crs(epsg=3857)
    street_block_gdf = street_block_gdf.reset_index(drop=True)
    street_block_gdf['_block_idx'] = street_block_gdf.index

    polygons_gdf = load_poi_polygons(osm_dir)
    if len(polygons_gdf) == 0:
        print("No POI polygon data found in osm/.")
        return

    polygons_gdf = polygons_gdf.to_crs(epsg=3857)
    polygons_gdf = polygons_gdf[
        polygons_gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])
    ].copy()

    if len(polygons_gdf) == 0:
        print("No valid polygon geometries found.")
        return

    # 空间叠加：求每个街区与 POI polygon 的交集
    overlay = gpd.overlay(
        street_block_gdf[['_block_idx', 'geometry']],
        polygons_gdf[['poi_type', 'geometry']],
        how='intersection',
        keep_geom_type=False
    )
    overlay = overlay[
        overlay.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])
    ].copy()
    overlay['intersect_area'] = overlay.geometry.area

    # 按街区 + poi_type 汇总面积
    area_sum = (
        overlay.groupby(['_block_idx', 'poi_type'])['intersect_area']
        .sum()
        .reset_index()
    )

    # 每个街区取面积最大的类型
    dominant = (
        area_sum.loc[area_sum.groupby('_block_idx')['intersect_area'].idxmax()]
        .set_index('_block_idx')[['poi_type', 'intersect_area']]
        .rename(columns={'poi_type': 'type', 'intersect_area': 'type_area'})
    )

    street_block_gdf = street_block_gdf.join(dominant, on='_block_idx')
    street_block_gdf.drop(columns=['_block_idx'], inplace=True)
    street_block_gdf['type'] = street_block_gdf['type'].fillna('unknown')
    street_block_gdf['type_area'] = street_block_gdf['type_area'].fillna(0)

    street_block_gdf.to_file(output_path, driver='GPKG')
    unknown_count = int((street_block_gdf['type'] == 'unknown').sum())
    print(f"Done. {len(street_block_gdf)} blocks classified.")
    print(f"Saved: {output_path}")
    print(f"unknown blocks: {unknown_count}")
    print(street_block_gdf['type'].value_counts().head(15))


if __name__ == '__main__':
    classify_street_blocks_by_area('Beijing')
