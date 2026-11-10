import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from overpass_query import query_landmark
import argparse
import logging
import time
import json
import geopandas as gpd
import pandas as pd
import shapely.geometry as geom
import convert_coords.convert_coords as convert_coords
import numpy as np
from constants import *

MAP_OFFSET = [-2097152.0, -1048576.0]; 
WORLD_TERRAIN_PATH = DATA_DIR /'world_terrain/worldmap_fill.tif'
ACTORS_DIR = DATA_DIR / 'actors'
landmark_offset = 10
def create_landmark_poly(landmark_point_gpkg: pathlib.Path, landmark_file: pathlib.Path):
    landmark_point_gdf = gpd.read_file(landmark_point_gpkg)
    landmark_poly_gdf = gpd.GeoDataFrame(columns=landmark_point_gdf.columns, crs=landmark_point_gdf.crs)
    convert_coords.load_base_gtiff(WORLD_TERRAIN_PATH)
    convert_coords.set_origin_point(MAP_OFFSET[0],MAP_OFFSET[1])
    convert_coords.set_pixel_scale(32)
    landmark_info_path = landmark_point_gpkg.parent.parent / 'landmark_info.json'
    landmark_info_list = []
    with open(landmark_file, 'r') as f:
        landmarks = json.load(f)
    for index, row in landmark_point_gdf.iterrows():
        scene_coord = convert_coords.convert_coords(x=row['geometry'].x, y=row['geometry'].y, from_wkt=landmark_point_gdf.crs.to_wkt())
        if row['name:en'] in landmarks:
            landmark = landmarks[row['name:en']]
        else:
            for key, lm in landmarks.items():
                if 'query' in lm:
                    query_field = next(iter(lm['query']))
                    query_name = lm['query'][query_field]
                    query_field = 'name:local' if query_field == 'name' else query_field
                    if query_field in row and query_name == row[query_field]:
                        landmark = lm
                        break
            else:
                continue
        x_min = 100
        y_min = 100
        x_max = -100
        y_max = -100
        if 'ExactBoundingBox' in landmark:
            bbox = landmark['ExactBoundingBox']
            x_min = bbox['min'][0]-landmark_offset
            y_min = bbox['min'][2]-landmark_offset
            x_max = bbox['max'][0]+landmark_offset
            y_max = bbox['max'][2]+landmark_offset
        else:
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
                                break
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
        extends = np.array([-x_min, x_max, -y_min, y_max])
        coords = convert_coords.obb_2_poly(scene_coord, extends, np.array([1,0]))
        poly = geom.Polygon(coords)
        new_row = row.copy()
        new_row['geometry'] = poly
        landmark_poly_gdf.loc[len(landmark_poly_gdf)] = new_row
    
    landmark_poly_gdf.set_crs(epsg=3857, inplace=True) if len(landmark_poly_gdf) > 0 else None
    landmark_poly_gdf.to_file(landmark_point_gpkg.parent / f'landmarks_poly.gpkg', layer='landmarks', driver='GPKG')

    with open(landmark_info_path, 'w') as f:
        json.dump(landmark_info_list, f)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Query city data from OpenStreetMap')
    parser.add_argument('-c','--city_name', type=str, help='City name')
    parser.add_argument('-f','--landmark_file', type=str, default='landmarks.json', help='Landmark file')
    parser.add_argument('--log', action='store_true', default=False, help='Log the output')

    args = parser.parse_args()
    city_name = args.city_name
    landmark_file = DATA_DIR / 'city' / city_name / args.landmark_file

    with open(landmark_file, 'r') as f:
        landmarks:dict = json.load(f)
    print(landmarks)
    landmark_names = [key for key, lm in landmarks.items()]
    query_landmark.query_landmark_dict(landmarks, city_name)
    create_landmark_poly(DATA_DIR / 'city' / city_name / 'landmarks' / 'landmarks.gpkg', landmark_file)


