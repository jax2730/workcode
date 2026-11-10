import json
import geopandas as gpd
import shapely

import pathlib
import sys
sys.path.append(pathlib.Path(__file__).parent.parent.absolute().as_posix())
from constants import *
DISTRICT_DIR = DATA_DIR / 'district'

PERCISION_LIST=['COUNTRY','state','district','county','town']

def get_admin_longlat(admin:gpd.GeoDataFrame)->dict:
    dict = {}
    for index, row in admin.iterrows():
        geometry = row.geometry
        if isinstance(geometry, shapely.geometry.Polygon):
            centroid = geometry.centroid
        elif isinstance(geometry, shapely.geometry.MultiPolygon):
            centroid = shapely.geometry.shape(geometry.geoms[0]).centroid
        name = row['name:en'] if 'name:en' in row and row['name:en'] else row['name']
        adcode = int(row['adcode'])
        dict[adcode] = {'name': name, 'long': centroid.x, 'lat': centroid.y}
    return dict

def get_admin_longlat_by_country(country_code:str, percision:int):
    admin_path = DISTRICT_DIR/country_code/f'{country_code}_admin_{PERCISION_LIST[percision]}.gpkg'
    admin = gpd.read_file(admin_path)
    longlat_pair = get_admin_longlat(admin)
    longlat_pair['percision'] = percision
    with open(DISTRICT_DIR/country_code/f'{country_code}_admin_longlat.json','w') as f:
        json.dump(longlat_pair, f, ensure_ascii=False, indent=4)


def merge_district_longlat(country_longlat:dict, world_admin:dict)->dict:
    world_admin_longlat = {'country_nums':world_admin['country_nums'], 'country_info':{}}
    for key, value in country_longlat.items():
        country_admin = world_admin['country_info'][str(key)]
        with open(value, 'r', encoding='utf-8') as f:
            admin_longlat = json.load(f)
            country_admin_longlat = {'name':country_admin['country_name_en'] if country_admin['country_name_en'] else country_admin['country_name'],'state_bit_count':country_admin['state_bit_count'], 'district_bit_count':country_admin['district_bit_count'], 'county_bit_count':country_admin['county_bit_count'], 'town_bit_count':country_admin['town_bit_count'], 'state_info':{}}
            for admin_key, admin_value in country_admin['state_info'].items():
                if admin_key in admin_longlat:
                    if 'percision' not in country_admin_longlat:
                        country_admin_longlat['percision'] = 1
                    elif country_admin_longlat['percision'] != 1:
                        raise Exception('country_admin_longlat["percision"] is not 1')
                    country_admin_longlat['state_info'][admin_key] = admin_longlat[admin_key]
                else:
                    country_admin_longlat['state_info'][admin_key] = {'name':admin_value["state_name:en"] if admin_value["state_name:en"] else admin_value["state_name"]\
                                                                      ,'city_nums':admin_value['city_nums'], 'city_info':{}}
                    for district_key, district_value in admin_value['city_info'].items():
                        if district_key in admin_longlat:
                            if 'percision' not in country_admin_longlat:
                                country_admin_longlat['percision'] = 2
                            elif country_admin_longlat['percision'] != 2:
                                raise Exception('country_admin_longlat["percision"] is not 2')
                            country_admin_longlat['state_info'][admin_key]['city_info'][district_key] = admin_longlat[district_key]
                        else:
                            country_admin_longlat['state_info'][admin_key]['city_info'][district_key] = {'name': district_value['city_name:en'] if district_value['city_name:en'] else district_value['city_name'],\
                                                                                                         'county_nums':district_value['county_nums'], 'county_info':{}}
                            for county_key, county_value in district_value['county_info'].items():
                                if county_key in admin_longlat:
                                    if 'percision' not in country_admin_longlat:
                                        country_admin_longlat['percision'] = 3
                                    elif country_admin_longlat['percision'] != 3:
                                        raise Exception('country_admin_longlat["percision"] is not 3')
                                    country_admin_longlat['state_info'][admin_key]['city_info'][district_key]['county_info'][county_key] = admin_longlat[county_key]
                                else:
                                    print(f'Cannot find {county_key} in admin_longlat')
            world_admin_longlat['country_info'][key] = country_admin_longlat
    return world_admin_longlat
    

if __name__ == '__main__':
    import pathlib

    # US_admin_path = pathlib.Path(__file__).parent.joinpath("./data/district/KR/KR_admin_county.geojson")
    # US_admin = gpd.read_file(US_admin_path)
    # US_admin_longlat = get_admin_longlat(US_admin)
    # with open("./out/district/KR/KR_admin_county_longlat.json", "w") as f:
    #     json.dump(US_admin_longlat, f, ensure_ascii=False, indent=4)

    get_admin_longlat_by_country('CN',2)
    get_admin_longlat_by_country('JP',1)
    get_admin_longlat_by_country('KR',3)
    get_admin_longlat_by_country('DE',2)
    get_admin_longlat_by_country('US',3)
    get_admin_longlat_by_country('TW',2)
    get_admin_longlat_by_country('FR',2)
    get_admin_longlat_by_country('IT',2)
    get_admin_longlat_by_country('ES',3)
    get_admin_longlat_by_country('GB',3)
    get_admin_longlat_by_country('GR',3)
    get_admin_longlat_by_country('FI',2)
    get_admin_longlat_by_country('PL',2)
    get_admin_longlat_by_country('NO',1)
    get_admin_longlat_by_country('SE',1)
    get_admin_longlat_by_country('DK',3)



    world_admin_path = DISTRICT_DIR/"world"/"world_admin_code.json"
    world_admin = json.load(world_admin_path.open('r', encoding='utf-8'))
    # country_longlat_pair = {
    #     1: pathlib.Path(__file__).parent.joinpath("./out/district/CN/CN_admin_district_longlat.json"),
    #     2: pathlib.Path(__file__).parent.joinpath("./out/district/JP/JP_admin_state_longlat.json"),
    #     3: pathlib.Path(__file__).parent.joinpath("./out/district/KR/KR_admin_county_longlat.json"),
    #     4: pathlib.Path(__file__).parent.joinpath("./out/district/DE/DE_admin_district_longlat.json"),
    #     5: pathlib.Path(__file__).parent.joinpath("./out/district/US/US_admin_county_longlat.json"),
    #     6: pathlib.Path(__file__).parent.joinpath("./out/district/TW/TW_admin_district_longlat.json"),
    # }
    country_longlat_pair = {}

    for key,country in world_admin['country_info'].items():
        country_code = country['country_Alpha2']
        country_longlat_pair[key] = DISTRICT_DIR/country_code/f'{country_code}_admin_longlat.json'


    merge_district_longlat = merge_district_longlat(country_longlat_pair, world_admin)
    with open(DISTRICT_DIR/"world"/"world_admin_code_longlat.json", "w") as f:
        json.dump(merge_district_longlat, f, ensure_ascii=False, indent=4)