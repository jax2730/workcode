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
import re

def _resolve_osm_db_dir() -> pathlib.Path:
    env_dir = os.environ.get('OSM_DB_DIR')
    if env_dir:
        return pathlib.Path(env_dir)

    candidates = [
        DATA_DIR / 'osm' / 'osm3s_db',
        pathlib.Path('/home/pixel/geo_tools/data/osm/osm3s_db'),
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0]


def _resolve_osm3s_query_exe() -> str | None:
    env_exe = os.environ.get('OSM3S_QUERY_EXE')
    if env_exe:
        return env_exe

    found = shutil.which('osm3s_query')
    if found:
        return found

    candidates = [
        '/usr/bin/osm3s_query',
        '/usr/local/bin/osm3s_query',
        '/home/pixel/geo_tools/bin/osm3s_query',
        '/home/pixel/geo_tools/osm3s/bin/osm3s_query',
        '/home/pixel/osm3s/bin/osm3s_query',
    ]
    for candidate in candidates:
        if pathlib.Path(candidate).exists():
            return candidate
    return None


OSM_DB_DIR = _resolve_osm_db_dir()
OSM3S_QUERY_EXE = _resolve_osm3s_query_exe()

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
                                'tertiary', 'road', 'unclassified', 'track', 'path']:
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


class BuildingHandler(osmium.SimpleHandler):
    """Handler for extracting building data from OSM.

    Handles:
    - Building ways (closed ways with building tag)
    - Building areas (multipolygons with building tag)
    - Extracts: building type, name, levels, area
    """
    def __init__(self):
        super().__init__()
        self.features = []
        self.wkbfab = osmium.geom.WKBFactory()

    def way(self, w: osm_types.Way):
        """Process building ways (must be closed)."""
        if w.tags and 'building' in w.tags:
            try:
                if w.is_closed():
                    # Try to create polygon
                    try:
                        wkb = self.wkbfab.create_polygon(w)
                    except:
                        # Fallback to linestring + manual polygon
                        wkb = self.wkbfab.create_linestring(w)

                    geom = wkblib.loads(wkb, hex=True)

                    # Convert LineString to Polygon if needed
                    if geom.geom_type == 'LineString' and geom.is_ring:
                        geom = geomlib.Polygon(geom.coords[:-1])

                    if geom.geom_type in ['Polygon', 'MultiPolygon']:
                        self.features.append({
                            'id': w.id,
                            'building': w.tags.get('building'),
                            'building:part': w.tags.get('building:part'),
                            'building:use': w.tags.get('building:use'),
                            'building:levels': w.tags.get('building:levels'),
                            'height': w.tags.get('height'),
                            'name:local': w.tags.get('name'),
                            'name:en': w.tags.get('name:en'),
                            'name:zh': w.tags.get('name:zh'),
                            'amenity': w.tags.get('amenity'),
                            'shop': w.tags.get('shop'),
                            'office': w.tags.get('office'),
                            'tourism': w.tags.get('tourism'),
                            'leisure': w.tags.get('leisure'),
                            'man_made': w.tags.get('man_made'),
                            'geometry': geom
                        })
            except Exception as e:
                print(f"Failed to parse building way {w.id}: {e}")

    def area(self, a: osm_types.Area):
        """Process building areas."""
        if a.tags and 'building' in a.tags:
            try:
                wkb = self.wkbfab.create_multipolygon(a)
                geom = wkblib.loads(wkb, hex=True)

                # Ensure it's a proper polygon
                if geom.geom_type in ['Polygon', 'MultiPolygon']:
                    self.features.append({
                        'id': a.id,
                        'building': a.tags.get('building'),
                        'building:part': a.tags.get('building:part'),
                        'building:use': a.tags.get('building:use'),
                        'building:levels': a.tags.get('building:levels'),
                        'height': a.tags.get('height'),
                        'name:local': a.tags.get('name'),
                        'name:en': a.tags.get('name:en'),
                        'name:zh': a.tags.get('name:zh'),
                        'amenity': a.tags.get('amenity'),
                        'shop': a.tags.get('shop'),
                        'office': a.tags.get('office'),
                        'tourism': a.tags.get('tourism'),
                        'leisure': a.tags.get('leisure'),
                        'man_made': a.tags.get('man_made'),
                        'geometry': geom
                    })
            except Exception as e:
                print(f"Failed to parse building area {a.id}: {e}")


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
    if log:
        print(f"ℹ️  Local OSM query exe: {OSM3S_QUERY_EXE}")
        print(f"ℹ️  Local OSM db: {OSM_DB_DIR}")
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

    response = requests.get(OVERPASS_URL, params={'data': query}, headers=HTTP_HEADERS)
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


