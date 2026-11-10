import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
from road_process import road_util, simplify_road as sr
import overpass_query.query_poi as query_poi
import convert_coords.convert_coords as convert_coords

import geopandas as gpd
import pandas as pd
import numpy as np
import shapely.geometry as geom
import shapely.ops as ops
import networkx as nx
import matplotlib.pyplot as plt
import math
import json
import fiona
import momepy
import argparse

def create_page_polygon(city_path: pathlib.Path, output_path: pathlib.Path, page_size: float):
    city_gdf = gpd.read_file(city_path)
    city_gdf = city_gdf.to_crs(epsg=3857)

    city_bound_box = city_gdf.total_bounds

    city_bound_ingame = convert_coords.convert_coords_list(np.array([(city_bound_box[0], city_bound_box[1]), (city_bound_box[2], city_bound_box[3])]), from_wkt=city_gdf.crs.to_wkt())

    city_center_ingame = (city_bound_ingame[0] + city_bound_ingame[1]) / 2

    city_half_width_ingame = abs((city_bound_ingame[1][0] - city_bound_ingame[0][0]) / 2)
    city_half_length_ingame = abs((city_bound_ingame[1][1] - city_bound_ingame[0][1]) / 2)

    page_rows = int(math.ceil(city_half_width_ingame * 2 / page_size))
    page_cols = int(math.ceil(city_half_length_ingame * 2 / page_size))

    polygons_gdf = gpd.GeoDataFrame(columns=['geometry', 'page_id', 'page_row', 'page_col'],crs=city_gdf.crs)

    for i in range(page_rows):
        for j in range(page_cols):
            page_id = i * page_cols + j
            page_row = i
            page_col = j

            page_center = (city_center_ingame[0] - city_half_width_ingame + j * page_size + page_size / 2, city_center_ingame[1] - city_half_length_ingame + i * page_size + page_size / 2)

            page_left = page_center[0] - page_size / 2
            page_right = page_center[0] + page_size / 2
            page_bottom = page_center[1] - page_size / 2
            page_top = page_center[1] + page_size / 2
            
            ingame_2_gis = lambda x,y: convert_coords.convert_2_gis_coords(x=x, y=y, to_epsg=3857)
            page_polygon = geom.Polygon([ingame_2_gis(page_left, page_bottom), ingame_2_gis(page_right, page_bottom), ingame_2_gis(page_right, page_top), ingame_2_gis(page_left, page_top)])
            page_gdf = gpd.GeoDataFrame({'geometry': [page_polygon]}, crs=city_gdf.crs)

            page_gdf['page_id'] = page_id
            page_gdf['page_row'] = page_row
            page_gdf['page_col'] = page_col

            polygons_gdf = pd.concat([polygons_gdf, page_gdf], ignore_index=True)

    polygons_gdf.to_file(output_path)

