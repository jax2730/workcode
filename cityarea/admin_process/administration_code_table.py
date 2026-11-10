import geopandas as gpd
import pandas as pd
import json
import pathlib


import pathlib
import sys
sys.path.append(pathlib.Path(__file__).parent.parent.absolute().as_posix())
from constants import *
DISTRICT_DIR = DATA_DIR / 'district'

def encode_district_code(gdf_town: gpd.GeoDataFrame, gdf_county: gpd.GeoDataFrame, gdf_district: gpd.GeoDataFrame, gdf_state: gpd.GeoDataFrame,\
                        state_bit_count: int, district_bit_count: int, county_bit_count: int, town_bit_count: int,\
                        country_code: int, country_name: str, country_name_en: str = None, country_name_zh: str = None) -> json:
    code_dict = dict()

    state_bit_count_act = state_bit_count
    state_bit_count = 25 - district_bit_count - county_bit_count - town_bit_count
    code_dict = {}
    for index, row in gdf_state.iterrows():
        state_code = row['state_id']
        state_mask = (1<<state_bit_count_act)-1
        state_code= int(state_code)&state_mask
        #check if the state code is valid
        if state_code < 1:
            continue
        state_adcode = country_code << state_bit_count | state_code
        state_adcode = int(state_adcode)
        if state_adcode not in code_dict:
            state_name_local = row['name']
            state_name_en = row['name:en']
            state_name_zh = row['name:zh']
            state_name = state_name_zh if state_name_zh is not None else state_name_en if state_name_en is not None else state_name_local
            city_count = row['child_count']
            code_dict[state_adcode] = {'state_name': state_name, 'state_name:en': state_name_en, 'state_name:zh': state_name_zh, 'state_name:local': state_name_local, 'city_nums': int(city_count), 'city_info': {}}
        gdf_state.loc[index, 'adcode'] = state_adcode

    for index, row in gdf_district.iterrows():
        state_code = row['state_id']
        state_mask = (1<<state_bit_count_act)-1
        state_code= int(state_code)&state_mask
        #check if the state code is valid
        if state_code < 1:
            continue
        state_adcode = country_code << state_bit_count | state_code
        if state_adcode not in code_dict:
            gdf_district.loc[index, 'adcode'] = 0
            continue
        city_mask = (1<<district_bit_count)-1
        city_code = int(row['district_id']) & city_mask
        city_adcode = state_adcode << district_bit_count | city_code
        city_adcode = int(city_adcode)
        if city_adcode not in code_dict[state_adcode]['city_info']:
            city_name_local = row['name']
            city_name_en = row['name:en']
            city_name_zh = row['name:zh']
            city_name = city_name_zh if city_name_zh is not None else city_name_en if city_name_en is not None else city_name_local
            county_count = row['child_count']
            code_dict[state_adcode]['city_info'][city_adcode] = {'city_name': city_name, 'city_name:en': city_name_en, 'city_name:zh': city_name_zh, 'city_name:local': city_name_local, 'county_nums': int(county_count), 'county_info': {}}
        gdf_district.loc[index, 'adcode'] = city_adcode

    for index, row in gdf_county.iterrows():
        state_code = row['state_id']
        if state_code is None:
            continue
        state_mask = (1<<state_bit_count_act)-1
        state_code= int(state_code)&state_mask
        #check if the state code is valid
        if state_code < 1:
            continue
        state_adcode = country_code << state_bit_count | state_code
        if state_adcode not in code_dict:
            gdf_county.loc[index, 'adcode'] = 0
            continue
        city_mask = (1<<district_bit_count)-1
        city_code = int(row['district_id']) & city_mask
        city_adcode = state_adcode << district_bit_count | city_code
        if city_adcode not in code_dict[state_adcode]['city_info']:
            gdf_county.loc[index, 'adcode'] = 0
            continue
        country_mask = (1<<county_bit_count)-1
        county_code = int(row['county_id'])&country_mask
        county_adcode = city_adcode << county_bit_count | county_code
        county_adcode = int(county_adcode)
        if county_adcode not in code_dict[state_adcode]['city_info'][city_adcode]['county_info']:
            county_name_local = row['name']
            county_name_en = row['name:en']
            county_name_zh = row['name:zh']
            county_name = county_name_zh if county_name_zh is not None else county_name_en if county_name_en is not None else county_name_local
            town_count = row['child_count']
            code_dict[state_adcode]['city_info'][city_adcode]['county_info'][county_adcode] = {'county_name': county_name, 'county_name:en': county_name_en, 'county_name:zh': county_name_zh, 'county_name:local': county_name_local, 'town_nums': int(town_count), 'town_info': {}}
        gdf_county.loc[index, 'adcode'] = county_adcode

    for index, row in gdf_town.iterrows():
        state_code = row['state_id']
        state_mask = (1<<state_bit_count_act)-1
        state_code= int(state_code)&state_mask
        #check if the state code is valid
        if state_code < 1:
            continue
        state_adcode = country_code << state_bit_count | state_code
        if state_adcode not in code_dict:
            gdf_town.loc[index, 'adcode'] = 0
            continue
        city_mask = (1<<district_bit_count)-1
        city_code = int(row['district_id']) & city_mask
        city_adcode = state_adcode << district_bit_count | city_code
        if city_adcode not in code_dict[state_adcode]['city_info']:
            gdf_town.loc[index, 'adcode'] = 0
            continue
        country_mask = (1<<county_bit_count)-1
        county_code = int(row['county_id'])&country_mask
        county_adcode = city_adcode << county_bit_count | county_code
        if county_adcode not in code_dict[state_adcode]['city_info'][city_adcode]['county_info']:
            gdf_town.loc[index, 'adcode'] = 0
            continue
        town_mask = (1<<town_bit_count)-1
        town_code = int(row['town_id'])&town_mask
        town_adcode = county_adcode << town_bit_count | town_code
        town_adcode = int(town_adcode)
        if town_adcode not in code_dict[state_adcode]['city_info'][city_adcode]['county_info'][county_adcode]['town_info']:
            town_name_local = row['name']
            town_name_en = row['name:en']
            town_name_zh = row['name:zh']
            town_name = town_name_zh if town_name_zh is not None else town_name_en if town_name_en is not None else town_name_local
            code_dict[state_adcode]['city_info'][city_adcode]['county_info'][county_adcode]['town_info'][town_adcode] = {'town_name': town_name, 'town_name:en': town_name_en, 'town_name:zh': town_name_zh, 'town_name:local': town_name_local}
        gdf_town.loc[index, 'adcode'] = town_adcode
    # recursively sort the code_dict by key
    # for state_code in code_dict:
    #     for city_code in code_dict[state_code]['city_info']:
    #         for county_code in code_dict[state_code]['city_info'][city_code]['county_info']:
    #             town_dict: dict = code_dict[state_code]['city_info'][city_code]['county_info'][county_code]['town_info']
    #             code_dict[state_code]['city_info'][city_code]['county_info'][county_code]['town_info'] = dict(sorted(town_dict.items()))
    #         county_dict: dict = code_dict[state_code]['city_info'][city_code]['county_info']
    #         code_dict[state_code]['city_info'][city_code]['county_info'] = dict(sorted(county_dict.items()))
    #     city_dict: dict = code_dict[state_code]['city_info']
    #     code_dict[state_code]['city_info'] = dict(sorted(city_dict.items()))
    contry_dict = {'country_name': country_name_zh, 'country_name_en': country_name_en, 'country_name_zh': country_name_zh,'country_name_local':country_name, 'country_code': country_code, \
                   'state_bit_count': state_bit_count, 'district_bit_count': district_bit_count, 'county_bit_count': county_bit_count, 'town_bit_count': town_bit_count, \
                   'state_nums': len(code_dict), \
                   'state_info': code_dict}
    return contry_dict

