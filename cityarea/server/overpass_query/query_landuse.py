if __name__ == "__main__" or __package__ is None:
    from osm_util import *
else:
    from .osm_util import *
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from constants import *

CITY_DIR = DATA_DIR / "city"


def query_city_landuse(city_name: str, radius: int = 25000, log: bool = False):
    query = f"""
    [out:xml][timeout:1200][maxsize:2147483648];
    node[place="city"]["name:en"="{city_name}"];
    nwr(around:{radius})[landuse];
    out body;
    >;
    out skel qt;
    """

    save_dir = CITY_DIR / city_name
    if not save_dir.exists():
        save_dir.mkdir(parents=True)

    save_file = save_dir / "landuse.osm"
    ret = query_osm_data_local(query, save_file, log)
    if ret != 0 or osm_result_empty(save_file):
        query_osm_data_remote(query, save_file, log)

    if osm_result_empty(save_file):
        print(f"No landuse data found for {city_name}")
        return

    gpkg_file = save_dir / "landuse.gpkg"
    convert_landuse_osm_2_gpkg(save_file, gpkg_file, layer_name='landuse')
    if gpkg_file.exists():
        print(f"Saved landuse gpkg: {gpkg_file}")


def query_cities_landuse(city_names: list[str], radius: int = 25000, log: bool = False):
    for city_name in city_names:
        query_city_landuse(city_name, radius=radius, log=log)
        print(f"Finish querying landuse in {city_name}")


if __name__ == "__main__":
    city_name = "Beijing"
    query_city_landuse(city_name, radius=50000, log=True)