def convert_building_osm_2_gpkg(input_osm_file: pathlib.Path, output_gpkg_file: pathlib.Path, log: bool = False) -> bool:
    """Convert OSM building data to GPKG with POI-like multi-layer output.

    Args:
        input_osm_file: Path to OSM file with building data
        output_gpkg_file: Path to output GPKG file
    """
    if osm_result_empty(input_osm_file):
        if log:
            print(f"❌  OSM result is empty: {input_osm_file}")
        return False

    building_handler = BuildingHandler()
    building_handler.apply_file(str(input_osm_file), locations=True)

    if len(building_handler.features) == 0:
        if log:
            print("❌  No building features parsed from OSM file")
        return False

    gdf = gpd.GeoDataFrame(building_handler.features)
    gdf.set_crs(epsg=4326, inplace=True)

    # Filter to valid polygons only
    gdf = gdf[gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
    if len(gdf) == 0:
        if log:
            print("❌  No polygon building geometries after filtering")
        return False

    gdf = gdf.reset_index(drop=True)

    # Remove building parts to reduce fragmented small pieces
    if 'building:part' in gdf.columns:
        gdf = gdf[gdf['building:part'].isna()].copy()
    if len(gdf) == 0:
        if log:
            print("❌  All geometries are building parts; nothing to export")
        return False

    # Keep original OSM type as grouping key
    gdf['building_type'] = gdf.apply(_derive_raw_building_type, axis=1)

    # Normalize display name for merge (name in QGIS attributes)
    gdf['name'] = gdf.apply(_derive_display_name, axis=1)

    # Calculate area in square meters
    gdf['area_m2'] = gdf.to_crs(epsg=3857).geometry.area

    # Save as POI-like multi-layer GPKG (one layer per building_type)
    output_gpkg_file.parent.mkdir(parents=True, exist_ok=True)
    if output_gpkg_file.exists():
        output_gpkg_file.unlink()

    layer_count = 0
    try:
        grouped = gdf.groupby('building_type', dropna=False)
        for building_type, type_gdf in grouped:
            if len(type_gdf) == 0:
                continue

            merged = _merge_named_polygons(type_gdf)
            if len(merged) == 0:
                continue

            layer_name = _sanitize_layer_name(str(building_type))
            if layer_count == 0:
                merged.to_file(str(output_gpkg_file), layer=layer_name, driver='GPKG')
            else:
                merged.to_file(str(output_gpkg_file), layer=layer_name, driver='GPKG', mode='a')
            layer_count += 1
    except Exception as error:
        if log:
            print(f"❌  Failed to write GPKG: {output_gpkg_file}")
            print(f"    error: {error}")
        return False

    if layer_count == 0:
        if log:
            print("❌  No building layers generated")
        return False

    print(f"Saved {len(gdf)} buildings into {layer_count} layers: {output_gpkg_file}")
    return True


def _derive_display_name(row: pd.Series) -> str | None:
    for key in ['name:local', 'name:zh', 'name:en']:
        value = row.get(key)
        if pd.notna(value):
            text = str(value).strip()
            if text:
                return text
    return None


def _merge_named_polygons(type_gdf: gpd.GeoDataFrame) -> gpd.GeoDataFrame:
    """Merge polygons with the same name to form larger, complete blocks."""
    df = type_gdf.copy()
    named = df[df['name'].notna()].copy()
    unnamed = df[df['name'].isna()].copy()

    frames = []
    if len(named) > 0:
        dissolved = named.dissolve(by='name', as_index=False)
        dissolved['name:local'] = dissolved['name']
        dissolved['name:zh'] = dissolved['name']
        dissolved['name:en'] = dissolved['name']
        dissolved = dissolved[['name', 'name:local', 'name:zh', 'name:en', 'geometry']]
        dissolved = dissolved[dissolved.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
        dissolved['area_m2'] = dissolved.to_crs(epsg=3857).geometry.area
        frames.append(dissolved)

    if len(unnamed) > 0:
        keep_cols = [c for c in ['name', 'name:local', 'name:zh', 'name:en', 'geometry'] if c in unnamed.columns]
        unnamed = unnamed[keep_cols].copy()
        unnamed['area_m2'] = unnamed.to_crs(epsg=3857).geometry.area
        frames.append(unnamed)

    if len(frames) == 0:
        return gpd.GeoDataFrame(columns=['name', 'name:local', 'name:zh', 'name:en', 'area_m2', 'geometry'], crs=type_gdf.crs)

    out = pd.concat(frames, ignore_index=True)
    out = gpd.GeoDataFrame(out, geometry='geometry', crs=type_gdf.crs)
    out = out[out.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
    return out


def _sanitize_layer_name(raw_name: str) -> str:
    text = (raw_name or 'unknown').strip()
    if text == '':
        text = 'unknown'
    text = re.sub(r'[^0-9a-zA-Z_]+', '_', text)
    text = re.sub(r'_+', '_', text).strip('_')
    if text == '':
        text = 'unknown'
    return text[:50]


def _derive_raw_building_type(row: pd.Series) -> str:
    """Derive building type directly from queried OSM tags.

    Priority:
    1) building tag when it is meaningful (not empty/yes)
    2) amenity/shop/office/tourism/leisure/man_made/building:use
    3) fallback to 'yes' or 'unknown'
    """
    building = row.get('building')
    amenity = row.get('amenity')
    shop = row.get('shop')
    office = row.get('office')
    tourism = row.get('tourism')
    leisure = row.get('leisure')
    man_made = row.get('man_made')
    building_use = row.get('building:use')

    if pd.notna(building):
        building_value = str(building).strip()
        if building_value and building_value.lower() != 'yes':
            return building_value

    for value in [amenity, shop, office, tourism, leisure, man_made, building_use]:
        if pd.notna(value):
            candidate = str(value).strip()
            if candidate:
                return candidate

    if pd.notna(building):
        building_value = str(building).strip()
        if building_value:
            return building_value

    return 'unknown'
