import geopandas as gpd
import pandas as pd
import shapely.geometry
import json
import warnings
import math

import pathlib
import sys
sys.path.append(pathlib.Path(__file__).parent.parent.absolute().as_posix())
from constants import *
DISTRICT_DIR = DATA_DIR / 'district'
warnings.filterwarnings('ignore', 'GeoSeries.notna', UserWarning)

def find_admin_parrent(gdf_town: gpd.GeoDataFrame, gdf_county:gpd.GeoDataFrame, gdf_district: gpd.GeoDataFrame, gdf_state: gpd.GeoDataFrame):
    gdf_town.set_crs('epsg:4326', allow_override=True)
    gdf_county.set_crs('epsg:4326', allow_override=True)
    gdf_district.set_crs('epsg:4326', allow_override=True)
    gdf_state.set_crs('epsg:4326', allow_override=True)

    #gen id for state gdf
    for i, row in gdf_state.iterrows():
        gdf_state.loc[i,'state_id'] = i +1 

    #add district count for state gdf
    gdf_state['child_count'] = 0

    for i, row in gdf_district.iterrows():
        if isinstance(row.geometry, shapely.geometry.Polygon):
            district_center = row.geometry.centroid
        elif isinstance(row.geometry, shapely.geometry.MultiPolygon):
            #get the centroid of the largest polygon
            district_center = shapely.geometry.shape(row.geometry.geoms[0]).centroid
        mask = gdf_state.contains(district_center)
        if mask.any():
            state_name = gdf_state[mask].iloc[0]['name']
            state_name_en = gdf_state[mask].iloc[0]['name:en']
            state_name_zh = gdf_state[mask].iloc[0]['name:zh']
            gdf_district.loc[i, 'state_name'] = state_name
            gdf_district.loc[i, 'state_name_en'] = state_name_en
            gdf_district.loc[i, 'state_name_zh'] = state_name_zh
            gdf_district.loc[i, 'state_id'] = gdf_state[mask].iloc[0]['state_id']
            gdf_state.loc[mask, 'child_count'] += 1
            gdf_district.loc[i, 'district_id'] = gdf_state[mask].iloc[0]['child_count']

            gdf_district.loc[i, 'child_count'] = 0

    country_name = gdf_state['country_code'].unique()[0]
    most_child_state_row = gdf_state.loc[gdf_state['child_count'].idxmax()]
    print(f'{country_name} states num: {gdf_state.shape[0]}, states max child num: {most_child_state_row["child_count"]}, states max child name: {most_child_state_row["name"]}')

    #add district count for county gdf
    gdf_district['child_count'] = 0

    for i, row in gdf_county.iterrows():
        #check if the geometry is polygon or multipolygon
        if isinstance(row.geometry, shapely.geometry.Polygon):
            county_center = row.geometry.centroid
        elif isinstance(row.geometry, shapely.geometry.MultiPolygon):
            #get the centroid of the largest polygon
            county_center = shapely.geometry.shape(row.geometry.geoms[0]).centroid
        mask = gdf_district.contains(county_center)
        if mask.any():
            district_name = gdf_district[mask].iloc[0]['name']
            district_name_en = gdf_district[mask].iloc[0]['name:en']
            district_name_zh = gdf_district[mask].iloc[0]['name:zh']
            gdf_county.loc[i, 'district_name'] = district_name
            gdf_county.loc[i, 'district_name_en'] = district_name_en
            gdf_county.loc[i, 'district_name_zh'] = district_name_zh
            gdf_county.loc[i, 'district_id'] = gdf_district[mask].iloc[0]['district_id']
            gdf_district.loc[mask, 'child_count'] += 1
            gdf_county.loc[i, 'county_id'] = gdf_district[mask].iloc[0]['child_count']

            state_name = gdf_district[mask].iloc[0]['state_name']
            state_name_en = gdf_district[mask].iloc[0]['state_name_en']
            state_name_zh = gdf_district[mask].iloc[0]['state_name_zh']
            gdf_county.loc[i, 'state_name'] = state_name
            gdf_county.loc[i, 'state_name_en'] = state_name_en
            gdf_county.loc[i, 'state_name_zh'] = state_name_zh
            gdf_county.loc[i, 'state_id'] = gdf_district[mask].iloc[0]['state_id']

            gdf_county.loc[i, 'child_count'] = 0
    most_child_district_row = gdf_district.loc[gdf_district['child_count'].idxmax()]
    print(f'{country_name} district num: {gdf_district.shape[0]}, districts max child num: {most_child_district_row["child_count"]}, districts max child name: {most_child_district_row["name"]}')

    gdf_county['child_count'] = 0

    for i, row in gdf_town.iterrows():
        if isinstance(row.geometry, shapely.geometry.Polygon):
            town_center = row.geometry.centroid
        elif isinstance(row.geometry, shapely.geometry.MultiPolygon):
            #get the centroid of the largest polygon
            town_center = shapely.geometry.shape(row.geometry.geoms[0]).centroid
        mask = gdf_county.contains(town_center)
        if mask.any():
            county_name = gdf_county[mask].iloc[0]['name']
            county_name_en = gdf_county[mask].iloc[0]['name:en']
            county_name_zh = gdf_county[mask].iloc[0]['name:zh']
            gdf_town.loc[i, 'county_name'] = county_name
            gdf_town.loc[i, 'county_name_en'] = county_name_en
            gdf_town.loc[i, 'county_name_zh'] = county_name_zh
            gdf_town.loc[i, 'county_id'] = gdf_county[mask].iloc[0]['county_id']
            gdf_county.loc[mask, 'child_count'] += 1
            gdf_town.loc[i, 'town_id'] = gdf_county[mask].iloc[0]['child_count']

            district_name = gdf_county[mask].iloc[0]['district_name']
            district_name_en = gdf_county[mask].iloc[0]['district_name_en']
            district_name_zh = gdf_county[mask].iloc[0]['district_name_zh']
            gdf_town.loc[i, 'district_name'] = district_name
            gdf_town.loc[i, 'district_name_en'] = district_name_en
            gdf_town.loc[i, 'district_name_zh'] = district_name_zh
            gdf_town.loc[i, 'district_id'] = gdf_county[mask].iloc[0]['district_id']

            state_name = gdf_county[mask].iloc[0]['state_name']
            state_name_en = gdf_county[mask].iloc[0]['state_name_en']
            state_name_zh = gdf_county[mask].iloc[0]['state_name_zh']
            gdf_town.loc[i, 'state_name'] = state_name
            gdf_town.loc[i, 'state_name_en'] = state_name_en
            gdf_town.loc[i, 'state_name_zh'] = state_name_zh
            gdf_town.loc[i, 'state_id'] = gdf_county[mask].iloc[0]['state_id']
    most_child_county_row = gdf_county.loc[gdf_county['child_count'].idxmax()]
    print(f'{country_name} county num: {gdf_county.shape[0]}, counties max child num: {most_child_county_row["child_count"]}, counties max child name: {most_child_county_row["name"]}')

    print(f'{country_name} town num: {gdf_town.shape[0]}')

    
