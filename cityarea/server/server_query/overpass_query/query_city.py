if __name__ == "__main__" or __package__ is None:
    from osm_util import *
else:
    from .osm_util import *
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from constants import *
import multiprocessing
from functools import partial

CITY_DIR = DATA_DIR / "city"


def _city_anchor_query(city_name: str) -> str:
        return f"""
        (
            node[place~"city|town"]["name:en"="{city_name}"];
            node[place~"city|town"]["name"="{city_name}"];
            node[place~"city|town"]["name:zh"="{city_name}"];
        )->.city;
        """

def query_city(city_name: str, radius: int = 25000, log: bool = False):
    query = f"""
    [out:xml][timeout:1000][maxsize:2147483648];
        {_city_anchor_query(city_name)}
    nwr(around.city:{radius});
    out body;
    >;
    out skel qt;
    """
    save_dir = CITY_DIR / city_name
    if not save_dir.exists():
        save_dir.mkdir(parents=True)
    save_file = save_dir / f"city.osm"
    query_osm_data_local(query, save_file, log)
    gpkg_file = save_dir / f"city.gpkg"
    convert_city_osm_2_gpkg(save_file, gpkg_file)

def get_query_dict(radius):
    return {
        'Airport': f'nwr(around:{radius})[aeroway = "aerodrome" ]; // airport',
        'AmusementPark': f'nwr(around:{radius})[tourism = "theme_park"]; // amusement park',
        'Bank': f'nwr(around:{radius})[amenity = "bank"]; // bank',
        'CarDealer': f'nwr(around:{radius})[shop="car"]; // car dealef',
        'CoachStation': f'nwr(around:{radius})[amenity="bus_station"]; // Bus station. Notice this should be intercity bus station. but right now just ragular bus station instead.',
        'ConvienceStore': f'nwr(around:{radius})[shop="convenience"]; // convenience store',
        'Factory': f'nwr(around:{radius})[industrial ="factory"]; // factory',
        'FarmerMarket': f'nwr(around:{radius})[amenity ="marketplace"]; // farmers market',
        'GasStation': f'nwr(around:{radius})[amenity ="fuel"]; // fuel station',
        'HairSalon': f'nwr(around:{radius})[shop ="hairdresser"]; // hairdressef',
        'HomeBuildingMaterialMarket': f'(// TODO: better query method for hardware store\n\
            nwr(around:{radius})[shop ="doityourself"];\
            nwr(around:{radius})[shop ="hardware"];\
            ); // hardware store. Notice this query is not working in China due to lack of data.',
        'Hospital': f'nwr(around:{radius})[amenity ="hospital"]; // hospital',
        'Hotel': f'nwr(around:{radius})[tourism ="hotel"]; // hotel',
        'Kindergarten': f'nwr(around:{radius})[amenity ="kindergarten"]; // kindergarten',
        'KTV': f'// TODO: better query method for karaoke\n\
            nwr(around:{radius})[amenity ="bar"]; // karaoke Notice: there is no data for karaoke in OSM. use bar instead.',
        'LanzhouRamen': f'nwr(around:{radius})[cuisine ="ramen"]; // ramen shop Notice: suppose to be Lanzhou ramen shop, but there is no data for Lanzhou Ramen in OSM. use ramen shop instead.',
        'Library': f'nwr(around:{radius})[amenity ="library"]; // library',
        'McDonalds': f'nwr(around:{radius})["brand:en"="McDonald\'s"]; // McDonald\'s',
        'Museum': f'nwr(around:{radius})[tourism="museum"]; // museum',
        'OfficeBuilding': f'nwr(around:{radius})[building = "commercial"]; // office building',
        'Onsen': f'nwr(around:{radius})[leisure = "water_park"]; // Onsen Notice: there is no data for Onsen in OSM. use water park instead.',
        'Park': f'nwr(around:{radius})[leisure = "park"]; // park',
        'Port': f'nwr(around:{radius})[industrial = "port"]; // port',
        'PostOffice': f'nwr(around:50000)[amenity = "post_office"]; // post office',
        'PublicBathroom': f'node(around:50000)[amenity = "toilets"]; // public bathroom',
        'ResidentalComplex': f'nwr(around:50000)[landuse = "residential"]; // residential complex',
        'School': f'(\
            nwr(around:{radius})[amenity = "university"];\
            nwr(around:{radius})[amenity = "school"];\
            ); // school and university',
        'ShoppingMall': f'(\
            nwr(around:{radius})[landuse="retail"];\
            nwr(around:{radius})[shop="mall"];\
                ); // shopping mall',
        'Stadium': f'nwr(around:{radius})[leisure = "stadium"]; // stadium',
        'TrainStation': f'nwr(around:{radius})[train][railway="station"]; // train station',
        'ViewSpot': f'nwr(around:{radius})[tourism="attraction"]; // tourist attraction',
        'Zoo': f'nwr(around:{radius})[tourism="zoo"]; // zoo'
    }

def query_poi_by_type_worker(poi_type:str,city_name:str,radius:int=25000,log:bool=False):
    save_dir = CITY_DIR / city_name
    osm_dir = save_dir / "osm"
    query_dict = get_query_dict(radius)
    query_body = query_dict[poi_type].replace(f"around:{radius}", f"around.city:{radius}")
    query = f"""
    [out:xml][timeout:1000][maxsize:2147483648];
    {_city_anchor_query(city_name)}
    (
    {query_body}
    );
    out body;
    >;
    out skel qt;
    """
    save_file = osm_dir / f"{poi_type}.osm"
    query_osm_data_local(query, save_file, log)
    print(f"Finish querying {poi_type} in {city_name}")
    return poi_type

def query_cities_poi(city_name: str, radius: int = 25000, log: bool = False):
    save_dir = CITY_DIR / city_name
    osm_dir = save_dir / "osm"
    gpkg_path = save_dir / "poi.gpkg"
    if gpkg_path.exists():
        gpkg_path.unlink()
    if not osm_dir.exists():
        osm_dir.mkdir(parents=True)
    query_dict = get_query_dict(radius)

    worker_func = partial(query_poi_by_type_worker,city_name=city_name,radius=radius,log=log)
    
    with multiprocessing.Pool(processes=4) as pool:
        poi_types = pool.map(worker_func, query_dict.keys())

    for poi_type in poi_types:
        save_file = osm_dir / f"{poi_type}.osm"
        convert_poi_osm_2_gpkg(save_file, gpkg_path, poi_type)
        print(f"Finish converting {poi_type} in {city_name}")

    # for type in query_dict:
    #     query_poi_by_type(type)


if __name__ == "__main__":
    city_name = "Beijing"
    # query_city(city_name,log=True)
    query_cities_poi(city_name,log=True)
