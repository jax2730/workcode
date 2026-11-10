import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
from convert_coords import convert_coords
from . import road_util
import osgeo.ogr as ogr
import osgeo.osr as osr
import osgeo.gdal as gdal
import geopandas as gpd
import pandas as pd
from skimage.morphology import skeletonize
import sknw
import numpy as np
import shapely.geometry as geom
import shapely.ops as ops
import networkx as nx
from networkx.utils import UnionFind
import matplotlib.pyplot as plt
import math
import json
import fiona
import momepy

ROAD_DIR = DATA_DIR / 'road'

HIGHWAY_BUFFER = {
    'motorway': 120,
    'trunk': 100,
    'primary': 75,
    'secondary': 50,
    'tertiary': 30,
    'unclassified': 25,
    'track': 25,
    'path': 25,
    'road': 25,
    None:25
    }

HIGHWAY_CODE = {
    'motorway': 1,
    'trunk': 2,
    'primary': 3,
   'secondary': 4,
    'tertiary': 5,
    'unclassified': 6,
    'track': 7,
    'path': 8,
    None: 9,
    'none': 9,
    'road': 9,
    'count': 10
}

def filter_road_by_type(country_name: str):
    save_dir = ROAD_DIR / country_name
    road_path = save_dir / 'roads.gpkg'
    road_gdf = gpd.read_file(road_path,layer='road')
    road_gdf.drop(["members","members_count"], axis=1, inplace=True)
    road_gdf_filtered = road_gdf[road_gdf['highway'].isin(['motorway', 'trunk'])].copy()
    road_gdf = None
    #simplify road
    road_gdf_filtered.to_crs(epsg=3857, inplace=True)
    road_gdf_filtered.geometry = road_gdf_filtered.geometry.apply(lambda x: road_util.simplify_geom_multipart(ops.unary_union(x), 250))
    road_gdf_filtered.to_file(save_dir / 'roads_filtered.gpkg', driver='GPKG')
    # road_gdf_filtered.to_file(save_dir / 'roads_filtered.geojson', driver='GeoJSON')

def buffer_line(input_path: pathlib.Path, output_path: pathlib.Path, distance: float|dict = 200):
    road_gdf = gpd.read_file(input_path)
    road_gdf.to_crs(epsg=3857, inplace=True)
    if isinstance(distance, float) or isinstance(distance, int):
        road_dissove = road_gdf.dissolve()
        road_dissove.to_crs(epsg=3857, inplace=True)
        road_buffer_geom = road_dissove.union_all()
        road_buffer = road_buffer_geom.buffer(distance=200, resolution=2)
        polygons:geom.MultiPolygon = road_buffer
        max_area = 0
        max_poly = None
        for polygon in polygons.geoms:
            if polygon.area > max_area:
                max_area = polygon.area
                max_poly = polygon
        road_buffer = gpd.GeoDataFrame({'geometry': [max_poly]}, crs=road_dissove.crs)
        road_buffer.to_file(output_path, driver='GPKG')
    else:
        buffered_gdf = gpd.GeoDataFrame(columns=['id','geometry'], crs=road_gdf.crs)
        for i,row in road_gdf.iterrows():
            highway = row['highway']
            if highway in distance:
                buffer_dist = distance[highway]
            else:
                buffer_dist = distance[None]
            row_buffered = row.copy()
            row_buffered['geometry'] = row_buffered['geometry'].buffer(distance=buffer_dist, resolution=2)
            buffered_gdf.iloc[i] = [i, row_buffered['geometry']]
        buffered_gdf.to_file(output_path, driver='GPKG')

def buffer_road(country_name: str):
    save_dir = ROAD_DIR / country_name
    road_path = save_dir / 'roads_filtered.gpkg'
    buffer_line(road_path, save_dir / 'road_buffer.gpkg', distance=200)