def merge_admin_levels(gdf_levels: list[gpd.GeoDataFrame], level: str) -> gpd.GeoDataFrame:
    gdf = gpd.GeoDataFrame(columns=['name', 'level', 'name:zh', 'name:en', 'geometry'])
    gdf.crs = 'EPSG:4326'
    gdf.set_crs('epsg:4326', allow_override=True)
    for gdf_level in gdf_levels:
        gdf_level.crs = 'EPSG:4326'
        for i, row in gdf_level.iterrows():
            row_shape = shapely.geometry.shape(row['geometry'])
            #check if the geometry is already in the gdf
            if not gdf.empty:
                if gdf.contains(row['geometry']).any():
                    continue
                if gdf.intersection(row_shape).any():
                    #get the intersected geometry
                    intersected = shapely.geometry.shape(gdf.intersection(row_shape).geometry.union_all())
                    #get the unintersected geometry
                    try:
                        unintersected = row_shape.difference(intersected)
                    except:
                        continue
                    new_row =  gpd.GeoDataFrame([{'name': row['name'] if 'name' in row else row['name:en'] if 'name:en' in row else row['name:zh'] if 'name:zh' in row else None,\
                                            'level': level, \
                                            'admin_level': row['admin_level'] if 'admin_level' in row else None, \
                                            'name:zh': row['name:zh'] if 'name:zh' in row else None,\
                                            'name:en': row['name:en'] if 'name:en' in row else None, \
                                            'geometry': unintersected}], crs='EPSG:4326')
                    gdf = pd.concat([gdf, new_row], ignore_index=True)
                    continue
            new_row =  gpd.GeoDataFrame([{'name': row['name'] if 'name' in row else row['name:en'] if 'name:en' in row else row['name:zh'] if 'name:zh' in row else None,\
                                    'level': level, \
                                    'admin_level': row['admin_level'] if 'admin_level' in row else None, \
                                    'name:zh': row['name:zh'] if 'name:zh' in row else None,\
                                    'name:en': row['name:en'] if 'name:en' in row else None, \
                                    'geometry': row['geometry']}], crs='EPSG:4326')
            gdf = pd.concat([gdf, new_row], ignore_index=True)
    gdf.crs = 'EPSG:4326'
    return gdf

