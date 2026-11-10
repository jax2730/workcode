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
import multiprocessing
from functools import partial
import pyproj
import shapely.geometry as geom

MAP_OFFSET = [-2097152.0, -1048576.0]; 

def convert_poi_2_world_cord(poi_path: pathlib.Path, world_coord_path: pathlib.Path):
    poi_gpkg = gpd.read_file(poi_path)
    poi_gpkg.to_crs(epsg=3857, inplace=True)
    city_bound = poi_gpkg.total_bounds
    crs_wkt = poi_gpkg.crs.to_wkt()
    world_terrian = DATA_DIR /'world_terrain/worldmap_fill.tif'
    convert_coords.load_base_gtiff(world_terrian)
    convert_coords.load_x_y_range_data((city_bound[0],city_bound[2]), (city_bound[1],city_bound[3]),from_wkt=crs_wkt)
    convert_coords.set_origin_point(MAP_OFFSET[0],MAP_OFFSET[1])
    convert_coords.set_pixel_scale(32)

    world_coord_list = []

    for index, row in poi_gpkg.iterrows():
        row: gpd.GeoSeries
        coord = convert_coords.convert_coords_with_height(coord=np.array(row.geometry.coords.xy).reshape(2), from_wkt=crs_wkt)
        poi_dict = {'name': row['name:local'], 'coord': coord.tolist()}
        world_coord_list.append(poi_dict)

    with open(world_coord_path, 'w') as f:
        json.dump(world_coord_list, f, ensure_ascii=False,indent=4)

def get_query_dict(radius:int,pos:tuple):
    x,y=pos
    return {
        'Airport': f'nwr(around:{radius},{y},{x})[aeroway = "aerodrome" ]; // airport',
        'AmusementPark': f'nwr(around:{radius},{y},{x})[tourism = "theme_park"]; // amusement park',
        'Bank': f'nwr(around:{radius},{y},{x})[amenity = "bank"]; // bank',
        'CarDealer': f'nwr(around:{radius},{y},{x})[shop="car"]; // car dealef',
        'CoachStation': f'nwr(around:{radius},{y},{x})[amenity="bus_station"]; // Bus station. Notice this should be intercity bus station. but right now just ragular bus station instead.',
        'ConvienceStore': f'nwr(around:{radius},{y},{x})[shop="convenience"]; // convenience store',
        'Factory': f'nwr(around:{radius},{y},{x})[industrial ="factory"]; // factory',
        'FarmerMarket': f'nwr(around:{radius},{y},{x})[amenity ="marketplace"]; // farmers market',
        'GasStation': f'nwr(around:{radius},{y},{x})[amenity ="fuel"]; // fuel station',
        'HairSalon': f'nwr(around:{radius},{y},{x})[shop ="hairdresser"]; // hairdressef',
        'HomeBuildingMaterialMarket': f'(// TODO: better query method for hardware store\n\
            nwr(around:{radius},{y},{x})[shop ="doityourself"];\
            nwr(around:{radius},{y},{x})[shop ="hardware"];\
            ); // hardware store. Notice this query is not working in China due to lack of data.',
        'Hospital': f'nwr(around:{radius},{y},{x})[amenity ="hospital"]; // hospital',
        'Hotel': f'nwr(around:{radius},{y},{x})[tourism ="hotel"]; // hotel',
        'Kindergarten': f'nwr(around:{radius},{y},{x})[amenity ="kindergarten"]; // kindergarten',
        'KTV': f'// TODO: better query method for karaoke\n\
            nwr(around:{radius},{y},{x})[amenity ="bar"]; // karaoke Notice: there is no data for karaoke in OSM. use bar instead.',
        'LanzhouRamen': f'nwr(around:{radius},{y},{x})[cuisine ="ramen"]; // ramen shop Notice: suppose to be Lanzhou ramen shop, but there is no data for Lanzhou Ramen in OSM. use ramen shop instead.',
        'Library': f'nwr(around:{radius},{y},{x})[amenity ="library"]; // library',
        'McDonalds': f'nwr(around:{radius},{y},{x})["brand:en"="McDonald\'s"]; // McDonald\'s',
        'Museum': f'nwr(around:{radius},{y},{x})[tourism="museum"]; // museum',
        'OfficeBuilding': f'nwr(around:{radius},{y},{x})[building = "commercial"]; // office building',
        'Onsen': f'nwr(around:{radius},{y},{x})[leisure = "water_park"]; // Onsen Notice: there is no data for Onsen in OSM. use water park instead.',
        'Park': f'nwr(around:{radius},{y},{x})[leisure = "park"]; // park',
        'Port': f'nwr(around:{radius},{y},{x})[industrial = "port"]; // port',
        'PostOffice': f'nwr(around:{radius},{y},{x})[amenity = "post_office"]; // post office',
        'PublicBathroom': f'node(around:{radius},{y},{x})[amenity = "toilets"]; // public bathroom',
        'ResidentalComplex': f'nwr(around:{radius},{y},{x})[landuse = "residential"]; // residential complex',
        'School': f'(\
            nwr(around:{radius},{y},{x})[amenity = "university"];\
            nwr(around:{radius},{y},{x})[amenity = "school"];\
            ); // school and university',
        'ShoppingMall': f'(\
            nwr(around:{radius},{y},{x})[landuse="retail"];\
            nwr(around:{radius},{y},{x})[shop="mall"];\
                ); // shopping mall',
        'Stadium': f'nwr(around:{radius},{y},{x})[leisure = "stadium"]; // stadium',
        'TrainStation': f'nwr(around:{radius},{y},{x})[train][railway="station"]; // train station',
        'ViewSpot': f'nwr(around:{radius},{y},{x})[tourism="attraction"]; // tourist attraction',
        'Zoo': f'nwr(around:{radius},{y},{x})[tourism="zoo"]; // zoo'
    }