def buffered_road_2_sekeleton(input_path: pathlib.Path, output_path: pathlib.Path, debug_dir: pathlib.Path = None):
    base_gitff_path = DATA_DIR /'world_terrain/worldmap_fill.tif'
    gitff_ds: gdal.Dataset = gdal.Open(str(base_gitff_path))
    in_ds = ogr.Open(input_path, 0)
    if in_ds is None:
        print("Could not open %s" % input_path)
        return
    in_lyr = in_ds.GetLayer()
    in_lyr_defn = in_lyr.GetLayerDefn()
    bounds = in_lyr.GetExtent()
    x_min, x_max, y_min, y_max = bounds
    convert_coords.load_base_gtiff(base_gitff_path)
    tiles = convert_coords.get_map_tiles((x_min, y_min, x_max, y_max), tile_size=1024, from_epsg=3857)
    total_gdf = gpd.GeoDataFrame(columns=['id','geometry'], crs=gitff_ds.GetProjectionRef())

    for xmin, ymin, xmax, ymax, col, row in tiles:
        print(f"Processing tile {col} {row}", end='\r', flush=True)
        tile_ds: gdal.Dataset = gdal.GetDriverByName('MEM').Create('', 1056, 1056, 1, gdal.GDT_Byte)
        tile_ds.SetGeoTransform((xmin- (gitff_ds.GetGeoTransform()[1]*16), gitff_ds.GetGeoTransform()[1], 0, ymax- (gitff_ds.GetGeoTransform()[5]*16), 0, gitff_ds.GetGeoTransform()[5]))
        tile_ds.SetProjection(gitff_ds.GetProjectionRef())
        x_mint, x_rest, x_skewt, y_maxt, y_skewt, y_rest = tile_ds.GetGeoTransform()
        tile_lyr = tile_ds.GetRasterBand(1)
        tile_lyr.SetNoDataValue(0)
        gdal.RasterizeLayer(tile_ds, [1], in_lyr, burn_values=[1], options=["ALL_TOUCHED=TRUE"])

        data_bytes = tile_lyr.ReadRaster(0, 0, tile_ds.RasterXSize, tile_ds.RasterYSize,
                             buf_type=gdal.GDT_Byte)
        in_data = np.frombuffer(data_bytes, dtype=np.uint8).reshape(tile_ds.RasterYSize, tile_ds.RasterXSize)
        in_data = (in_data > 0).astype(np.uint8)
        if np.sum(in_data) == 0:
            # print("No data in tile")
            continue
        skel = skeletonize(in_data)
        graph:nx.Graph = sknw.build_sknw(skel)
        edges_list = []
        for i, (s, e) in enumerate(graph.edges()):
            ps = graph[s][e]['pts']
            coords = [
                ( (pts[1] + 0.5) * x_rest + x_mint, 
                  (pts[0] + 0.5) * y_rest + y_maxt ) 
                for pts in ps
            ]
            if len(coords) >= 2: 
                edges_list.append({
                    'id': i + len(total_gdf),
                    'geometry': geom.LineString(coords)
                })
        if not edges_list:
            print("No skeleton found!")
            continue
        gdf = gpd.GeoDataFrame(edges_list, crs=gitff_ds.GetProjectionRef())
        if total_gdf.empty:
            total_gdf = gdf
        else:
            total_gdf = pd.concat([total_gdf, gdf], ignore_index=True)
        if debug_dir is not None and debug_dir.exists():
            gdf.to_file(debug_dir / f"tile_{col}_{row}.gpkg", driver='GPKG')
    total_gdf.to_crs(epsg=3857, inplace=True)
    total_gdf.to_file(output_path, driver='GPKG')
    

def buffered_road_2_sekeleton_road(country_name: str):
    base_gitff_path = DATA_DIR /'world_terrain/worldmap_fill.tif'
    gitff_ds: gdal.Dataset = gdal.Open(str(base_gitff_path))
    save_dir = ROAD_DIR / country_name
    debug_dir = save_dir / 'debug'
    if not debug_dir.exists():
        debug_dir.mkdir()
    road_buffer_gpkg = save_dir / 'road_buffer.gpkg'
    buffered_road_2_sekeleton(road_buffer_gpkg, save_dir / 'road_skeleton.gpkg')