def find_admin_parrent_country(country_code:str):
    gdf_country_town = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_town.gpkg')
    gdf_country_county = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_county.gpkg')
    gdf_country_district = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_district.gpkg')
    gdf_country_state = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_state.gpkg')

    gdf_country_state['country_code'] = country_code
    gdf_country_district['country_code'] = country_code
    gdf_country_county['country_code'] = country_code
    gdf_country_town['country_code'] = country_code

    #remove all empty geometry

    gdf_country_town = gdf_country_town[gdf_country_town['geometry'].notna()& ~gdf_country_town.geometry.is_empty]
    gdf_country_county = gdf_country_county[gdf_country_county['geometry'].notna()& ~gdf_country_county.geometry.is_empty]
    gdf_country_district = gdf_country_district[gdf_country_district['geometry'].notna()& ~gdf_country_district.geometry.is_empty]
    gdf_country_state = gdf_country_state[gdf_country_state['geometry'].notna()& ~gdf_country_state.geometry.is_empty]

    find_admin_parrent(gdf_country_town, gdf_country_county, gdf_country_district, gdf_country_state)

    gdf_country_town.to_file(DISTRICT_DIR/country_code/f'{country_code}_admin_town.gpkg')
    gdf_country_county.to_file(DISTRICT_DIR/country_code/f'{country_code}_admin_county.gpkg')
    gdf_country_district.to_file(DISTRICT_DIR/country_code/f'{country_code}_admin_district.gpkg')
    gdf_country_state.to_file(DISTRICT_DIR/country_code/f'{country_code}_admin_state.gpkg')