def match_poi_to_polygon(poi_path:pathlib.Path, polygon_path:pathlib.Path, output_path:pathlib.Path, page_size:float):
    
    poi_gdf = gpd.read_file(poi_path,layer='poi')
    polygon_gdf = gpd.read_file(polygon_path)

    poi_gdf = poi_gdf.to_crs(polygon_gdf.crs)

    joined_gdf = gpd.sjoin(poi_gdf, polygon_gdf, how='left', predicate='intersects')

    grouped_gdf = joined_gdf.groupby('page_id')

    pages_list = []
    city_bound = polygon_gdf.total_bounds

    # world_terrian = DATA_DIR /'world_terrain/worldmap_fill.tif'
    # convert_coords.load_base_gtiff(world_terrian)
    
    crs_wkt = polygon_gdf.crs.to_wkt()
    # convert_coords.load_x_y_range_data((city_bound[0],city_bound[2]), (city_bound[1],city_bound[3]),from_wkt=crs_wkt)
    # convert_coords.set_origin_point(MAP_OFFSET[0],MAP_OFFSET[1])
    # convert_coords.set_pixel_scale(32)

    city_center = (city_bound[0] + city_bound[2]) / 2, (city_bound[1] + city_bound[3]) / 2
    city_pos = convert_coords.convert_coords(coord=np.array([city_center[0], city_center[1]]), from_wkt=crs_wkt)

    for index, row in polygon_gdf.iterrows():
        page_id = row['page_id']
        page_row = row['page_row']
        page_col = row['page_col']
        page_geom = row['geometry']

        if page_id not in grouped_gdf.groups:
            continue

        group = grouped_gdf.get_group(page_id)

        group.to_file(output_path.parent / f'{output_path.stem}.gpkg', layer=f'page_{page_id}', driver='GPKG')
        page_center = page_geom.centroid
        page_pose = convert_coords.convert_coords(coord=np.array([page_center.x, page_center.y]), from_wkt=crs_wkt)
        page_relative = np.array([page_pose[0] - city_pos[0], page_pose[1] - city_pos[1]])
        poi_list = []
        for index, poi_row in group.iterrows():
            poi_geom:geom.Point = poi_row['geometry']
            poi_pos = convert_coords.convert_coords(coord=np.array([poi_geom.x, poi_geom.y]), from_wkt=crs_wkt)
            poi_relative = np.array([poi_pos[0] - page_pose[0], poi_pos[1] - page_pose[1]])
            poi_dict = {
                'name': poi_row['name'],
                'type': poi_row['type'],
                'pos': poi_pos.tolist(),
                'relative_pos': poi_relative.tolist()
            }
            poi_list.append(poi_dict)

        
        polygon_outer = geom.LineString(page_geom.exterior.coords)
        polygon_outer = convert_coords.convert_linestring(polygon_outer, from_wkt=crs_wkt)
        polygon_outer_relative = np.array([polygon_outer[:,0] - page_pose[0], polygon_outer[:,1] - page_pose[1]]).T
        page_relative = np.round(page_relative).astype(int)
        page_pose = np.round(page_pose).astype(int)
        polygon_outer_relative = np.round(polygon_outer_relative).astype(int)
        page_dict = {
            'id': int(page_id),
            'row': int(page_row),
            'col': int(page_col),
            'pos': page_pose.tolist(),
            'relative_pos': page_relative.tolist(),
            'poi_count': len(poi_list),
            'pois': poi_list,
            'width': page_size,
            # 'bound': polygon_outer_relative.tolist(),
        }
        
        pages_list.append(page_dict)

    city_dict = {
        'city_name': poi_path.parent.name,
        'pos': city_pos.tolist(),
        'pages': pages_list
    }

    
    with open(output_path.parent / f'{output_path.stem}.json', 'w') as f:
        json.dump(city_dict, f, indent=4, ensure_ascii=False)

if __name__ == '__main__':

    city_path = DATA_DIR/'city'/'Beijing'/'city_sector.gpkg'
    output_path = DATA_DIR/'city'/'Beijing'/'page_polygons.gpkg'

    
    world_terrian = DATA_DIR /'world_terrain/worldmap_fill.tif'
    convert_coords.load_base_gtiff(world_terrian)
    
    # crs_wkt = polygon_gdf.crs.to_wkt()
    # convert_coords.load_x_y_range_data((city_bound[0],city_bound[2]), (city_bound[1],city_bound[3]),from_wkt=crs_wkt)
    convert_coords.set_origin_point(MAP_OFFSET[0],MAP_OFFSET[1])
    convert_coords.set_pixel_scale(32)
    

    create_page_polygon(city_path, output_path, 256)
    query_poi.query_poi_around_city('Beijing', radius=25000, log=False)
    match_poi_to_polygon(DATA_DIR/'city'/'Beijing'/'poi.gpkg', DATA_DIR/'city'/'Beijing'/'page_polygons.gpkg', DATA_DIR/'city'/'Beijing'/'page_pois.gpkg',256)