def encode_district_code_from_json(json_path: pathlib.Path, thread_num:int = 1):
    with open(json_path, 'r', encoding='utf-8') as f:
        contry_dict = json.load(f)
    countries: dict = contry_dict['country_info']
    gdf_word_town = gpd.GeoDataFrame(columns=['name', 'name:en', 'name:zh', 'geometry', 'adcode'],crs = 'EPSG:4326')
    if thread_num > 1:
        import multiprocessing
        arg_list = []
        for country_id, country in countries.items():
            country_code = country['country_Alpha2']
            state_bit_count = country['state_bit_count']
            district_bit_count = country['district_bit_count']
            county_bit_count = country['county_bit_count']
            town_bit_count = country['town_bit_count']
            country_id_int = int(country_id)

            name = country['name']
            name_en = country['name:en']
            name_zh = country['name:zh']

            gdf_town = gpd.read_file(DISTRICT_DIR/{country_code}/f'{country_code}_admin_town.gpkg')
            gdf_county = gpd.read_file(DISTRICT_DIR/{country_code}/f'{country_code}_admin_county.gpkg')
            gdf_district = gpd.read_file(DISTRICT_DIR/{country_code}/f'{country_code}_admin_district.gpkg')
            gdf_state = gpd.read_file(DISTRICT_DIR/{country_code}/f'{country_code}_admin_state.gpkg')

            gdf_town = gdf_town[gdf_town['state_id'].notna() & gdf_town['state_id'].notnull() & gdf_town['district_id'].notna() & gdf_town['district_id'].notnull() & gdf_town['county_id'].notna() & gdf_town['county_id'].notnull().notna() ]
            gdf_county = gdf_county[gdf_county['state_id'].notna() & gdf_county['state_id'].notnull() & gdf_county['district_id'].notna() & gdf_county['district_id'].notnull()]
            gdf_district = gdf_district[gdf_district['state_id'].notna() & gdf_district['state_id'].notnull()]

            arg_list.append((gdf_town, gdf_county, gdf_district, gdf_state, \
                                            state_bit_count, district_bit_count, county_bit_count, town_bit_count,\
                                            country_id_int, name, name_en, name_zh))

        with multiprocessing.Pool(thread_num) as pool:
            results = pool.starmap(encode_district_code, arg_list)
        for result in results:
            for key, value in result.items():
                countries[f"{result['country_code']}"][key] = value
        for gdf_town,_,_,_,_,_,_,_,_,_,_,_ in arg_list:
            gdf_word_town = pd.concat([gdf_word_town, gdf_town], ignore_index=True)
    else:
        for country_id, country in countries.items():
            country_id_int = int(country_id)
            country_code = country['country_Alpha2']
            state_bit_count = country['state_bit_count']
            district_bit_count = country['district_bit_count']
            county_bit_count = country['county_bit_count']
            town_bit_count = country['town_bit_count']

            name = country['name:local'] if 'name:local' in country else country['name']
            name_en = country['name:en']
            name_zh = country['name:zh']

            gdf_town = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_town.gpkg')
            gdf_county = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_county.gpkg')
            gdf_district = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_district.gpkg')
            gdf_state = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_state.gpkg')

            gdf_town = gdf_town[gdf_town['state_id'].notna() & gdf_town['state_id'].notnull() & gdf_town['district_id'].notna() & gdf_town['district_id'].notnull() & gdf_town['county_id'].notna() & gdf_town['county_id'].notnull().notna() ]
            gdf_county = gdf_county[gdf_county['state_id'].notna() & gdf_county['state_id'].notnull() & gdf_county['district_id'].notna() & gdf_county['district_id'].notnull()]
            gdf_district = gdf_district[gdf_district['state_id'].notna() & gdf_district['state_id'].notnull()]

            code_dict = encode_district_code(gdf_town, gdf_county, gdf_district, gdf_state, \
                                            state_bit_count, district_bit_count, county_bit_count, town_bit_count,\
                                            country_id_int, name, name_en, name_zh)
            
            gdf_town.to_file(DISTRICT_DIR/country_code/f'{country_code}_admin_town.gpkg')
            gdf_county.to_file(DISTRICT_DIR/country_code/f'{country_code}_admin_county.gpkg')
            gdf_district.to_file(DISTRICT_DIR/country_code/f'{country_code}_admin_district.gpkg')
            gdf_state.to_file(DISTRICT_DIR/country_code/f'{country_code}_admin_state.gpkg')

            with open(f'./data/district/{country_code}/{country_code}_admin_code_table.json', 'w', encoding='utf-8') as f:
                json.dump(code_dict, f, ensure_ascii=False, indent=4)

            for key, value in code_dict.items():
                countries[country_id][key] = value
            
            gdf_word_town = pd.concat([gdf_word_town, gdf_town], ignore_index=True)

    with open(json_path, 'w', encoding='utf-8') as f:
        json.dump(contry_dict, f, ensure_ascii=False, indent=4)

    gdf_word_town.to_file(WORKING_DIR.joinpath(DISTRICT_DIR/'world'/'world_admin_town.gpkg'), driver='GPKG')

