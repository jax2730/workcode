import requests
if __name__ == "__main__" or __package__ is None:
    from osm_util import *
else:
    from .osm_util import *
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from constants import *
import multiprocessing

def query_road_local(country_code: str):
    save_dir = WORKING_DIR / "data" / "subway" / country_code
    relation_path = save_dir / "subway_relation.osm"
    way_path = save_dir / "subway_way.osm"
    gpkg_path = save_dir / "subway.gpkg"
    if not save_dir.exists():
        save_dir.mkdir(parents=True)

    query_relation = f"""
    [out:xml][timeout:300];
    // gather results
    area["ISO3166-1"="{country_code}"]["admin_level"="2"]->.country;
    relation["building"="北京地铁"]["type"="route"]["route"="subway"](area.country);
    out body;
    """

    query_way = f"""
    [out:xml][timeout:300][maxsize:1073741824];
    // gather results
    area["ISO3166-1"="{country_code}"]["admin_level"="2"]->.country;
    relation["network"="北京地铁"]["type"="route"]["route"="subway"](area.country);
    way["railway"="subway"];
    out geom;
    """

    p1 = multiprocessing.Process(target=query_osm_data_local, args=(query_relation, relation_path,True))
    p2 = multiprocessing.Process(target=query_osm_data_local, args=(query_way, way_path,True))
    p1.start()
    p2.start()
    p1.join()
    p2.join()
    convert_subway_osm_2_gpkg(relation_path, way_path, gpkg_path)
    print("Downloaded OSM data")

def query_road_local_bbox(bbox, save_dir):

    save_dir = WORKING_DIR / "data" / "subway" / save_dir
    if not save_dir.exists():
        save_dir.mkdir(parents=True)
    relation_path = save_dir / "roads_relation.osm"
    way_path = save_dir / "roads_way.osm"
    gpkg_path = save_dir / "roads.gpkg"
    bbox_str = f"{bbox[0]},{bbox[1]},{bbox[2]},{bbox[3]}"
    query_relation = f"""
    [out:xml][timeout:600][bbox:{bbox_str}];
    // gather results
    relation["network"="北京地铁"]["type"="route"]["route"="subway"];
    out body;
    """
    query_way = f"""
    [out:xml][timeout:600][bbox:{bbox_str}];
    // gather results
    relation["network"="北京地铁"]["type"="route"]["route"="subway"];
    >;
    way._;
    out skel geom;
    """
    query_osm_data_local(query_relation, relation_path)
    query_osm_data_local(query_way, way_path)
    convert_subway_osm_2_gpkg(relation_path, way_path, gpkg_path)
    print("Downloaded OSM data")


def process_roads_seprate_relation_way(country_code:str):
    save_dir = WORKING_DIR / "data" / "road" / country_code
    if not save_dir.exists():
        save_dir.mkdir(parents=True)
    relation_path = save_dir / "subway_relation.osm"
    way_path = save_dir / "subway_way.osm"
    gpkg_path = save_dir / "subway.gpkg"
    convert_road_osm_2_gpkg(relation_path, way_path, gpkg_path)
    print("Processed OSM data")



if __name__ == "__main__":
    #query_roads_seprate_relation_way('CN')
    query_road_local('CN')
    #query_road_local_bbox((21.8, 118.0,26.5,122.5), "TW")

    # highway_path = WORKING_DIR/"data"/"road"/"DE"/"DE_highway.osm"
    # highway_gpkg_path = WORKING_DIR/"data"/"road"/"DE"/"DE_highway.gpkg"
    # convert_osm_to_geopackage(highway_path, highway_gpkg_path)