def breakdown_intersections(input_path: pathlib.Path, output_path: pathlib.Path):
    road_gdf = gpd.read_file(input_path)
    road_gdf.to_crs(epsg=3857, inplace=True)

    road_union = ops.unary_union(road_gdf.geometry)
    road_gdf = gpd.GeoDataFrame({'geometry': [line for line in road_union.geoms]}, crs=road_gdf.crs)
    road_gdf = momepy.remove_false_nodes(road_gdf)
    road_gdf.to_file(output_path, driver='GPKG')

def breakdown_intersections_road(country_name:str):
    save_dir = ROAD_DIR / country_name
    breakdown_intersections(save_dir / 'road_skeleton.gpkg', save_dir / 'road_union.gpkg')

def remove_line_cycle(input_path: pathlib.Path, output_path: pathlib.Path, cycle_length: int = 8000, dangle_length: int = 2500):
    road_gdf = gpd.read_file(input_path)
    road_gdf.to_crs(epsg=3857, inplace=True)
    road_gdf = momepy.remove_false_nodes(road_gdf)
    road_gdf = road_gdf[~(road_gdf.geometry.is_ring & (road_gdf.geometry.length < 8000))].copy()
    road_network = momepy.gdf_to_nx(road_gdf, approach='primal', multigraph=False)
    
    while True:
        degrees = dict(road_network.degree())
        dangle_to_remove = []

        for u,v,data in road_network.edges(data=True):
            if data['mm_len'] < dangle_length:
                if degrees[u] == 1 or degrees[v] == 1:
                    dangle_to_remove.append((u,v))

        if not dangle_to_remove:
            break
        road_network.remove_edges_from(dangle_to_remove)
        road_network.remove_nodes_from(list(nx.isolates(road_network)))

    cycles = nx.cycle_basis(road_network)
    edge_to_remove = []
    edge_to_add = []
    nodes_to_remove = set()
    for cycle in cycles:
        cycle_len = 0
        cycle_edges = []
        if len(cycle) == 1:
            node = cycle[0]
            edge_data = road_network.get_edge_data(node, node)
            if edge_data and edge_data['mm_len'] < 2* dangle_length:
                edge_to_remove.append((node, node))
                continue

        for i in range(len(cycle)):
            u,v = cycle[i], cycle[(i+1)%len(cycle)]
            edge_data = road_network.get_edge_data(u,v)
            if edge_data:
                cycle_len += edge_data['mm_len']
                cycle_edges.append((u,v))
        if cycle_len < cycle_length:
            edge_to_remove.extend(cycle_edges)
            node_coords = [geom.Point(road_network.nodes[n]['x'], road_network.nodes[n]['y']) for n in cycle]
            convex_hull = geom.MultiPoint(node_coords).centroid
            new_node_geom = convex_hull.centroid
            new_node_id = (new_node_geom.x, new_node_geom.y)

            for node in cycle:
                for neighbor in road_network.neighbors(node):
                    if neighbor in cycle:
                        continue
                    old_edge_data = road_network.get_edge_data(node, neighbor)
                    old_geom:geom.LineString = old_edge_data['geometry']
                    #replace old edge node with new node
                    if old_geom.coords[0] == node:
                        new_line = geom.LineString([*new_node_geom.coords, *old_geom.coords])
                    elif old_geom.coords[-1] == node:
                        new_line = geom.LineString([*old_geom.coords, *new_node_geom.coords])
                    edge_to_add.append((new_node_id, neighbor, new_line))
                nodes_to_remove.add(node)
    road_network.remove_edges_from(edge_to_remove)
    road_network.remove_nodes_from(list(nodes_to_remove))
    for u,v,data in edge_to_add:
        road_network.add_edge(u,v,geometry=data,mm_len=data.length)
    removed_nodes_gdf = gpd.GeoDataFrame(columns=['id','geometry'], crs=road_gdf.crs)
    for node in nodes_to_remove:
        removed_nodes_gdf.loc[len(removed_nodes_gdf)] = {'id': node, 'geometry': geom.Point(node[0], node[1])}
    # removed_nodes_gdf.set_crs(epsg=3857, inplace=True)
    # removed_nodes_gdf.to_file(save_dir / 'road_removed_nodes.gpkg', driver='GPKG')

    while True:
        degrees = dict(road_network.degree())
        node_to_remove = []
        edge_to_add = []
        for node in road_network.nodes:
            if degrees[node] == 2:
                node_to_remove.append(node)
                edges = list(road_network.edges(node))
                edge0 = edges[0]
                edge1 = edges[1]
                
                node0 = edge0[0] if edge0[0] != node else edge0[1]
                node1 = edge1[0] if edge1[0] != node else edge1[1]
                data0 = road_network.get_edge_data(node,node0)
                data1 = road_network.get_edge_data(node,node1)
                geom0 = data0['geometry']
                geom1 = data1['geometry']

                new_data = data0.copy() if geom0.length > geom1.length else data1.copy()
                new_geom = ops.linemerge([geom0,geom1])
                if new_geom.geom_type != 'LineString':
                    print(f'Warning: merge two edges failed, new_geom type is {new_geom.geom_type}')
                new_data['geometry'] = new_geom
                new_data['mm_len'] = new_geom.length
                edge_to_add.append((node1,node0,new_data))
                break
        if len(node_to_remove) == 0:
            break
        road_network.remove_nodes_from(node_to_remove)
        for u,v,data in edge_to_add:
            road_network.add_edge(u,v,**data)

    gdf_final = momepy.nx_to_gdf(road_network,points=False,lines=True)
    gdf_final.set_crs(epsg=3857, inplace=True)
    gdf_final.to_file(output_path, driver='GPKG')
    