def get_country_admin_static(country_code:str):
    gdf_country_town = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_town.gpkg')
    gdf_country_county = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_county.gpkg')
    gdf_country_district = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_district.gpkg')
    gdf_country_state = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_state.gpkg')
    
    state_count = gdf_country_state.shape[0]
    district_count = gdf_country_district.shape[0]
    county_count = gdf_country_county.shape[0]
    town_count = gdf_country_town.shape[0]

    max_state_row = gdf_country_state.iloc[gdf_country_state['child_count'].idxmax()]
    max_district_row = gdf_country_district.iloc[gdf_country_district['child_count'].idxmax()]
    max_county_row = gdf_country_county.iloc[gdf_country_county['child_count'].idxmax()]

    state_bits = math.ceil(math.log2(state_count+1))
    district_bits = math.ceil(math.log2(max_state_row['child_count']+1))
    county_bits = math.ceil(math.log2(max_district_row['child_count']+1))
    town_bits = math.ceil(math.log2(max_county_row['child_count']+1))

    print(f"Country: {country_code}\n\
    state count: {state_count}, district count: {district_count}, county count: {county_count}, town count: {town_count}\n\
    max children country: {country_code}, state count: {state_count}, bits need: {state_bits}\n\
    max children state: {max_state_row['name']}, district count: {max_state_row['child_count']}, bits need: {district_bits}\n\
    max children district: {max_district_row['name']}, county count: {max_district_row['child_count']}, bits need: {county_bits}\n\
    max children county: {max_county_row['name']}, town count: {max_county_row['child_count']}, bits need: {town_bits}\n\
        total bits: {state_bits + district_bits + county_bits + town_bits}")

    return (state_bits, district_bits, county_bits, town_bits)

def get_country_admin_static_batch(country_codes: list, save_path: str, threads: int):
    country_bits = {}
    max_state_bits = 0
    max_district_bits = 0
    max_county_bits = 0
    max_town_bits = 0
    if threads == 1:
        for country_id,country_code, name_local, name_en, name_zh in country_codes:
            state_bits, district_bits, county_bits, town_bits = get_country_admin_static(country_code)
            country_bits[country_id] = {'country_Alpha2': country_code, \
                                        'name': name_zh, 'name:en': name_en, 'name:zh': name_zh, 'name:local': name_local,\
                                        'state_bit_count': state_bits, 'district_bit_count': district_bits, 'county_bit_count': county_bits, 'town_bit_count': town_bits}
            max_state_bits = max(max_state_bits, state_bits)
            max_district_bits = max(max_district_bits, district_bits)
            max_county_bits = max(max_county_bits, county_bits)
            max_town_bits = max(max_town_bits, town_bits)
    else:
        import multiprocessing
        country_ids, country_code, name_local, name_en, name_zh = zip(*country_codes)
        country_ids = list(country_ids)
        country_code = list(country_code)
        name_local = list(name_local)
        name_en = list(name_en)
        name_zh = list(name_zh)
        empty_list = [None] * len(country_ids)
        country_codes = list(zip(country_code))
        with multiprocessing.Pool(threads) as pool:
            res_bits = pool.starmap(get_country_admin_static,country_codes)
        for i in range(len(country_ids)):
            state_bits, district_bits, county_bits, town_bits = res_bits[i]
            country_bits[country_ids[i]] = {'country_Alpha2': country_code[i], \
                                            'name': name_local[i], 'name:en': name_en[i], 'name:zh': name_zh[i],\
                                            'state_bit_count': state_bits, 'district_bit_count': district_bits, 'county_bit_count': county_bits, 'town_bit_count': town_bits}
            max_state_bits = max(max_state_bits, state_bits)
            max_district_bits = max(max_district_bits, district_bits)
            max_county_bits = max(max_county_bits, county_bits)
            max_town_bits = max(max_town_bits, town_bits)
    with open(save_path, 'w') as f:
        json.dump({'country_nums': len(country_codes),\
                   'max_state_bits': max_state_bits, 'max_district_bits': max_district_bits, 'max_county_bits': max_county_bits, 'max_town_bits': max_town_bits,\
                   'country_info':country_bits}, f, ensure_ascii=False, indent=4)

