import geopandas as gpd
import pandas as pd

import pathlib
import sys
sys.path.append(pathlib.Path(__file__).parent.parent.absolute().as_posix())
from constants import *
from . import braekdown_lines

ROAD_DIR = DATA_DIR / 'road'

def filter_highways(input_file: str, output_file:str, highway_type = ['primary','motorway']):
    """
    Filter highways from a geopackage and save the result as a new geopackage.

    Parameters:
    input_file (str): Path to the input geopackage.
    output_file (str): Path to the output geopackage.

    Returns:
    None
    """
    # Read the input shapefile
    gdf = gpd.read_file(input_file,layer='road')

    # Filter highways
    highways = gdf[gdf['highway'].notnull()]
    highways = highways[highways['highway'].isin(highway_type)]
    # Save the result as a new shapefile
    highways.to_file(output_file, driver='GPKG')

def filter_highways_by_length(input_file: str, output_file:str, min_length: float = 25000):
    """
    Filter highways from a geopackage and save the result as a new geopackage.

    Parameters:
    input_file (str): Path to the input geopackage.
    output_file (str): Path to the output geopackage.
    min_length (float): Minimum length in meters of highways to be kept.

    Returns:
    None
    """
    # Read the input shapefile
    gdf = gpd.read_file(input_file)
    # reprojec the geometries to projected WKT
    gdf = gdf.to_crs(PROJECTED_WKT_STR)
    # Filter highways by length
    highways = gdf[gdf.geometry.length > min_length]
    #projet the geometries back to WGS84
    highways = highways.to_crs(epsg=4326)
    # Save the result as a new shapefile
    highways.to_file(output_file, driver='GPKG')



def filter_highways_country(country_code:str, highway_type = ['primary','motorway'], min_length = 25000):
    country_dir = ROAD_DIR / country_code
    filter_highways(str(country_dir / 'roads.gpkg'), str(country_dir / 'roads_filtered.gpkg'), highway_type = highway_type)
    filter_highways_by_length(str(country_dir / 'roads_filtered.gpkg'), str(country_dir / 'roads_filtered.gpkg'), min_length=25000)
    road_path = country_dir/'roads_filtered.gpkg'
    road_gdf = gpd.read_file(road_path, encoding='utf-8')
    road_gdf = braekdown_lines.remove_close_lines(road_gdf)
    road_gdf.to_file(road_path, driver='GPKG')
    print('Filtered highways saved to', country_dir / 'roads_filtered.gpkg')

if __name__ == '__main__':
    filter_highways_country('DE')
    

    