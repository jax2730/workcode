from osm_util import query_osm_data_local
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from constants import *
import osm2geojson
import geopandas as gpd

ADMIN_DIR = DATA_DIR / 'district'

def query_admin_boundaries_by_country(country_code: str):
    
    query = f"""
    [out:xml][timeout:180];
    area["ISO3166-1"="{country_code}"][admin_level=2];
    (
      relation(area)["boundary"="administrative"];
    );
    out skel qt;
    >;
    out geom;
    """

    osm_file = ADMIN_DIR / country_code / 'admin.osm'
    gpkg_file = ADMIN_DIR / 'admin.gpkg'
    query_osm_data_local(query, osm_file)
    # with open(osm_file, 'r') as f:
    #     xml_data = f.read()
    # geojson_data = osm2geojson.json2geojson(xml_data)
    # features = geojson_data["features"]
    # reformed_features = []
    # for feature in features:
    #     geometry = feature["geometry"]
    #     geom_type = geometry["type"]
    #     if(geom_type != "Polygon" and geom_type != "MultiPolygon"):
    #         #remove feature
    #         continue
    #     properties = feature["properties"]
    #     if("tags" not in properties):
    #         continue
    #     tags = properties["tags"]
    #     new_tags = {}
    #     properties.pop("tags")
    #     if "admin_level" in tags:
    #         properties["admin_level"] = tags["admin_level"]
    #     if "name" in tags:
    #         properties["name"] = tags["name"]
    #     if "name:en" in tags:
    #         properties["name:en"] = tags["name:en"]
    #     if "name:zh-Hans" in tags:
    #         properties["name:zh"] = tags["name:zh-Hans"]
    #     elif "name:zh" in tags:
    #         properties["name:zh"] = tags["name:zh"]
    #     elif "name:zh-Hant" in tags:
    #         properties["name:zh"] = tags["name:zh-Hant"]
    #     if "place" in tags:
    #         properties["place"] = tags["place"]
    #     if "ISO3166-1:alpha2" in tags:
    #         properties["country_Alpha2"] = tags["ISO3166-1:alpha2"]
    #     elif "ISO3166-1" in tags:
    #         properties["country_Alpha2"] = tags["ISO3166-1"]
    #     properties["tags"] = new_tags
    #     feature["properties"] = properties
    #     reformed_features.append(feature)
    # geojson_data["features"] = reformed_features
    # gdf = gpd.GeoDataFrame.from_features(geojson_data)
    # gdf.to_file(gpkg_file, layer='admin', driver='GPKG')

if __name__ == '__main__':
    query_admin_boundaries_by_country('TW')

