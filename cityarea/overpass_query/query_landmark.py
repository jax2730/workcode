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

def query_landmark(landmark_list: list[str], city_name:str, country_code:str):

    save_dir = DATA_DIR / 'city' / city_name / 'landmarks'
    if not save_dir.exists():
        save_dir.mkdir(parents=True)

    for landmark_name in landmark_list:
        query = f"""
            [out:xml][timeout:25];
            area["ISO3166-1" = "{country_code}"]->.country;
            node["place"= "city"]["name:en" = "{city_name}"](area.country)->.city;
            nwr["name" ~"{landmark_name}"](around.city:50000);
            out body;
            >;
            out skel qt;
        """
        print(query)
        query_osm_data_local(query, save_dir / f'{landmark_name}.osm', True)

        convert_poi_osm_2_gpkg(save_dir / f'{landmark_name}.osm', save_dir / f'{landmark_name}.gpkg','landmark')
    
    landmarks_gdf = gpd.GeoDataFrame()

    for landmark_name in landmark_list:
        if not (save_dir / f'{landmark_name}.gpkg').exists():
            continue
        landmark_gdf = gpd.read_file(save_dir / f'{landmark_name}.gpkg', layer='landmark')
        if len(landmarks_gdf) == 0:
            landmarks_gdf = landmark_gdf
        else:
            landmarks_gdf = pd.concat([landmarks_gdf, landmark_gdf])

    landmarks_gdf.to_file(save_dir / f'landmarks.gpkg', layer='landmarks', driver='GPKG')

def convert_landmarks_2_world_cord(landmarks_path: pathlib.Path, world_coord_path: pathlib.Path):
    landmarks_gpkg = gpd.read_file(landmarks_path)
    landmarks_gpkg.to_crs(epsg=3857, inplace=True)
    landmarks_gpkg = landmarks_gpkg[landmarks_gpkg.geometry.notnull()]
    city_bound = landmarks_gpkg.total_bounds
    crs_wkt = landmarks_gpkg.crs.to_wkt()
    world_terrian = DATA_DIR /'world_terrain/worldmap_fill.tif'
    convert_coords.load_base_gtiff(world_terrian)
    convert_coords.load_x_y_range_data((city_bound[0],city_bound[2]), (city_bound[1],city_bound[3]),from_wkt=crs_wkt)
    convert_coords.set_origin_point(MAP_OFFSET[0],MAP_OFFSET[1])
    convert_coords.set_pixel_scale(32)

    world_coord_list = []

    for index, row in landmarks_gpkg.iterrows():
        row: gpd.GeoSeries
        coord = convert_coords.convert_coords_with_height(coord=np.array(row.geometry.coords.xy).reshape(2), from_wkt=crs_wkt)
        landmarks_dict = {'name': row['name:zh'] if 'name:zh' in row.index and not pd.isna(row['name:zh'])\
                           else row['name:en'] if 'name:en' in row.index and not pd.isna(row['name:en'])\
                           else row['name:local'],
                    'pos': coord.tolist()}
        world_coord_list.append(landmarks_dict)

    with open(world_coord_path, 'w') as f:
        json.dump(world_coord_list, f, ensure_ascii=False,indent=4)

def landmarks_2_json(city_name:str, country_code:str):
    save_dir = DATA_DIR / 'city' / city_name / 'landmarks'
    if not save_dir.exists():
        raise FileNotFoundError(f"{save_dir} not found")
    
    convert_landmarks_2_world_cord(save_dir / f'landmarks.gpkg', save_dir / f'landmarks_world_coord.json')

if __name__ == "__main__":
    city_name = 'Beijing'
    country_code = 'CN'
    landmark_list = [
        "中央电视台",
        "中国尊",
        "北京奥林匹克塔",
        "水立方",
        "中央电视塔",
        "鸟巢"
    ]

    # query_landmark(landmark_list, city_name,country_code)

    landmarks_2_json(city_name, country_code)