import osgeo.ogr as ogr
import osgeo.osr as osr
import osgeo.gdal as gdal
import geopandas as gpd
import xml.etree.ElementTree as ET
import sys
import pathlib
import fiona
import pandas as pd
import json
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
from typing import Literal
MAP_OFFSET = [-2097152.0, -1048576.0]; 
CITY_DIR = DATA_DIR / 'city'

def get_building_cfg():
    cfg_dir = DATA_DIR / 'cfg'
    buildingTypes_path = cfg_dir / 'BuildingTypes.json'
    with open(buildingTypes_path, 'r') as f:
        buildingTypes_dict = json.load(f)
    building_list = buildingTypes_dict['cfg']
    buildingTypes = {}
    for building in building_list:
        name = building['desc']
        actor_file = building['actor']
        id = building['id']
        buildingTypes[name] = {'id': id, 'actorFile': actor_file, 'desc': name}
    
    return buildingTypes


def _resolve_building_type(layer: str, building_types: dict) -> dict:
    if layer in building_types:
        return building_types[layer]

    layer_lower = layer.lower()
    for key, value in building_types.items():
        if key.lower() == layer_lower:
            return value

    print(f"Warning: layer '{layer}' not found in BuildingTypes.json; fallback actor mapping is used.")
    return {'id': -1, 'actorFile': f'actor/{layer}.act'.lower(), 'desc': layer}

def poi_pos_convert(city:str, format: Literal['json','csv','yaml','xml'] = 'json'):
    save_dir = CITY_DIR / city
    gpkg_path = save_dir / 'poi_processed.gpkg'
    processed_path = save_dir / 'poi_point.gpkg'
    world_terrian = DATA_DIR /'world_terrain/worldmap_fill_001_001.tif'
    terrian_ds = gdal.Open(str(world_terrian))
    if terrian_ds is None:
        print("Could not open %s" % world_terrian)
        return
    x_min, x_res, x_skew, y_max, y_skew, y_res = terrian_ds.GetGeoTransform()
    terrian_crs = osr.SpatialReference()
    terrian_crs.ImportFromWkt(terrian_ds.GetProjectionRef())
    crs_wkt = terrian_crs.ExportToWkt()

    poi_dict = {}
    buildingTypes = get_building_cfg()
    for layer in fiona.listlayers(gpkg_path):
        gdf = gpd.read_file(gpkg_path, layer=layer)
        gdf.to_crs(3857, inplace=True)
        gdf_center = gdf.copy()
        gdf_center.geometry = gdf_center.geometry.centroid
        gdf_center.to_file(processed_path, layer=layer, driver='GPKG')
        gdf_center.to_crs(crs_wkt, inplace=True)
        points = []
        for index, row in gdf_center.iterrows():
            row: gpd.GeoSeries
            x = (row.geometry.x - x_min) / x_res
            y = (row.geometry.y - y_max) / y_res
            x = (x-1)*32 + MAP_OFFSET[0]
            y = (y-1)*32 + MAP_OFFSET[1]
            building_cfg = _resolve_building_type(layer, buildingTypes)
            actor_file = building_cfg['actorFile']
            id = building_cfg['id']
            dict_row = {"name": row['name:zh'] if row['name:zh'] else row['name:en'] if row['name:en'] else row['name:local'] if row['name:local'] else layer,\
                           "pos": [x, 0,y],
                           "actorFile": actor_file,'id': id}
            points.append(dict_row)
        poi_dict[layer] = {"data": points,"oriDataLen": len(points)}
    # sort poi_dict by alphabet
    poi_dict = dict(sorted(poi_dict.items()))
    match format:
        case 'json':
            import json
            with open(save_dir / 'poi_pos.json', 'w') as f:
                json.dump(poi_dict, f, indent=4, ensure_ascii=False)
        case 'csv':
            import csv
            with open(save_dir / 'poi_pos.csv', 'w', newline='') as f:
                writer = csv.writer(f)
                writer.writerow(['name', 'pos', 'actorFile'])
                for layer in poi_dict:
                    for point in poi_dict[layer]['data']:
                        writer.writerow([point['name'], point['pos'], point['actorFile']])
        case 'yaml':
            import yaml
            with open(save_dir / 'poi_pos.yaml', 'w') as f:
                yaml.dump(poi_dict, f, sort_keys=False)
        case 'xml':
            import xml.dom.minidom
            root = ET.Element('root')
            for layer in poi_dict:
                layer_elem = ET.SubElement(root, 'layer', name=layer)
                for point in poi_dict[layer]['data']:
                    point_elem = ET.SubElement(layer_elem, 'point', name=point['name'])
                    pos_elem = ET.SubElement(point_elem, 'pos')
                    pos_elem.text = str(point['pos'])
                    actor_elem = ET.SubElement(point_elem, 'actorFile')
                    actor_elem.text = point['actorFile']
            xml_str = ET.tostring(root, encoding='utf-8').decode('utf-8')
            xml_str = xml.dom.minidom.parseString(xml_str).toprettyxml(indent='    ')
            with open(save_dir / 'poi_pos.xml', 'w') as f:
                f.write(xml_str)
        case _:
            print(f"Unsupported format: {format}")



def process_poi(city:str):
    save_dir = CITY_DIR / city
    gpkg_path = save_dir / 'poi.gpkg'
    processed_path = save_dir / 'poi_processed.gpkg'
    if processed_path.exists():
        processed_path.unlink()
    for layer in fiona.listlayers(gpkg_path):
        gdf = gpd.read_file(gpkg_path, layer=layer)
        point_gdf = gdf[gdf.geometry.geom_type.isin(['Point', 'MultiPoint'])]
        line_gdf = gdf[gdf.geometry.geom_type.isin(['LineString', 'MultiLineString'])]
        polygon_gdf = gdf[gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])]
        if not polygon_gdf.empty:
            polygon_union = polygon_gdf.union_all()
            if not point_gdf.empty:
                point_gdf = point_gdf[~point_gdf.within(polygon_union)]
            if not line_gdf.empty:
                line_gdf = line_gdf[~line_gdf.within(polygon_union)]
        gdf = gpd.GeoDataFrame(pd.concat([point_gdf, line_gdf, polygon_gdf], ignore_index=True))
        gdf.to_file(processed_path, layer=layer, driver='GPKG')

if __name__ == '__main__':
    city = 'Beijing'
    process_poi(city)
    poi_pos_convert(city)

            

    
