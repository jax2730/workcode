import osmium
import osmium.osm.types as osm_types
import osmium.geom
import geopandas as gpd
import pandas as pd
import shapely.wkb as wkblib
import shapely.wkt as wktlib
import shapely.ops as ops
import pathlib
import subprocess
import sys
import pathlib
import requests
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
import xml.etree.ElementTree as ET
from lxml import etree

OSM_DB_DIR = pathlib.Path('/home/pixel/geo_tools/data/osm/osm3s_db')

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

class SubwayWayHandler(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.ways = []
        self.wkbfab = osmium.geom.WKBFactory()

    def way(self, w):
        try:
            if w.tags.get("railway") != "subway":
                return

            wkb = self.wkbfab.create_linestring(w)
            geom = wkblib.loads(wkb)

            self.ways.append({
                "id": w.id,
                "name": w.tags.get("name"),
                "ref": w.tags.get("ref"),
                "railway": w.tags.get("railway"),
                "tunnel": w.tags.get("tunnel"),
                "geometry": geom
            })

        except Exception as e:
            print(f"Failed way {w.id}: {e}")

class SubwayRelationHandler(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.relations = []

    def relation(self, r):

        if r.tags.get("route") != "subway":
            return

        self.relations.append({
            "id": r.id,
            "ref": r.tags.get("ref"),
            "name": r.tags.get("name"),
            "network": r.tags.get("network"),
            "operator": r.tags.get("operator"),

            "members": [(m.type, m.ref, m.role) for m in r.members],
            "members_count": len(r.members)
        })

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
                                'tertiary', 'road', 'unclassified', 'residential', 'track', 'path']:
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
        self.way_geoms = {}

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
        try:
            wkb = self.wkbfab.create_linestring(w)
            geom = wkblib.loads(wkb, hex=True)
            self.way_geoms[w.id] = geom
            if w.tags:
                if not w.is_closed():
                    
                    self.features.append({
                        'id': w.id,
                        'geometry': geom,
                        'name:local': w.tags.get('name'),
                        'name:en': w.tags.get('name:en') if 'name:en' in w.tags else None,
                        'name:zh' : w.tags.get('name:zh') if 'name:zh' in w.tags else\
                                w.tags.get('name:ko-Hani') if 'name:ko-Hani' in w.tags else None
                    })
        except Exception as e:
            print(f"// Failed to parse way {w.id}: {e}\r\n")
            pass

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
                print(f"// Failed to parse area {a.id}: {e}\r\n")
                pass

    def relation(self, r: osm_types.Relation):
        if r.tags:
            try:
                if r.tags.get('type') in ('multipolygon', 'boundary'):
                    return
                if r.tags.get('type') == 'site':
                    members_to_union = []
                    for m in r.members:
                        if m.ref in self.way_geoms:
                            members_to_union.append(self.way_geoms[m.ref])
                    if members_to_union:
                        geom = ops.unary_union(members_to_union)
                    else:
                        geom = None
                else:
                    wkb = self.wkbfab.create_multipolygon(r)
                    geom = wkblib.loads(wkb, hex=True)
                self.features.append({
                    'id': r.id,
                    'geometry': geom,
                    'name:local': r.tags.get('name'),
                    'name:en': r.tags.get('name:en') if 'name:en' in r.tags else None,
                    'name:zh' : r.tags.get('name:zh') if 'name:zh' in r.tags else\
                            r.tags.get('name:ko-Hani') if 'name:ko-Hani' in r.tags else None
                })
            except Exception as e:
                print(f"// Failed to parse relation {r.id}: {e} \r\n")
                pass

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
    cmd = [
        'osm3s_query',
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
        print(f"➡️ Saving OSM data to {output_file}")
        if len(stdout_data) == 0:
            print(f"❌  Query OSM data returned no data: {query}")
            return 1
        
        parser = etree.XMLParser(remove_blank_text=True)
        root = etree.fromstring(stdout_data, parser)
        
        # 分类提取
        # 使用 xpath 一次性获取所有元素类型
        elements_map = {
            'node': [],
            'way': [],
            'relation': [],
            'other': []
        }
        
        for elem in root:
            tag = elem.tag
            if tag in elements_map:
                elements_map[tag].append(elem)
            else:
                elements_map['other'].append(elem)

        # 清空根节点以重新构建
        for elem in list(root):
            root.remove(elem)

        # 添加非数据节点 (如 osm 标签里的 note, meta 等)
        for elem in elements_map['other']:
            root.append(elem)

        # 定义去重和排序的闭包函数
        def append_sorted_unique(tag_name):
            seen_ids = set()
            # 排序：按 ID 转为整数排序
            sorted_list = sorted(
                elements_map[tag_name], 
                key=lambda x: int(x.get('id', 0))
            )
            for item in sorted_list:
                obj_id = item.get('id')
                if obj_id not in seen_ids:
                    root.append(item)
                    seen_ids.add(obj_id)

        # 按 OSM 标准顺序写入：Node -> Way -> Relation
        append_sorted_unique('node')
        append_sorted_unique('way')
        append_sorted_unique('relation')

        # 写入文件
        with open(output_file, 'wb') as f:
            f.write(etree.tostring(root, pretty_print=True, xml_declaration=True, encoding='utf-8'))
            
        print(f"✅  Query OSM data successful: {query}")
    return process.returncode

def query_osm_data_local_str(query: str, log: bool = False)->str:
    cmd = [
        'osm3s_query',
        f'--db-dir={OSM_DB_DIR}',
    ]
    process = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout_data, stderr_data = process.communicate(query.encode('utf-8'))
    if process.returncode != 0:
        if log:
            print(f"❌  Failed to query OSM data: {query}")
            print(f"    stdout: {stdout_data.decode('utf-8')}")
            print(f"    stderr: {stderr_data.decode('utf-8')}")
            return ""
    else:
        print(f"➡️ reformat OSM data to string") if log else None
        if len(stdout_data) == 0:
            print(f"❌  Query OSM data returned no data: {query}") if log else None
            return ""
        
        parser = etree.XMLParser(remove_blank_text=True)
        root = etree.fromstring(stdout_data, parser)
        
        # 分类提取
        # 使用 xpath 一次性获取所有元素类型
        elements_map = {
            'node': [],
            'way': [],
            'relation': [],
            'other': []
        }
        
        for elem in root:
            tag = elem.tag
            if tag in elements_map:
                elements_map[tag].append(elem)
            else:
                elements_map['other'].append(elem)

        # 清空根节点以重新构建
        for elem in list(root):
            root.remove(elem)

        # 添加非数据节点 (如 osm 标签里的 note, meta 等)
        for elem in elements_map['other']:
            root.append(elem)

        # 定义去重和排序的闭包函数
        def append_sorted_unique(tag_name):
            seen_ids = set()
            # 排序：按 ID 转为整数排序
            sorted_list = sorted(
                elements_map[tag_name], 
                key=lambda x: int(x.get('id', 0))
            )
            for item in sorted_list:
                obj_id = item.get('id')
                if obj_id not in seen_ids:
                    root.append(item)
                    seen_ids.add(obj_id)

        # 按 OSM 标准顺序写入：Node -> Way -> Relation
        append_sorted_unique('node')
        append_sorted_unique('way')
        append_sorted_unique('relation')
        if len(elements_map['node']) == 0 and len(elements_map['way']) == 0 and len(elements_map['relation']) == 0:
            return ""

        return etree.tostring(root, pretty_print=True, xml_declaration=True, encoding='utf-8')

def query_osm_data_local_no_output(query: str, log: bool = False):
    cmd = [
        'osm3s_query',
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

def convert_subway_osm_2_gpkg(input_relation_osm_file: pathlib.Path, input_way_osm_file: pathlib.Path, output_gpkg_file: pathlib.Path):
    way_handler = SubwayWayHandler()
    relation_handler = SubwayRelationHandler()

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
                    geom_list.append(way.geometry)
        if len(geom_list) > 1:
            geometry_list.append(gpd.GeoSeries(geom_list).union_all())
        elif len(geom_list) == 1:
            geometry_list.append(geom_list[0])
        else:
            geometry_list.append(None)

    gdf_relations = gpd.GeoDataFrame(df_relations,geometry=geometry_list, crs=gdf_ways.crs)
    gdf_relations.to_file(str(output_gpkg_file), layer='subway', driver='GPKG')


def convert_river_osm_2_gpkg_alt(input_osm_file: pathlib.Path, output_gpkg_file: pathlib.Path):
    way_handler = RiverWayHandlerAlt()

    way_handler.apply_file(str(input_osm_file))

    gdf_ways = gpd.GeoDataFrame(way_handler.ways)
    gdf_ways.set_crs(epsg=4326, inplace=True)
    gdf_ways.to_file(str(output_gpkg_file), layer='river', driver='GPKG')

def convert_city_osm_2_gpkg(input_osm_file: pathlib.Path, output_gpkg_file: pathlib.Path):
    city_handler = CityHandler()

    city_handler.apply_file(str(input_osm_file), locations=True)

    gdf_nodes = gpd.GeoDataFrame(city_handler.nodes, geometry='geometry', crs='EPSG:4326')
    gdf_ways = gpd.GeoDataFrame(city_handler.ways, geometry='geometry', crs='EPSG:4326')
    if output_gpkg_file.exists():
        output_gpkg_file.unlink()
    # gdf_nodes.to_file(str(output_gpkg_file), layer='city_place', driver='GPKG')
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
    gdf_nodes.geometry = gdf_nodes.geometry.centroid
    gdf_nodes.set_crs(epsg=4326, inplace=True)
    gdf_nodes.to_file(str(output_gpkg_file), layer=type, driver='GPKG')

def convert_poi_2_gdf(osm_string: str, type: str) -> gpd.GeoDataFrame:
    poi_handler = POIHandler()
    poi_handler.apply_buffer(osm_string,format='xml', locations=True)
    gdf_nodes = gpd.GeoDataFrame(poi_handler.features)
    gdf_nodes.geometry = gdf_nodes.geometry.centroid
    gdf_nodes['type'] = type
    gdf_nodes.set_crs(epsg=4326, inplace=True)
    return gdf_nodes
