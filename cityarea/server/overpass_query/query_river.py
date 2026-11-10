if __name__ == "__main__" or __package__ is None:
    from osm_util import  *
else:
    from .osm_util import *
import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *

def query_river_by_country(country_code:str):
    river_dir = DATA_DIR / 'river'
    save_dir = river_dir / country_code
    if not save_dir.exists():
        save_dir.mkdir(parents=True)
    query_relation =f"""
    [out:xml][timeout:180];
    area["ISO3166-1"="{country_code}"]["admin_level"="2"]->.country;
    relation["waterway"~"^(river|canal)$"](area.country);
    out body;
    """
    query_way = f"""
    [out:xml][timeout:180];
    area["ISO3166-1"="{country_code}"]["admin_level"="2"]->.country;
    relation["waterway"~"^(river|canal)$"](area.country);
    >;
    way._;
    out skel geom;
    """

    query_way_alt = f"""
    [out:xml][timeout:600];
    area["ISO3166-1"="{country_code}"]["admin_level"="2"]->.country;
    way["waterway"~"(river|canal)"]["name"~"."](area.country);
    out body geom;
    """

    #query_osm_data_remote(query_relation, save_dir / 'osm_relation.osm',True)
    #query_osm_data_remote(query_way, save_dir / 'osm_way.osm',True)
    query_osm_data_local(query_way_alt, save_dir / 'osm_way.osm',True)
    #convert_river_osm_2_gpkg(save_dir / 'osm_relation.osm', save_dir / 'osm_way.osm', save_dir / 'river.gpkg')
    convert_river_osm_2_gpkg_alt(save_dir / 'osm_way.osm', save_dir / 'river.gpkg')

if __name__ == "__main__":
    query_river_by_country('KR')