def query_poi_by_type_worker(poi_type:str,pos:tuple,radius:int=25000,log:bool=False):
    query_dict = get_query_dict(radius,pos)
    query = f"""
    [out:xml][timeout:1000][maxsize:2147483648];
    {query_dict[poi_type]}
    out body;
    >;
    out skel qt;
    """
    str = query_osm_data_local_str(query, log)
    if len(str) == 0:
        print(f"Error querying {poi_type}") if log else None
        return
    gdf = convert_poi_2_gdf(str, poi_type)
    print(f"Finish querying {poi_type}") if log else None
    return gdf

def query_poi_around_pos(pos:tuple,radius:int=25000,log:bool=False):
    save_dir = DATA_DIR / "query_poi"

    osm_dir = save_dir / "osm"
    if not osm_dir.exists():
        osm_dir.mkdir(parents=True)
    else:
        for file in osm_dir.iterdir():
            file.unlink()
        gpkg = save_dir / "poi.gpkg"
        if gpkg.exists():
            gpkg.unlink()
    query_dict = get_query_dict(radius,pos)
    work_func = partial(query_poi_by_type_worker,pos=pos,radius=radius,log=log)
    with multiprocessing.Pool(processes=4) as pool:
        gdfs = pool.map(work_func, query_dict.keys())
    gdfs = [gdf for gdf in gdfs if gdf is not None]
    poi_gdf = gpd.GeoDataFrame(pd.concat(gdfs, ignore_index=True))
    poi_gdf.to_crs(epsg=3857, inplace=True)
    city_bound = poi_gdf.total_bounds
    crs_wkt = poi_gdf.crs.to_wkt()
    world_terrian = DATA_DIR /'world_terrain/worldmap_fill.tif'
    convert_coords.load_base_gtiff(world_terrian)
    convert_coords.load_x_y_range_data((city_bound[0],city_bound[2]), (city_bound[1],city_bound[3]),from_wkt=crs_wkt)
    convert_coords.set_origin_point(MAP_OFFSET[0],MAP_OFFSET[1])
    convert_coords.set_pixel_scale(32)

    world_coord_list = []
    pos_point = geom.Point(pos)
    transformer = pyproj.Transformer.from_crs('EPSG:4326','EPSG:3857',always_xy=True)
    x,y = transformer.transform(pos[0],pos[1])
    pos_nd = np.array([x,y])
    pos_nd = convert_coords.convert_coords(coord=pos_nd, from_wkt=crs_wkt)
    for index, row in poi_gdf.iterrows():
        row: gpd.GeoSeries
        name = row['name:zh'] if row['name:zh'] else row['name:en'] if row['name:en'] else row['name:local']
        if name is None:
            name = 'unnamed'
        coord = convert_coords.convert_coords(coord=np.array(row.geometry.coords.xy).reshape(2), from_wkt=crs_wkt)
        poi_dict = {'type': row['type'],'name': name, 'pos': (coord-pos_nd).tolist(),'coord':coord.tolist()}
        world_coord_list.append(poi_dict)

    with open(save_dir / "poi_world_coord.json", 'w') as f:
        json.dump(world_coord_list, f, ensure_ascii=False,indent=4)


    poi_gdf.to_file(save_dir / "poi.gpkg", driver="GPKG")
    return world_coord_list