def remove_short_roads(input_path: pathlib.Path, output_path: pathlib.Path, min_length: int = 1000):
    road_gdf = gpd.read_file(input_path)
    road_gdf.to_crs(epsg=3857, inplace=True)
    road_network = momepy.gdf_to_nx(road_gdf, approach='primal', multigraph=False)
    subsets = UnionFind()
    for u, v, d in road_network.edges(data=True):
        if d.get('mm_len', 0) < min_length:
            subsets.union(u, v)
    
    groups = {}
    for node in road_network.nodes():
        root = subsets[node]
        if root not in groups:
            groups[root] = []
        groups[root].append(node)
    
    new_road_network = road_network.copy()
    for root, members in groups.items():
        if len(members) < 2:
            continue

        avg_x = sum(road_network.nodes[m]['x'] for m in members) / len(members)
        avg_y = sum(road_network.nodes[m]['y'] for m in members) / len(members)

        main_node = members[0]
        for other_node in members[1:]:
            if new_road_network.has_node(other_node):
                new_road_network = nx.contracted_nodes(new_road_network, main_node, other_node, self_loops=False)

        new_road_network.nodes[main_node]['x'] = avg_x
        new_road_network.nodes[main_node]['y'] = avg_y

        for neighbor in new_road_network.neighbors(main_node):
            edge_data = new_road_network.get_edge_data(main_node, neighbor)
            old_coords = list(edge_data['geometry'].coords)

            if geom.Point(old_coords[0]).distance(geom.Point(road_network.nodes[main_node]['x'], road_network.nodes[main_node]['y']))<1e-3:
                new_coords = [(avg_x, avg_y)] + old_coords[1:]
            elif geom.Point(old_coords[-1]).distance(geom.Point(road_network.nodes[main_node]['x'], road_network.nodes[main_node]['y']))<1e-3:
                new_coords = old_coords[:-1] + [(avg_x, avg_y)]
            elif geom.Point(old_coords[0]).distance(geom.Point(road_network.nodes[main_node]['x'], road_network.nodes[main_node]['y']))<\
                geom.Point(old_coords[-1]).distance(geom.Point(road_network.nodes[main_node]['x'], road_network.nodes[main_node]['y'])):
                new_coords = [(avg_x, avg_y)] + old_coords[1:]
            else:
                new_coords = old_coords[:-1] + [(avg_x, avg_y)]
            new_line = geom.LineString(new_coords)
            edge_data['geometry'] = new_line
            edge_data['mm_len'] = new_line.length
    gdf_final = momepy.nx_to_gdf(new_road_network,points=False,lines=True)
    gdf_final.set_crs(epsg=3857, inplace=True)
    gdf_final.to_file(output_path, driver='GPKG')