def reproject_2_proj_crs(gdf: gpd.GeoDataFrame, proj_wkt: str) -> gpd.GeoDataFrame:
    return gdf.to_crs(proj_wkt)

def validate_adcode(json_path: pathlib.Path):
    gdf_world_town = gpd.read_file(DISTRICT_DIR/'world'/'world_admin_town.gpkg')
    with open(json_path, 'r', encoding='utf-8') as f:
        json_data = json.load(f)
    for country_code, country in json_data['country_info'].items():
        state_bit_count = country['state_bit_count']
        district_bit_count = country['district_bit_count']
        county_bit_count = country['county_bit_count']
        town_bit_count = country['town_bit_count']
        states = country['state_info']
        states_count = country['state_nums']
        assert len(states) == states_count, f'states_count: {states_count}, states: {len(states)}, country_code: {country_code}'
        for state_code, state in states.items():
            state_code = int(state_code)
            state_id = state_code&(1<<state_bit_count)-1
            assert not state_id == 0 and state_id <= states_count
            districts = state['city_info']
            cities_count = state['city_nums']
            assert len(districts) == cities_count , f'cities_count: {cities_count}, districts: {len(districts)}, state_code: {state_code}'
            for district_code, district in districts.items():
                district_code = int(district_code)
                district_id = district_code&(1<<district_bit_count)-1
                assert not district_id == 0 and district_id <= cities_count
                state_id_from_district_code = (district_code>>district_bit_count)&(1<<state_bit_count)-1
                counties = district['county_info']
                counties_count = district['county_nums']
                assert len(counties) == counties_count, f'counties_count: {counties_count}, counties: {len(counties)}, district_code: {district_code}'
                for county_code, county in counties.items():
                    county_code = int(county_code)
                    county_id = county_code&(1<<county_bit_count)-1
                    assert not county_id == 0 and county_id <= counties_count
                    district_id_from_county_code = (county_code>>county_bit_count)&(1<<district_bit_count)-1
                    state_id_from_county_code = (county_code>>(county_bit_count+district_bit_count))&(1<<state_bit_count)-1
                    towns = county['town_info']
                    towns_count = county['town_nums']
                    assert len(towns) == towns_count, f'towns_count: {towns_count}, towns: {len(towns)}, county_code: {county_code}'
                    for town_code, town in towns.items():
                        town_code = int(town_code)
                        town_data = gdf_world_town[gdf_world_town['adcode'] == int(town_code)]
                        assert len(town_data) == 1
                        town_data = town_data.iloc[0]
                        # county_id = town_data['county_id']
                        # district_id = town_data['district_id']
                        # state_id = town_data['state_id']
                        town_id = town_code&(1<<town_bit_count)-1
                        county_id_from_town_code = (town_code>>town_bit_count)&(1<<county_bit_count)-1
                        district_id_from_town_code = (town_code>>(town_bit_count+county_bit_count))&(1<<district_bit_count)-1
                        state_id_from_town_code = (town_code>>(town_bit_count+county_bit_count+district_bit_count))&(1<<state_bit_count)-1
                        
                        assert not town_id == 0 and town_id <= towns_count, f'town_id: {town_id}, town_code: {town_code}'
                        assert county_id == county_id_from_town_code, f'county_id: {county_id}, county_id_from_town_code: {county_id_from_town_code}, town_code: {town_code}'
                        assert county_id == int(town_data['county_id']), f'county_id: {county_id}, county_id_from_town_code: {county_id_from_town_code}, town_code: {town_code}'
                        assert district_id == district_id_from_town_code, f'district_id: {district_id}, district_id_from_town_code: {district_id_from_town_code}, town_code: {town_code}'
                        assert district_id == int(town_data['district_id']), f'district_id: {district_id}, district_id_from_town_code: {district_id_from_town_code}, town_code: {town_code}'
                        assert state_id == state_id_from_town_code, f'state_id: {state_id}, state_id_from_town_code: {state_id_from_town_code}, town_code: {town_code}'
                        assert state_id == int(town_data['state_id']), f'state_id: {state_id}, state_id_from_town_code: {state_id_from_town_code}, town_code: {town_code}'
                    assert district_id == district_id_from_county_code, f'district_id: {district_id}, district_id_from_county_code: {district_id_from_county_code}, county_code: {county_code}'
                    assert state_id == state_id_from_county_code, f'state_id: {state_id}, state_id_from_county_code: {state_id_from_county_code}, county_code: {county_code}'
                assert state_id == state_id_from_district_code, f'state_id: {state_id}, state_id_from_district_code: {state_id_from_district_code}, district_code: {district_code}'

