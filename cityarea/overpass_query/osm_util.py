import osmium
import osmium.osm.types as osm_types
import osmium.geom
import geopandas as gpd
import pandas as pd
import shapely.wkb as wkblib
import shapely.wkt as wktlib
import shapely.geometry as geomlib
import pathlib
import subprocess
import sys
import pathlib
import requests
import os
import shutil
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
import xml.etree.ElementTree as ET

OSM_DB_DIR = pathlib.Path(
    os.environ.get('OSM_DB_DIR', '/home/pixel/geo_tools/data/osm/osm3s_db')
)
OSM3S_QUERY_EXE = os.environ.get('OSM3S_QUERY_EXE') or shutil.which('osm3s_query')

class HighwayWayHandler(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.ways = []
        self.wkbfab = osmium.geom.WKBFactory()

    def way(self, w):
        try:
            wkb = self.wkbfab.create_linestring(w)
            geom = wkblib.loads(wkb, hex=True)
            self.ways.append({
                'id': w.id,
                'highway': w.tags.get('highway'),
                'geometry': geom
            })
        except Exception as e:
            print(f"Failed to parse way {w.id}: {e}")

class HighwayRelationHandler(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.relations = []

    def relation(self, r):
        self.relations.append({
            'id': r.id,
            'highway': r.tags.get('highway'),
            'name:local': r.tags.get('name'),
            'name:en': r.tags.get('name:en') if 'name:en' in r.tags else None,
            'name:zh' : r.tags.get('name:zh') if 'name:zh' in r.tags else\
                    r.tags.get('name:ko-Hani') if 'name:ko-Hani' in r.tags else None,
            'members': [(m.type, m.ref, m.role) for m in r.members],
            'members_count': len(r.members)
        })

class RiverRelationHandler(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.relations = []

    def relation(self, r: osm_types.Relation):
        self.relations.append({
            'id': r.id,
            'waterway': r.tags.get('waterway'),
            'name:local': r.tags.get('name'),
            'name:en': r.tags.get('name:en') if 'name:en' in r.tags else None,
            'name:zh' : r.tags.get('name:zh') if 'name:zh' in r.tags else\
                    r.tags.get('name:ko-Hani') if 'name:ko-Hani' in r.tags else None,
            'members': [(m.type, m.ref, m.role) for m in r.members],
            'members_count': len(r.members)
        })

class RiverWayHandler(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.ways = []
        self.wkbfab = osmium.geom.WKBFactory()
    def way(self, w: osm_types.Way):
        try:
            wkb = self.wkbfab.create_linestring(w)
            geom = wkblib.loads(wkb, hex=True)
            self.ways.append({
                'id': w.id,
                'geometry': geom
            })
        except Exception as e:
            print(f"Failed to parse way {w.id}: {e}")

class RiverWayHandlerAlt(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.ways = []
        self.wkbfab = osmium.geom.WKBFactory()
    def way(self, w: osm_types.Way):
        try:
            wkb = self.wkbfab.create_linestring(w)
            geom = wkblib.loads(wkb, hex=True)
            self.ways.append({
                'id': w.id,
                'waterway': w.tags.get('waterway'),
                'name:local': w.tags.get('name'),
                'name:en': w.tags.get('name:en') if 'name:en' in w.tags else None,
                'name:zh' : w.tags.get('name:zh') if 'name:zh' in w.tags else\
                    w.tags.get('name:ko-Hani') if 'name:ko-Hani' in w.tags else None,
                'geometry': geom
            })
        except Exception as e:
            print(f"Failed to parse way {w.id}: {e}")

class CityHandler(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.nodes = []
        self.ways = []
        self.nodes_dict = {}
        self.wkbfab = osmium.geom.WKBFactory()

    def node(self, n: osm_types.Node):
        if "place" in n.tags or "amenity" in n.tags:
            wkb = self.wkbfab.create_point(n)
            geom = wkblib.loads(wkb, hex=True)
            self.nodes.append({
                'id': n.id,
                'lat': n.location.lat,
                'lon': n.location.lon,
                'type':n.tags.get('place') if "place" in n.tags else n.tags.get('amenity') if "amenity" in n.tags else None,
                'name:local': n.tags.get('name'),
                'name:en': n.tags.get('name:en') if 'name:en' in n.tags else None,
                'name:zh' : n.tags.get('name:zh') if 'name:zh' in n.tags else\
                    n.tags.get('name:ko-Hani') if 'name:ko-Hani' in n.tags else None,
                'geometry': geom
            })
        # self.nodes_dict[n.id] = n

    def way(self, w: osm_types.Way):
        if "highway" in w.tags:
            heighway_type = w.tags.get('highway')
            if heighway_type in ['motorway', 'trunk', 'primary', 'secondary', \
                                'tertiary', 'road', 'unclassified', 'track', 'residential', 'path']:
                try:
                    # members = [m.ref for m in w.nodes]
                    # if len(members) >1 :
                    #     nodes = [self.nodes_dict[m] for m in members]
                    wkb = self.wkbfab.create_linestring(w)
                    geom = wkblib.loads(wkb, hex=True)
                    self.ways.append({
                        'id': w.id,
                        'highway': heighway_type,
                        'geometry': geom,
                        'name:local': w.tags.get('name'),
                        'name:en': w.tags.get('name:en') if 'name:en' in w.tags else None,
                        'name:zh' : w.tags.get('name:zh') if 'name:zh' in w.tags else\
                            w.tags.get('name:ko-Hani') if 'name:ko-Hani' in w.tags else None
                    })
                except Exception as e:
                    print(f"Failed to parse way {w.id}: {e}")

class POIHandler(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.features = []
        self.wkbfab = osmium.geom.WKBFactory()
        self.wktfab = osmium.geom.WKTFactory()

    def node(self, n: osm_types.Node):
        if n.tags:
            wkb = self.wkbfab.create_point(n)
            geom = wkblib.loads(wkb, hex=True)
            self.features.append({
                'id': n.id,
                'name:local': n.tags.get('name'),
                'name:en': n.tags.get('name:en') if 'name:en' in n.tags else None,
                'name:zh' : n.tags.get('name:zh') if 'name:zh' in n.tags else\
                        n.tags.get('name:ko-Hani') if 'name:ko-Hani' in n.tags else None,
                'geometry': geom
            })

    def way(self, w: osm_types.Way):
        if w.tags:
            try:
                if not w.is_closed():
                    wkb = self.wkbfab.create_linestring(w)
                    geom = wkblib.loads(wkb, hex=True)
                    self.features.append({
                        'id': w.id,
                        'geometry': geom,
                        'name:local': w.tags.get('name'),
                        'name:en': w.tags.get('name:en') if 'name:en' in w.tags else None,
                        'name:zh' : w.tags.get('name:zh') if 'name:zh' in w.tags else\
                                w.tags.get('name:ko-Hani') if 'name:ko-Hani' in w.tags else None
                    })
            except Exception as e:
                print(f"Failed to parse way {w.id}: {e}")

    def area(self, a: osm_types.Area):
        if a.tags:
            try:
                wkb = self.wkbfab.create_multipolygon(a)
                geom = wkblib.loads(wkb, hex=True)
                self.features.append({
                    'id': a.id,
                    'geometry': geom,
                    'name:local': a.tags.get('name'),
                    'name:en': a.tags.get('name:en') if 'name:en' in a.tags else None,
                    'name:zh' : a.tags.get('name:zh') if 'name:zh' in a.tags else\
                            a.tags.get('name:ko-Hani') if 'name:ko-Hani' in a.tags else None
                })
            except Exception as e:
                print(f"Failed to parse area {a.id}: {e}")

    # def relation(self, r: osm_types.Relation):
    #     if r.tags:
    #         try:
    #             wkb = self.wkbfab.create_multipolygon(r)
    #             geom = wkblib.loads(wkb, hex=True)
    #             self.features.append({
    #                 'id': r.id,
    #                 'geometry': geom,
    #                 'name:local': r.tags.get('name'),
    #                 'name:en': r.tags.get('name:en') if 'name:en' in r.tags else None,
    #                 'name:zh' : r.tags.get('name:zh') if 'name:zh' in r.tags else\
    #                         r.tags.get('name:ko-Hani') if 'name:ko-Hani' in r.tags else None
    #             })
    #         except Exception as e:
    #             print(f"Failed to parse relation {r.id}: {e}")

class LanduseHandler(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.features = []
        self.wkbfab = osmium.geom.WKBFactory()

    def way(self, w: osm_types.Way):
        if 'landuse' in w.tags and w.is_closed():
            try:
                if hasattr(self.wkbfab, 'create_polygon'):
                    wkb = self.wkbfab.create_polygon(w)
                    geom = wkblib.loads(wkb, hex=True)
                else:
                    wkb = self.wkbfab.create_linestring(w)
                    line = wkblib.loads(wkb, hex=True)
                    coords = list(line.coords)
                    if len(coords) < 4:
                        return
                    if coords[0] != coords[-1]:
                        coords.append(coords[0])
                    geom = geomlib.Polygon(coords)
                    if not geom.is_valid:
                        geom = geom.buffer(0)
                    if geom.is_empty:
                        return
                self.features.append({
                    'id': w.id,
                    'osm_type': 'way',
                    'landuse': w.tags.get('landuse'),
                    'name:local': w.tags.get('name'),
                    'name:en': w.tags.get('name:en') if 'name:en' in w.tags else None,
                    'name:zh': w.tags.get('name:zh') if 'name:zh' in w.tags else \
                        w.tags.get('name:ko-Hani') if 'name:ko-Hani' in w.tags else None,
                    'geometry': geom
                })
            except Exception as e:
                print(f"Failed to parse landuse way {w.id}: {e}")

    def area(self, a: osm_types.Area):
        if 'landuse' in a.tags:
            try:
                wkb = self.wkbfab.create_multipolygon(a)
                geom = wkblib.loads(wkb, hex=True)
                self.features.append({
                    'id': a.id,
                    'osm_type': 'area',
                    'landuse': a.tags.get('landuse'),
                    'name:local': a.tags.get('name'),
                    'name:en': a.tags.get('name:en') if 'name:en' in a.tags else None,
                    'name:zh': a.tags.get('name:zh') if 'name:zh' in a.tags else \
                        a.tags.get('name:ko-Hani') if 'name:ko-Hani' in a.tags else None,
                    'geometry': geom
                })
            except Exception as e:
                print(f"Failed to parse landuse area {a.id}: {e}")

class AdminHandler(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.relations = []
        self.ways=[]
        self.wkbfab = osmium.geom.WKBFactory()

    def relation(self, r: osm_types.Relation):
        if 'admin_level' in r.tags:
            self.relations.append({
                'id': r.id,
                'name:local': r.tags.get('name'),
                'name:en': r.tags.get('name:en'),
                'name:zh' : r.tags.get('name:zh'),
                'admin_level': r.tags.get('admin_level'),
                'members': [(m.type, m.ref, m.role) for m in r.members],
                'members_count': len(r.members)
            })

    def way(self, w: osm_types.Way):
        if 'admin_level' in w.tags:
            try:
                wkb = self.wkbfab.create_linestring(w)
                geom = wkblib.loads(wkb, hex=True)
                self.ways.append({
                    'id': w.id,
                    'admin_level': w.tags.get('admin_level'),
                    'geometry': geom
                })
            except Exception as e:
                print(f"Failed to parse way {w.id}: {e}")

def query_osm_data_local(query: str, output_file: pathlib.Path, log: bool = False):
    if OSM3S_QUERY_EXE is None:
        raise FileNotFoundError('osm3s_query is not found. Set OSM3S_QUERY_EXE or add it to PATH.')
    if not OSM_DB_DIR.exists():
        raise FileNotFoundError(f'OSM_DB_DIR does not exist: {OSM_DB_DIR}')

    cmd = [
        OSM3S_QUERY_EXE,
        f'--db-dir={OSM_DB_DIR}',
    ]
    process = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout_data, stderr_data = process.communicate(query.encode('utf-8'))
    if process.returncode != 0:
        if log:
            print(f"❌  Failed to query OSM data: {query}")
            print(f"    stdout: {stdout_data.decode('utf-8')}")
            print(f"    stderr: {stderr_data.decode('utf-8')}")
    else:
        root = ET.fromstring(stdout_data)
        #sort the elements in the xml file
        nodes = [elem for elem in root.findall('node')]
        ways = [elem for elem in root.findall('way')]
        relations = [elem for elem in root.findall('relation')]
        other_elements = [elem for elem in root.findall('*') if elem.tag not in ['node', 'way','relation']]

        nodes.sort(key=lambda x: int(x.get('id')))

        ways.sort(key=lambda x: int(x.get('id')))
        relations.sort(key=lambda x: int(x.get('id')))

        for elem in list(root):
            root.remove(elem)

        for elem in other_elements:
            root.append(elem)

        seen_nodes = set()
        seen_ways = set()
        seen_relations = set()
        for node in nodes:
            if node.get('id') not in seen_nodes:
                root.append(node)
                seen_nodes.add(node.get('id'))

        for way in ways:
            if way.get('id') not in seen_ways:
                root.append(way)
                seen_ways.add(way.get('id'))

        for relation in relations:
            if relation.get('id') not in seen_relations:
                root.append(relation)
                seen_relations.add(relation.get('id'))

        tree = ET.ElementTree(root)
        tree.write(output_file, encoding='utf-8', xml_declaration=True)
        if log:
            print(f"✅  Query OSM data successful: {query}")
    return process.returncode

def query_osm_data_local_no_output(query: str, log: bool = False):
    if OSM3S_QUERY_EXE is None:
        raise FileNotFoundError('osm3s_query is not found. Set OSM3S_QUERY_EXE or add it to PATH.')
    if not OSM_DB_DIR.exists():
        raise FileNotFoundError(f'OSM_DB_DIR does not exist: {OSM_DB_DIR}')

    cmd = [
        OSM3S_QUERY_EXE,
        f'--db-dir={OSM_DB_DIR}',
    ]
    process = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout_data, stderr_data = process.communicate(query.encode('utf-8'))
    if process.returncode != 0:
        if log:
            print(f"❌  Failed to query OSM data: {query}")
            print(f"    stdout: {stdout_data.decode('utf-8')}")
            print(f"    stderr: {stderr_data.decode('utf-8')}")
    else:
        if log:
            print(f"✅  Query OSM data successful: {query}")
    return process.returncode, stdout_data.decode('utf-8')

def osm_result_empty(osm_file: pathlib.Path):
    if not osm_file.exists():
        return True
    xml_root = ET.parse(str(osm_file)).getroot()
    for child in xml_root:
        if child.tag == 'node' or child.tag == 'way' or child.tag =='relation':
            return False
    return True

def query_osm_data_remote(query: str, output_file: pathlib.Path, log: bool = False):
    try:
        response = requests.get(OVERPASS_URL, params={'data': query}, headers=HTTP_HEADERS, timeout=180)
    except requests.RequestException as e:
        if log:
            print(f"❌  Failed to query OSM data (remote request error): {e}")
        return 500
    if response.status_code != 200:
        if log:
            print(f"❌  Failed to query OSM data: {query}")
            print(f"    status code: {response.status_code}")
            print(f"    response: {response.text}")
    else:
        with open(output_file, 'w',encoding='utf-8') as f:
            f.write(response.text)
            if log:
                print(f"✅  Query OSM data successful: {query}")
    return response.status_code

def convert_river_osm_2_gpkg(input_relation_osm_file: pathlib.Path, input_way_osm_file: pathlib.Path, output_gpkg_file: pathlib.Path):
    way_handler = RiverWayHandler()
    relation_handler = RiverRelationHandler()

    way_handler.apply_file(str(input_way_osm_file))
    relation_handler.apply_file(str(input_relation_osm_file))

    gdf_ways = gpd.GeoDataFrame(way_handler.ways)
    df_relations = pd.DataFrame(relation_handler.relations)
    geometry_list = []
    #gdf_relations.set_geometry('geometry', inplace=True)
    gdf_ways.set_crs(epsg=4326, inplace=True)
    for i, row in df_relations.iterrows():
        relation_id = row["id"]
        members = row['members']
        geom_list = []
        for type, ref, role in members:
            if type == 'w':
                way = gdf_ways[gdf_ways['id']==ref]
                if len(way)!= 1:
                    print(f"Way {ref} not found in ways dataframe, skipping")
                    continue
                way = way.iloc[0]
                geom_list.append(way.geometry)
            else:
                print(f"Unsupported member type {type}, skipping")
        if len(geom_list) > 1:
            geometry_list.append(gpd.GeoSeries(geom_list).union_all())
        elif len(geom_list) == 1:
            geometry_list.append(geom_list[0])

    gdf_relations = gpd.GeoDataFrame(df_relations,geometry=geometry_list, crs=gdf_ways.crs)
    gdf_relations.to_file(str(output_gpkg_file), layer='river', driver='GPKG')


def convert_road_osm_2_gpkg(input_relation_osm_file: pathlib.Path, input_way_osm_file: pathlib.Path, output_gpkg_file: pathlib.Path):
    way_handler = HighwayWayHandler()
    relation_handler = HighwayRelationHandler()

    way_handler.apply_file(str(input_way_osm_file))
    relation_handler.apply_file(str(input_relation_osm_file))

    gdf_ways = gpd.GeoDataFrame(way_handler.ways)
    df_relations = pd.DataFrame(relation_handler.relations)
    geometry_list = []
    # gdf_bridges  = gpd.GeoDataFrame(columns=[*df_relations.columns,'geometry'],crs=gdf_ways.crs)
    #gdf_relations.set_geometry('geometry', inplace=True)
    gdf_ways.set_crs(epsg=4326, inplace=True)
    for i, row in df_relations.iterrows():
        relation_id = row["id"]
        members = row['members']
        geom_list = []
        for type, ref, role in members:
            if type == 'w':
                # if role :
                    way = gdf_ways[gdf_ways['id']==ref]
                    if len(way)!= 1:
                        continue
                    way = way.iloc[0]
                    if row['highway'] is None:
                        df_relations.at[i, 'highway'] = way['highway']
                    geom_list.append(way.geometry)
        if len(geom_list) > 1:
            geometry_list.append(gpd.GeoSeries(geom_list).union_all())
        elif len(geom_list) == 1:
            geometry_list.append(geom_list[0])
        else:
            geometry_list.append(None)

    gdf_relations = gpd.GeoDataFrame(df_relations,geometry=geometry_list, crs=gdf_ways.crs)
    # gdf_bridges.set_crs(epsg=4326, inplace=True)
    gdf_relations.to_file(str(output_gpkg_file), layer='road', driver='GPKG')
    # gdf_bridges.to_file(str(output_gpkg_file), layer='bridge', driver='GPKG')

def convert_river_osm_2_gpkg_alt(input_osm_file: pathlib.Path, output_gpkg_file: pathlib.Path):
    way_handler = RiverWayHandlerAlt()

    way_handler.apply_file(str(input_osm_file))

    gdf_ways = gpd.GeoDataFrame(way_handler.ways)
    gdf_ways.set_crs(epsg=4326, inplace=True)
    gdf_ways.to_file(str(output_gpkg_file), layer='river', driver='GPKG')

def convert_city_osm_2_gpkg(input_osm_file: pathlib.Path, output_gpkg_file: pathlib.Path):
    city_handler = CityHandler()

    city_handler.apply_file(str(input_osm_file), locations=True)

    gdf_nodes = gpd.GeoDataFrame(city_handler.nodes)
    gdf_ways = gpd.GeoDataFrame(city_handler.ways)
    gdf_nodes.set_crs(epsg=4326, inplace=True)
    gdf_ways.set_crs(epsg=4326, inplace=True)
    gdf_nodes.to_file(str(output_gpkg_file), layer='city_place', driver='GPKG')
    gdf_ways.to_file(str(output_gpkg_file), layer='city_road', driver='GPKG')

def convert_poi_osm_2_gpkg(input_osm_file: pathlib.Path, output_gpkg_file: pathlib.Path, type: str):
    poi_handler = POIHandler()
    if osm_result_empty(input_osm_file):
        return

    # fp = (osmium.FileProcessor(str(input_osm_file)).with_locations("flex_mem").with_areas())
    # # for o in fp:
    # #     if o.is_area():
    # #         poi_handler.area(o)
    # #     elif o.is_node():
    # #         poi_handler.node(o)
    # #     elif o.is_way():
    # #         poi_handler.way(o)
    # #     elif o.is_relation():
    # #         poi_handler.relation(o)
    # fp.header=poi_handler

    poi_handler.apply_file(str(input_osm_file), locations=True)

    gdf_nodes = gpd.GeoDataFrame(poi_handler.features)
    gdf_nodes.set_crs(epsg=4326, inplace=True)
    gdf_nodes.geometry = gdf_nodes.geometry.centroid
    gdf_nodes.to_file(str(output_gpkg_file), layer=type, driver='GPKG')


def convert_landuse_osm_2_gpkg(input_osm_file: pathlib.Path, output_gpkg_file: pathlib.Path, layer_name: str = 'landuse'):
    if osm_result_empty(input_osm_file):
        return

    landuse_handler = LanduseHandler()
    landuse_handler.apply_file(str(input_osm_file), locations=True)

    if len(landuse_handler.features) == 0:
        return

    gdf = gpd.GeoDataFrame(landuse_handler.features)
    gdf.set_crs(epsg=4326, inplace=True)
    gdf = gdf[gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
    if len(gdf) == 0:
        return

    gdf = gdf.reset_index(drop=True)
    gdf['landuse_area_m2'] = gdf.to_crs(epsg=3857).geometry.area
    gdf.to_file(str(output_gpkg_file), layer=layer_name, driver='GPKG')
