import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
import osgeo.ogr as ogr
import osgeo.osr as osr
import osgeo.gdal as gdal
import geopandas as gpd
import pandas as pd
import sknw
import numpy as np
import shapely.geometry as geom
import shapely.ops as ops
import json
import tarfile
import matplotlib.pyplot as plt
import networkx as nx
import convert_coords.convert_coords as convert_coords
import momepy

CITY_DIR = DATA_DIR / 'city'
ACTORS_DIR = DATA_DIR / 'actors'

landmark_offset = 10

MAP_OFFSET = [-2097152.0, -1048576.0]; 
WORLD_TERRAIN_PATH = DATA_DIR /'world_terrain/worldmap_fill.tif'

def get_landmark_polygons(city_name:str) -> gpd.GeoDataFrame:
    save_dir = CITY_DIR / city_name
    landmark_json_path = save_dir / 'landmark_list.json'
    landmark_info_path = save_dir / 'landmark_info.json'
    landmark_info_list = []
    landmarks = json.load(open(landmark_json_path, 'r'))
    landmark_gdf = gpd.GeoDataFrame(columns=['name', 'pos', 'type', 'geometry'],crs=3857)
    convert_coords.load_base_gtiff(WORLD_TERRAIN_PATH)
    convert_coords.set_origin_point(MAP_OFFSET[0],MAP_OFFSET[1])
    convert_coords.set_pixel_scale(32)
    for landmark in landmarks:
        coord = np.array(landmark['pos'])
        name = landmark['name']
        id = landmark['id']
        actor_path = ACTORS_DIR / f'{id}.distantbuilding'
        extends = np.array([1,1,1,1])
        if actor_path.exists():
            actor_dict = json.load(open(actor_path, 'r'))
            if 'ComponentArray' in actor_dict:
                components = actor_dict['ComponentArray']
                for component in components:
                    if component["ID"] == "rootNode":
                        if 'ExactBoundingBox' in component:
                            bbox = component['ExactBoundingBox']
                            x_min = bbox['min'][0]-landmark_offset
                            y_min = bbox['min'][2]-landmark_offset
                            x_max = bbox['max'][0]+landmark_offset
                            y_max = bbox['max'][2]+landmark_offset
                            extends = np.array([-x_min, x_max, -y_min, y_max])
                            landmark_info = {
                                "name":	f"distantbuilding/{id}/{id}.distantbuilding",
                                "id":	0,
                                "type":	-2147483648,
                                "leftExtend":	-x_min,
                                "rightExtend":	x_max,
                                "frontExtend":	y_min,
                                "backExtend":	y_max
                            }
                            landmark_info_list.append(landmark_info)
                            break
            
        coords = convert_coords.obb_2_poly(coord, extends, np.array([1,0]))
        poly = geom.Polygon(coords)
        landmark_gdf.loc[len(landmark_gdf)] = {
            'actor_index': len(landmark_gdf),
            'name': name,
            'pos': coord,
            'type': id,
            'geometry': poly
        }
    landmark_gdf.set_crs(epsg=3857, inplace=True)
    landmark_gdf.to_file(save_dir / 'landmark_polygons.geojson', driver='GeoJSON')

    with open(landmark_info_path, 'w') as f:
        json.dump(landmark_info_list, f)

if __name__ == '__main__':
    city_name = 'Beijing'
    get_landmark_polygons(city_name)
    