def remove_road_cycle(country_name:str):
    save_dir = ROAD_DIR / country_name
    remove_line_cycle(save_dir / 'road_union.gpkg', save_dir / 'road_cleaned.gpkg', cycle_length=8000, dangle_length=2500)
    remove_short_roads(save_dir / 'road_cleaned.gpkg', save_dir / 'road_cleaned_final.gpkg', min_length=1000)

def match_segment_to_road(country_name:str):
    save_dir = ROAD_DIR / country_name
    road_gdf = gpd.read_file(save_dir / 'roads_filtered.gpkg')
    simplified_gdf = gpd.read_file(save_dir / 'road_cleaned.gpkg')
    road_gdf.to_crs(epsg=3857, inplace=True)
    road_gdf = road_gdf.reset_index(names=['original_id'])
    road_buffer_gdf = road_gdf.copy()
    road_buffer_gdf['geometry'] = road_buffer_gdf.apply(lambda row: row.geometry.buffer(HIGHWAY_BUFFER[row['highway']]), axis=1)
    road_buffer_gdf = road_buffer_gdf.dissolve(by=['highway','name:local'])
    simplified_gdf = simplified_gdf.to_crs(epsg=3857)
    simplified_gdf = simplified_gdf.reset_index(names=['simp_id'])
    original_cols_for_join = ['original_id', road_gdf.geometry.name]

    joined_gdf: gpd.GeoDataFrame = simplified_gdf.sjoin(road_buffer_gdf[original_cols_for_join], how='left', predicate='within')
    
    finded_roads = joined_gdf[joined_gdf['original_id'].notnull()]
    un_finded_simplified = simplified_gdf[~simplified_gdf.index.isin(finded_roads.simp_id)]
    joined_unfind_gdf = gpd.GeoDataFrame(columns=['simp_id','id', 'geometry','right_id','original_id'])
    for i, row in un_finded_simplified.iterrows():
        row: gpd.GeoSeries
        geometry:geom.LineString = row['geometry']
        start_p = geom.Point(geometry.coords[0])
        end_p = geom.Point(geometry.coords[-1])
        centroid = geometry.centroid
        nearest_id = (road_buffer_gdf.distance(centroid) + road_buffer_gdf.distance(start_p) + road_buffer_gdf.distance(end_p)).idxmin()
        original_id = road_buffer_gdf.loc[nearest_id]['original_id']
        row_dict = {'simp_id': i, 'id': len(joined_unfind_gdf)+1, 'geometry': geometry, 'right_id': nearest_id, 'original_id': original_id}
        joined_unfind_gdf.loc[len(joined_unfind_gdf)] = row_dict

    joined_unfind_gdf = joined_unfind_gdf.drop_duplicates(subset=['simp_id'], keep='first')
    joined_gdf = pd.concat([finded_roads, joined_unfind_gdf])
    joined_gdf = joined_gdf.sort_values(
        by=['simp_id']
    )

    final_gdf = gpd.GeoDataFrame(columns=['geometry', 'name:local', 'name:en', 'name:zh', 'highway'])
    for simp_id, group in joined_gdf.groupby('simp_id'):
        group: gpd.GeoDataFrame

        original_ids = group['original_id'].tolist()

        data = [{
                    'highway':road_gdf.loc[id]['highway'],
                    'name:local':road_gdf.loc[id]['name:local'],
                    'name:en':road_gdf.loc[id]['name:en'],
                    'name:zh':road_gdf.loc[id]['name:zh']
                }\
                for id in original_ids]
        
        data_gdf = gpd.GeoDataFrame(data, columns=['highway', 'name:local', 'name:en', 'name:zh'])
        data_gdf['priority_key'] = data_gdf['highway'].map(HIGHWAY_CODE).fillna(11)
        data_gdf.sort_values(by='priority_key', ascending=True, inplace=True)
        data_gdf.drop(columns=['priority_key'], inplace=True)
        data_gdf.drop_duplicates(subset=['name:local'], keep='first', inplace=True)

        data = [r.to_dict() for _,r in data_gdf.iterrows()]
        
        names_local = [d['name:local'] for d in data]
        name_en = [d['name:en'] for d in data]
        name_zh = [d['name:zh'] for d in data]
        highway = [d['highway'] for d in data]

        geometry = simplified_gdf.loc[simp_id].geometry
        row = {'geometry': geometry, 'name:local': names_local, 'name:en': name_en, 'name:zh': name_zh, 'highway': highway}
        final_gdf.loc[len(final_gdf)] = row

    final_gdf['highway_code'] = final_gdf['highway'].apply(lambda x: HIGHWAY_CODE[x[0]])
    final_gdf['name:local'] = final_gdf['name:local'].apply(lambda x: json.dumps(x,ensure_ascii=False))
    final_gdf['name:en'] = final_gdf['name:en'].apply(lambda x: json.dumps(x,ensure_ascii=False))
    final_gdf['name:zh'] = final_gdf['name:zh'].apply(lambda x: json.dumps(x,ensure_ascii=False))
    final_gdf['highway'] = final_gdf['highway'].apply(lambda x: json.dumps(x,ensure_ascii=False))

    final_gdf.set_crs(epsg=3857, inplace=True)

    final_gdf.to_file(save_dir / 'city_matched.gpkg', driver='GPKG')
    return final_gdf