def query_poi_around_city(city_name:str, radius: int = 25000, log: bool = False):
    save_dir = DATA_DIR / "city" / city_name
    osm_dir = save_dir / "osm"
    if not osm_dir.exists():
        osm_dir.mkdir(parents=True)
    else:
        for file in osm_dir.iterdir():
            file.unlink()
    
    query_city = f"""
    [out:xml][timeout:25];
    node[place="city"]["name:en"="{city_name}"];
    out body;
    >;
    out skel qt;
    """

    res = query_osm_data_local_str(query_city, log)
    if len(res) == 0:
        print(f"Error querying city {city_name}") if log else None
        return
    import xml.etree.ElementTree as ET
    root = ET.fromstring(res)
    city_node = root.find('node')
    if city_node is None:
        print(f"Error querying city {city_name}") if log else None
        return
    city_pos = (float(city_node.get('lon')),float(city_node.get('lat')))
    query_dict = get_query_dict(radius,city_pos)
    work_func = partial(query_poi_by_type_worker,pos=city_pos,radius=radius,log=log)
    with multiprocessing.Pool(processes=4) as pool:
        gdfs = pool.map(work_func, query_dict.keys())
    gdfs = [gdf for gdf in gdfs if gdf is not None]
    poi_gdf = gpd.GeoDataFrame(pd.concat(gdfs, ignore_index=True))
    poi_gdf['name'] = poi_gdf.apply(lambda row: row['name:zh'] if row['name:zh'] else row['name:en'] if row['name:en'] else row['name:local'], axis=1)
    # poi_gdf = poi_gdf[poi_gdf['name'].notna()]
    poi_gdf.to_crs(epsg=3857, inplace=True)
    if (save_dir / "poi.gpkg").exists():
        (save_dir / "poi.gpkg").unlink()
    poi_gdf.to_file(save_dir / "poi.gpkg", driver="GPKG")

def query_poi_around_stop(city_name:str, radius: int = 25000, log: bool = False):
    save_dir = DATA_DIR / "subway" / city_name
    osm_dir = save_dir / "osm"
    if not osm_dir.exists():
        osm_dir.mkdir(parents=True)
    else:
        for file in osm_dir.iterdir():
            file.unlink()
    
    query_city = f"""
    [out:xml][timeout:25];
    area["ISO3166-1"="CN"]["admin_level"="2"]->.country;
    node["railway"="stop"]["subway"="yes"](area.country);
    out body;
    >;
    out skel qt;
    """

    res = query_osm_data_local_str(query_city, log)
    if len(res) == 0:
        print(f"Error querying city {city_name}") if log else None
        return
    import xml.etree.ElementTree as ET
    root = ET.fromstring(res)
    city_node = root.find('node')
    if city_node is None:
        print(f"Error querying city {city_name}") if log else None
        return
    city_pos = (float(city_node.get('lon')),float(city_node.get('lat')))
    query_dict = {'stop': f'nwr(around:{radius},{city_pos[1]},{city_pos[0]})["railway"="stop"]["subway"="yes"]'}
    work_func = partial(query_poi_by_type_worker,pos=city_pos,radius=radius,log=log)
    with multiprocessing.Pool(processes=4) as pool:
        gdfs = pool.map(work_func, query_dict.keys())
    gdfs = [gdf for gdf in gdfs if gdf is not None]
    poi_gdf = gpd.GeoDataFrame(pd.concat(gdfs, ignore_index=True))
    poi_gdf['name'] = poi_gdf.apply(lambda row: row['name:zh'] if row['name:zh'] else row['name:en'] if row['name:en'] else row['name:local'], axis=1)
    # poi_gdf = poi_gdf[poi_gdf['name'].notna()]
    poi_gdf.to_crs(epsg=3857, inplace=True)
    if (save_dir / "poi.gpkg").exists():
        (save_dir / "poi.gpkg").unlink()
    poi_gdf.to_file(save_dir / "poi.gpkg", driver="GPKG")


def query_poi(query_str:str, country_code:str):
    query = f"""
    [out:xml][timeout:25];
    area["ISO3166-1"="{country_code}"]["admin_level"="2"]->.country;
    nwr[{query_str}](area.country);
    out body;
    >;
    out skel qt;
    """

    save_dir = DATA_DIR / 'poi' / f'{country_code}'
    if not save_dir.exists():
        save_dir.mkdir(parents=True)

    query_osm_data_local(query, save_dir / f'{query_str}.osm', True)

    convert_poi_osm_2_gpkg(save_dir / f'{query_str}.osm', save_dir / f'{query_str}.gpkg','poi')
    convert_poi_2_world_cord(save_dir / f'{query_str}.gpkg', save_dir / f'{query_str}_world_coord.json')

if __name__ == "__main__":
    
    import osgeo.gdal as gdal
    import argparse
    gdal.DontUseExceptions()
    parser = argparse.ArgumentParser(description='Query POI around a position')
    parser.add_argument('pos', type=float, nargs=2, help='position (lon,lat)')
    parser.add_argument('-r','--radius', type=float, default=1000, help='radius in meters')
    parser.add_argument('-g','--ingame_coords',type=bool,default=False,help='whether to use ingame coordinates')
    args = parser.parse_args()
    pos = (args.pos[0],args.pos[1])
    radius = args.radius
    if args.ingame_coords:
        world_terrian = DATA_DIR /'world_terrain/worldmap_fill.tif'
        convert_coords.load_base_gtiff(world_terrian)
        
        gis_pos = convert_coords.convert_2_gis_coords(x=pos[0],y=pos[1],to_epsg=3857)
        transformer = pyproj.Transformer.from_crs('EPSG:3857','EPSG:4326',always_xy=True)
        x,y = transformer.transform(gis_pos[0],gis_pos[1])
        pos = (x,y)
        radius = radius*8
    res = query_poi_around_pos(pos,radius,False)
    print(json.dumps(res,ensure_ascii=False))

