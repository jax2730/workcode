import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
import geopandas as gpd
import pandas as pd

CITY_DIR = DATA_DIR / 'city'
ROAD_DIR = DATA_DIR / 'road'

def filter_ways(city_name:str, ways_gdf:gpd.GeoDataFrame):
    """
    filter out ways that are in the city sector
    """
    city_sector_path = CITY_DIR / city_name /'city_sector.gpkg'
    city_sector_gdf = gpd.read_file(city_sector_path, layer='lvl_0')
    ways_gdf = ways_gdf.to_crs(city_sector_gdf.crs)

    # filter out ways that are in the city sector
    filtered_ways_gdf = ways_gdf[~ways_gdf.intersects(city_sector_gdf.union_all())]

    return filtered_ways_gdf

if __name__ == '__main__':
    city_name = 'Beijing'
    ways_gdf = gpd.read_file(ROAD_DIR/'world'/'road_trimmed.gpkg')
    filtered_ways_gdf = filter_ways(city_name, ways_gdf)
    (ROAD_DIR/'world'/'road_filtered.gpkg').unlink(missing_ok=True)
    filtered_ways_gdf.to_file(ROAD_DIR/'world'/'road_filtered.gpkg', driver='GPKG')