if __name__ == '__main__':

    # country_code = 'GB'

    # gdf_town = gpd.read_file(f'./data/district/{country_code}/{country_code}_admin_town.gpkg')
    # gdf_county = gpd.read_file(f'./data/district/{country_code}/{country_code}_admin_county.gpkg')
    # gdf_district = gpd.read_file(f'./data/district/{country_code}/{country_code}_admin_district.gpkg')
    # gdf_state = gpd.read_file(f'./data/district/{country_code}/{country_code}_admin_state.gpkg')

    # gdf_town = gdf_town[gdf_town['state_id'].notna() & gdf_town['state_id'].notnull() & gdf_town['district_id'].notna() & gdf_town['district_id'].notnull() & gdf_town['county_id'].notna() & gdf_town['county_id'].notnull().notna() ]
    # gdf_county = gdf_county[gdf_county['state_id'].notna() & gdf_county['state_id'].notnull() & gdf_county['district_id'].notna() & gdf_county['district_id'].notnull() & gdf_county['county_id'].notna() & gdf_county['county_id'].notnull().notna() ]
    # gdf_district = gdf_district[gdf_district['state_id'].notna() & gdf_district['state_id'].notnull()]

    # code_dict = encode_district_code(gdf_town, gdf_district, gdf_county, gdf_state, 3,1,7,12, 7, 'United Kingdom', 'United Kingdom', '大不列颠及北爱尔兰联合王国')

    # with open(f'./data/district/{country_code}/{country_code}_admin_code.json', 'w') as f:
    #     json.dump(code_dict, f, ensure_ascii=False, indent=4)

    # gdf_town.to_file(f'./data/district/{country_code}/{country_code}_admin_town.gpkg')
    # gdf_county.to_file(f'./data/district/{country_code}/{country_code}_admin_county.gpkg')
    # gdf_district.to_file(f'./data/district/{country_code}/{country_code}_admin_district.gpkg')
    # gdf_state.to_file(f'./data/district/{country_code}/{country_code}_admin_state.gpkg')

    encode_district_code_from_json(DISTRICT_DIR/'world'/'world_admin_code.json')

    validate_adcode(DISTRICT_DIR/'world'/'world_admin_code.json')

    # gdf_world_town = gpd.read_file(DISTRICT_DIR/'world'/'world_admin_town.gpkg')
    # gdf_world_town = reproject_2_proj_crs(gdf_world_town, PROJECTED_WKT_STR)
    # gdf_world_town.to_file(DISTRICT_DIR/'world'/'world_admin_town_proj.gpkg')