def process_raw_admin_boundaries(country_code:str, admin_levels:list[int], town_level:list[int], county_level:list[int], district_level:list[int], state_level:list[int],\
                                  save_path = DISTRICT_DIR):
    raw_admin_levels  = {}
    for lvl in admin_levels:
        raw_admin_levels[lvl] = gpd.read_file(DISTRICT_DIR/country_code/f'{country_code}_admin_level_{lvl}.gpkg')
    town_adm_lvl = [ raw_admin_levels[i] for i in town_level ]
    county_adm_lvl = [ raw_admin_levels[i] for i in county_level ]
    district_adm_lvl = [ raw_admin_levels[i] for i in district_level ]
    state_adm_lvl = [ raw_admin_levels[i] for i in state_level ]

    gdf_country_town = merge_admin_levels(town_adm_lvl, 'town')
    gdf_country_county = merge_admin_levels(county_adm_lvl, 'county')
    gdf_country_district = merge_admin_levels(district_adm_lvl, 'district')
    gdf_country_state = merge_admin_levels(state_adm_lvl, 'state')
    gdf_country_town['country_code'] = country_code
    gdf_country_county['country_code'] = country_code
    gdf_country_district['country_code'] = country_code
    gdf_country_state['country_code'] = country_code
    gdf_country_town.to_file(save_path/f'{country_code}/{country_code}_admin_town.gpkg')
    gdf_country_county.to_file(save_path/f'{country_code}/{country_code}_admin_county.gpkg')
    gdf_country_district.to_file(save_path/f'{country_code}/{country_code}_admin_district.gpkg')
    gdf_country_state.to_file(save_path/f'{country_code}/{country_code}_admin_state.gpkg')


    find_admin_parrent_country(country_code)
    print(f"✅ Saved to {country_code}_admin_town.gpkg, {country_code}_admin_county.gpkg, {country_code}_admin_district.gpkg, {country_code}_admin_state.gpkg")
    

