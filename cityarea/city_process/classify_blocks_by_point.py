import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
import geopandas as gpd
import pandas as pd
import fiona

CITY_DIR = DATA_DIR / 'city'


def classify_street_blocks_by_point(city_name: str):
    """
    对每个街区，统计各 POI 类型落在街区内的点数，
    点数最多的类型作为该街区的 type 字段。
    数据来源：poi_alt.gpkg（每个 layer 为一种类型，几何为 centroid 点）
    结果写回 city_street_block_filled.gpkg（新增 type、type_count 列）。
    """
    city_dir = CITY_DIR / city_name
    street_block_path = city_dir / 'city_street_block_filled.gpkg'
    poi_path = city_dir / 'poi_alt.gpkg'

    street_block_gdf = gpd.read_file(street_block_path)
    street_block_gdf = street_block_gdf.to_crs(epsg=3857)
    street_block_gdf = street_block_gdf.reset_index(drop=True)
    street_block_gdf['_block_idx'] = street_block_gdf.index

    # 读取所有 layer，layer 名即为 POI 类型
    poi_layers = fiona.listlayers(poi_path)
    poi_gdfs = []
    for layer in poi_layers:
        gdf = gpd.read_file(poi_path, layer=layer)
        gdf['poi_type'] = layer
        poi_gdfs.append(gdf)

    poi_all = gpd.GeoDataFrame(pd.concat(poi_gdfs, ignore_index=True), geometry='geometry')
    poi_all = poi_all[poi_all.geometry.geom_type == 'Point']
    poi_all = poi_all.to_crs(epsg=3857)

    # 空间连接：点落在哪个街区内
    joined = gpd.sjoin(
        poi_all[['poi_type', 'geometry']],
        street_block_gdf[['_block_idx', 'geometry']],
        how='inner',
        predicate='within'
    )

    # 按街区 + poi_type 统计点数
    count_df = (
        joined.groupby(['_block_idx', 'poi_type'])
        .size()
        .reset_index(name='poi_count')
    )

    # 每个街区取点数最多的类型
    dominant = (
        count_df.loc[count_df.groupby('_block_idx')['poi_count'].idxmax()]
        .set_index('_block_idx')[['poi_type', 'poi_count']]
        .rename(columns={'poi_type': 'type', 'poi_count': 'type_count'})
    )

    street_block_gdf = street_block_gdf.join(dominant, on='_block_idx')
    street_block_gdf.drop(columns=['_block_idx'], inplace=True)
    street_block_gdf['type'] = street_block_gdf['type'].fillna('unknown')
    street_block_gdf['type_count'] = street_block_gdf['type_count'].fillna(0).astype(int)

    street_block_gdf.to_file(street_block_path, driver='GPKG')
    print(f"Done. {len(street_block_gdf)} blocks classified.")
    print(street_block_gdf['type'].value_counts().head(15))


if __name__ == '__main__':
    classify_street_blocks_by_point('Beijing')