def smooth_road(country_name:str):
    save_dir = ROAD_DIR / country_name
    road_util.dP_simplification(save_dir / 'city_matched.gpkg', save_dir / 'city_matched_smooth.gpkg', 250)

def process_road(country_code:str):
    # filter_road_by_type(country_code)
    # buffer_road(country_code)
    # buffered_road_2_sekeleton_road(country_code)
    # breakdown_intersections_road(country_code)
    remove_road_cycle(country_code)
    match_segment_to_road(country_code)
    smooth_road(country_code)

def process_railway():
    save_dir = DATA_DIR / 'road'
    # buffer_line(save_dir/'chian_highspeed_railways.geojson',save_dir/'china_railways_buffer.gpkg',1000)
    # buffered_road_2_sekeleton(save_dir/'china_railways_buffer.gpkg',save_dir/'china_railways_skeleton.gpkg')
    # breakdown_intersections(save_dir/'china_railways_skeleton.gpkg',save_dir/'china_railways_union.gpkg')
    remove_line_cycle(save_dir/'china_railways_union.gpkg',save_dir/'china_railways_cleaned.gpkg',cycle_length=8000,dangle_length=2500)
    road_util.dP_simplification(save_dir/'china_railways_cleaned.gpkg',save_dir/'china_railways_cleaned_smooth.gpkg',250)

if __name__ == '__main__':
    # process_railway()
    process_road('JP')
    # process_road('DE')
    # process_road('DK')
    # process_road('ES')
    # process_road('FI')
    # process_road('FR')
    # process_road('GB')
    # process_road('GR')
    # process_road('IT')
    # process_road('NO')
    # process_road('PL')
    # process_road('SE')
    # process_road('TW')
    # process_road('US')