if __name__ == "__main__":

    """    
    process_raw_admin_boundaries('CN',[8,7,6,4],[8,7,6],[6,5],[5,4],[4])
    process_raw_admin_boundaries('KR',[8,7,6,4],[8,7,6,4],[6,4],[4],[4])
    process_raw_admin_boundaries('JP',[9,8,7,6,5,4],[9,8,7,6],[7,4],[5,6,4],[4])
    process_raw_admin_boundaries('US',[9,8,7,6,4],[8,7,6],[6,4],[4],[4])
    process_raw_admin_boundaries('DE',[9,8,7,6,4],[8,9],[6,4],[4],[4])
    process_raw_admin_boundaries('TW',[9,8,7,4,2],[9,8,7,4],[8,7,4],[4,2],[2])
    process_raw_admin_boundaries('FR',[9,8,7,6,4],[9,8,7],[7,6,4],[6,4],[4])
    process_raw_admin_boundaries('IT',[8,7,6,4],[8,7],[7,6,4],[6,4],[4])
    process_raw_admin_boundaries('ES',[8,7,6,4],[8],[7,6,4],[6,4],[4])
    process_raw_admin_boundaries('GB',[10,7,6,5,4],[10,7,6,5,4],[7,6,5,4],[4],[4])
    process_raw_admin_boundaries('GR',[8,7,6,5,4],[8,7],[6],[5],[4])
    process_raw_admin_boundaries('FI',[10,9,8,7,4],[10,9,8],[8],[7,4],[4])
    process_raw_admin_boundaries('PL',[9,8,7,6,4],[9,8,7],[7],[6],[4])
    process_raw_admin_boundaries('NO',[9,7,4],[9,7,4],[7],[4],[4])
    process_raw_admin_boundaries('SE',[10,9,8,7,4],[10,9,8,7],[7],[4],[4])
    process_raw_admin_boundaries('DK',[7,4],[7],[7],[4],[4])
    """
    #
    # arg_list =[
    #     ('CN', [8,7,6,5,4], [8,7,6], [6,5], [5,4], [4]),
    #     ('KR', [8,7,6,4], [8,7,6,4], [6,4], [4], [4]),
    #     ('JP', [9,8,7,6,5,4], [9,8,7,6], [7,4], [5,6,4], [4]),
    #     ('US', [9,8,7,6,4], [8,7,6], [6,4], [4], [4]),
    #     ('DE', [9,8,7,6,4], [8,9], [6,4], [4], [4]),
    #     ('TW', [9,8,7,4,2], [9,8,7], [8,7], [4], [2]),
    #     ('FR', [9,8,7,6,4], [9,8,7], [7,6,4], [6,4], [4]),
    #     ('IT', [8,7,6,4], [8,7], [7,6,4], [6,4], [4]),
    #     ('ES', [8,7,6,4], [8], [7,6,4], [6,4], [4]),
    #     ('GB', [10,7,6,5,4], [10,7,6,5,4], [7,6,5,4], [4], [4]),
    #     ('GR', [8,7,6,5,4], [8,7], [6], [5], [4]),
    #     ('FI', [10,9,8,7,4], [10,9,8], [8], [7,4], [4]),
    #     ('PL', [9,8,7,6,4], [9,8,7], [7], [6], [4]),
    #     ('NO', [9,7,4], [9,7,4], [7], [4], [4]),
    #     ('SE', [10,9,8,7,4], [10,9,8,7], [7], [4], [4]),
    #     ('DK', [7,4], [7], [7], [4], [4])
    # ]
    # import multiprocessing
    # with multiprocessing.Pool(processes=14) as pool:
    #     pool.starmap(process_raw_admin_boundaries, arg_list)

    #process_raw_admin_boundaries('TW',[9,8,7,4,2],[9,8,7,4],[8,7,4],[4,2],[2])
    #find_admin_parrent_country('TW')
    process_raw_admin_boundaries('DE',[9,8,7,6,4],[8,9,6,4],[6,4],[4],[4])

    country_codes=[
        'CN',
        'KR',
        'JP',
        'US',
        'DE',
        'TW',
        'FR',
        'IT',
        'ES',
        'GB',
        'GR',
        'FI',
        'PL',
        'NO',
        'SE',
        'DK'
    ]
    # with multiprocessing.Pool(processes=14) as pool:
    #     pool.map(find_admin_parrent_country,country_codes)
    
    country_infos = [
        #(id, code, name_local, name_en, name_zh)
        (1, 'CN','中国', 'China', '中国'), #CN
        (2, 'KR','한국', 'Korea', '韩国'), #KR
        (3, 'JP','日本', 'Japan', '日本'), #JP
        (4, 'US','United States', 'United States', '美国'), #US
        (5, 'DE','Deutschland', 'Germany', '德国'), #DE
        (6, 'TW','台灣', 'Taiwan', '台湾'), #TW
        (7, 'FR','France', 'France', '法国'), #FR
        (8, 'IT','Italia', 'Italy', '意大利'), #IT
        (9, 'ES','España', 'Spain', '西班牙'), #ES
        (10, 'GB','United Kingdom', 'United Kingdom', '英国'), #GB
        (11, 'GR','Ελλάδα', 'Greece', '希腊'), #GR
        (12, 'FI','Suomi', 'Finland', '芬兰'), #FI
        (13, 'PL','Polska', 'Poland', '波兰'), #PL
        (14, 'NO','Norge', 'Norway', '挪威'), #NO
        (15, 'SE','Sverige', 'Sweden', '瑞典'), #SE
        (16, 'DK','Danmark', 'Denmark', '丹麦'), #DK
    ]

    # # for country_code in country_codes:
    # #     get_country_admin_static(country_code)
            
    # #find_admin_parrent_country('JP')
    # import multiprocessing

    # with multiprocessing.Pool() as pool:
    #     pool.map(get_country_admin_static, country_codes)

    get_country_admin_static_batch(country_infos, DISTRICT_DIR/'world_admin_code.json'.absolute(), 8)

    # with multiprocessing.Pool() as pool:
    #     pool.map(find_admin_parrent_country, country_codes)
