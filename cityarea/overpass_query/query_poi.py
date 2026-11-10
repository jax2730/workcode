if __name__ == "__main__" or __package__ is None:
    from osm_util import *
else:
    from .osm_util import *
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from constants import *
import convert_coords.convert_coords as convert_coords
import geopandas as gpd
import numpy as np
import json

MAP_OFFSET = [-2097152.0, -1048576.0]; 

def convert_poi_2_world_cord(poi_path: pathlib.Path, world_coord_path: pathlib.Path):
    poi_gpkg = gpd.read_file(poi_path)
    poi_gpkg.to_crs(epsg=3857, inplace=True)
    city_bound = poi_gpkg.total_bounds
    crs_wkt = poi_gpkg.crs.to_wkt()
    world_terrian = DATA_DIR /'world_terrain/worldmap_fill.tif'
    convert_coords.load_base_gtiff(world_terrian)
    convert_coords.load_x_y_range_data((city_bound[0],city_bound[2]), (city_bound[1],city_bound[3]),from_wkt=crs_wkt)
    convert_coords.set_origin_point(MAP_OFFSET[0],MAP_OFFSET[1])
    convert_coords.set_pixel_scale(32)

    world_coord_list = []

    for index, row in poi_gpkg.iterrows():
        row: gpd.GeoSeries
        coord = convert_coords.convert_coords_with_height(coord=np.array(row.geometry.coords.xy).reshape(2), from_wkt=crs_wkt)
        poi_dict = {'name': row['name:local'], 'coord': coord.tolist()}
        world_coord_list.append(poi_dict)

    with open(world_coord_path, 'w') as f:
        json.dump(world_coord_list, f, ensure_ascii=False,indent=4)

def query_poi(query_str:str, country_code:str):
    query = f"""
    [out:xml][timeout:25];
    area["ISO3166-1"="{country_code}"]["admin_level"="2"]->.country;
    nwr[{query_str}](area.country);
    out body;
    >;
    out skel qt;
    """

    save_dir = DATA_DIR / 'poi' / f'{country_code}'
    if not save_dir.exists():
        save_dir.mkdir(parents=True)

    query_osm_data_local(query, save_dir / f'{query_str}.osm', True)

    convert_poi_osm_2_gpkg(save_dir / f'{query_str}.osm', save_dir / f'{query_str}.gpkg','poi')
    convert_poi_2_world_cord(save_dir / f'{query_str}.gpkg', save_dir / f'{query_str}_world_coord.json')

if __name__ == "__main__":
    query_str = "aeroway=aerodrome"
    query_poi(query_str, 'CN')

