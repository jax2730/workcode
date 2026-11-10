import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
import geopandas as gpd
import pandas as pd
import osmium
import osmium.geom
import shapely.wkb as wkblib
import shapely.geometry as geom

CITY_DIR = DATA_DIR / 'city'


class POIAreaHandler(osmium.SimpleHandler):
    """解析 OSM 文件，分别收集 point 和 polygon 几何"""

    def __init__(self):
        super().__init__()
        self.points = []
        self.polygons = []
        self.wkbfab = osmium.geom.WKBFactory()

    def node(self, n):
        if n.tags:
            try:
                wkb = self.wkbfab.create_point(n)
                self.points.append(wkblib.loads(wkb, hex=True))
            except Exception:
                pass

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


def load_poi_from_osm(osm_dir: pathlib.Path) -> tuple[gpd.GeoDataFrame, gpd.GeoDataFrame]:
    """
    读取 osm/ 目录下所有 .osm 文件，
    返回 (points_gdf, polygons_gdf)，均含 poi_type 列。
    """
    point_rows = []
    polygon_rows = []

    for osm_file in sorted(osm_dir.glob('*.osm')):
        poi_type = osm_file.stem
        handler = POIAreaHandler()
        handler.apply_file(str(osm_file), locations=True)

        for pt in handler.points:
            point_rows.append({'poi_type': poi_type, 'geometry': pt})
        for poly in handler.polygons:
            polygon_rows.append({'poi_type': poi_type, 'geometry': poly})

    points_gdf = gpd.GeoDataFrame(point_rows, crs='EPSG:4326') if point_rows else \
        gpd.GeoDataFrame(columns=['poi_type', 'geometry'])
    polygons_gdf = gpd.GeoDataFrame(polygon_rows, crs='EPSG:4326') if polygon_rows else \
        gpd.GeoDataFrame(columns=['poi_type', 'geometry'])

    return points_gdf, polygons_gdf


def classify_street_blocks(city_name: str):
    """
    对每个街区，综合 POI 点数和 POI 面积两种方式打分，
    得分最高的类型作为该街区的 type 字段。

    面积得分：该类型 polygon 与街区的交集面积之和（单位 m²）
    点数得分：该类型 point 落在街区内的数量 × 街区面积均值（归一化到同量级）
    最终得分 = 面积得分 + 点数得分（归一化后相加）
    """
    city_dir = CITY_DIR / city_name
    street_block_path = city_dir / 'city_street_block_filled.gpkg'
    osm_dir = city_dir / 'osm'

    street_block_gdf = gpd.read_file(street_block_path)
    street_block_gdf = street_block_gdf.to_crs(epsg=3857)
    street_block_gdf = street_block_gdf.reset_index(drop=True)
    street_block_gdf['_block_idx'] = street_block_gdf.index
    mean_block_area = street_block_gdf.geometry.area.mean()

    points_gdf, polygons_gdf = load_poi_from_osm(osm_dir)
    points_gdf = points_gdf.to_crs(epsg=3857)
    polygons_gdf = polygons_gdf.to_crs(epsg=3857)

    score_frames = []

    # --- 面积得分 ---
    if len(polygons_gdf) > 0:
        poly_only = polygons_gdf[
            polygons_gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])
        ].copy()
        if len(poly_only) > 0:
            overlay = gpd.overlay(
                street_block_gdf[['_block_idx', 'geometry']],
                poly_only[['poi_type', 'geometry']],
                how='intersection',
                keep_geom_type=False
            )
            overlay = overlay[
                overlay.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])
            ].copy()
            overlay['area_score'] = overlay.geometry.area
            area_score = (
                overlay.groupby(['_block_idx', 'poi_type'])['area_score']
                .sum()
                .reset_index()
            )
            score_frames.append(area_score.rename(columns={'area_score': 'score'}))

    # --- 点数得分（点数 × 街区平均面积，量纲对齐）---
    if len(points_gdf) > 0:
        pt_only = points_gdf[
            points_gdf.geometry.geom_type == 'Point'
        ].copy()
        if len(pt_only) > 0:
            joined = gpd.sjoin(
                pt_only[['poi_type', 'geometry']],
                street_block_gdf[['_block_idx', 'geometry']],
                how='inner',
                predicate='within'
            )
            count_df = (
                joined.groupby(['_block_idx', 'poi_type'])
                .size()
                .reset_index(name='count')
            )
            count_df['score'] = count_df['count'] * mean_block_area
            score_frames.append(count_df[['_block_idx', 'poi_type', 'score']])

    if not score_frames:
        print("No POI data found.")
        return

    # 合并两种得分
    all_scores = pd.concat(score_frames, ignore_index=True)
    total_scores = (
        all_scores.groupby(['_block_idx', 'poi_type'])['score']
        .sum()
        .reset_index()
    )

    # 每个街区取得分最高的类型
    dominant = (
        total_scores.loc[total_scores.groupby('_block_idx')['score'].idxmax()]
        .set_index('_block_idx')[['poi_type', 'score']]
        .rename(columns={'poi_type': 'type', 'score': 'type_score'})
    )

    street_block_gdf = street_block_gdf.join(dominant, on='_block_idx')
    street_block_gdf.drop(columns=['_block_idx'], inplace=True)
    street_block_gdf['type'] = street_block_gdf['type'].fillna('unknown')
    street_block_gdf['type_score'] = street_block_gdf['type_score'].fillna(0)

    street_block_gdf.to_file(street_block_path, driver='GPKG')
    print(f"Done. {len(street_block_gdf)} blocks classified.")
    print(street_block_gdf['type'].value_counts().head(15))


if __name__ == '__main__':
    classify_street_blocks('Beijing')
