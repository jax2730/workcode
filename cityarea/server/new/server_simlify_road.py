import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
from road_process import road_util, simplify_road as sr
from gen_voronoi import gen_voroni
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
import matplotlib.pyplot as plt
import math
import json
import fiona
import momepy
import argparse
import convert_coords.convert_coords as convert_coords
from skan.csr import Skeleton as SkanSkeleton
from common.db import SessionLocal
from common.model import SimplyfyTask,LandmarkTask

MAP_OFFSET = [-2097152.0*2, -1048576.0*2];
CITY_DIR = DATA_DIR / 'city'

BATCH_STEP3_CITY_NAMES = [
    "Baoding",
    "Langfang",
    "Datong",
    "Qinghuangdao",
    "Shijiazhuang",
    "Chengde",
    "Hengshui",
    "Tangshan",
    "Xiong'an New Area",
    "Zhangjiakou",
]

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


def _extract_primary_highway(highway_value):
    if isinstance(highway_value, str):
        try:
            highway_value = json.loads(highway_value)
        except json.JSONDecodeError:
            return highway_value

    if isinstance(highway_value, list) and len(highway_value) > 0:
        return highway_value[0]
    return highway_value


def _prune_short_leaf_spurs(road_network, max_length: float | None = None, protected_highway=None):
    if protected_highway is None:
        protected_highway = []

    while True:
        nodes_to_remove = []
        for node, degree in list(road_network.degree()):
            if degree != 1:
                continue

            neighbors = list(road_network.neighbors(node))
            if len(neighbors) != 1:
                continue

            neighbor = neighbors[0]
            edge_data = road_network.get_edge_data(node, neighbor)
            if edge_data is None:
                continue

            if road_network.is_multigraph():
                if len(edge_data) != 1:
                    continue
                edge_data = next(iter(edge_data.values()))

            geometry = edge_data.get('geometry')
            if geometry is None:
                continue
            if max_length is not None and geometry.length > max_length:
                continue

            highway = _extract_primary_highway(edge_data.get('highway'))
            if highway in protected_highway:
                continue
            nodes_to_remove.append(node)

        if len(nodes_to_remove) == 0:
            break

        road_network.remove_nodes_from(nodes_to_remove)

def merge_two_lines_at_node(geom0, geom1, node_xy, tol=1e-6):
    c0 = list(geom0.coords)
    c1 = list(geom1.coords)

    def close(p, q):
        return abs(p[0]-q[0]) < tol and abs(p[1]-q[1]) < tol

    # 让 geom0 以 node 结尾
    if close(c0[0], node_xy):
        c0 = c0[::-1]
    elif not close(c0[-1], node_xy):
        return None

    # 让 geom1 以 node 开头
    if close(c1[-1], node_xy):
        c1 = c1[::-1]
    elif not close(c1[0], node_xy):
        return None

    merged = c0 + c1[1:]
    return geom.LineString(merged)

def remove_degree_2_node_gpkg(road_gpkg: pathlib.Path, out_gpkg: pathlib.Path):
    road_gdf = gpd.read_file(road_gpkg)
    road_gdf = road_gdf.to_crs(epsg=3857)
    road_gdf = road_gdf[road_gdf.geometry.isna() == False]
    road_gdf = road_gdf.explode(index_parts=False).reset_index(drop=True)
    road_network = momepy.gdf_to_nx(road_gdf, approach='primal', multigraph=True)
    # remove degree 2 node and disslove 2 edges
    while True:
        node_to_remove = []
        edge_to_add = []
        for node in road_network.nodes:
            edges = list(road_network.edges(node, keys=True, data=True))
            if len(edges) == 2:
                e0 = edges[0]
                e1 = edges[1]

                u0, v0, k0, d0 = e0
                u1, v1, k1, d1 = e1

                node0 = u0 if v0 == node else v0
                node1 = u1 if v1 == node else v1

                data0 = d0
                data1 = d1
                geom0 = data0['geometry']
                geom1 = data1['geometry']

                new_data = data0.copy() if geom0.length > geom1.length else data1.copy()
                geom0 = ops.snap(geom0, geom1, tolerance=10)
                new_geom = merge_two_lines_at_node(geom0, geom1, node, tol=1e-6)

                if new_geom.geom_type != 'LineString':
                    print(f'Warning: merge two edges failed, new_geom type is {new_geom.geom_type}')
                new_data['geometry'] = new_geom
                new_data['mm_len'] = new_geom.length
                node_to_remove.append(node)
                edge_to_add.append((node1,node0,new_data))
                break
        if len(node_to_remove) == 0:
            break
        road_network.remove_nodes_from(node_to_remove)
        for u,v,data in edge_to_add:
            road_network.add_edge(u,v,**data)

    largest_cc = max(nx.connected_components(road_network), key=len)
    road_network = road_network.subgraph(largest_cc).copy()

    #_prune_short_leaf_spurs(road_network)
    road_gdf = momepy.nx_to_gdf(road_network, points=False, lines=True)
    road_gdf.reset_index(inplace=True,drop=True)
    road_gdf['road_id'] = road_gdf.index
    road_gdf.to_file(out_gpkg, driver='GPKG')

_NEIGHBOR_OFFSETS_8 = [
    (-1, -1), (-1, 0), (-1, 1),
    (0, -1),           (0, 1),
    (1, -1),  (1, 0),  (1, 1),
]


def _pixel_neighbors(pixel, skel):
    r, c = pixel
    h, w = skel.shape
    for dr, dc in _NEIGHBOR_OFFSETS_8:
        nr = r + dr
        nc = c + dc
        if 0 <= nr < h and 0 <= nc < w and skel[nr, nc]:
            yield nr, nc


def _pixel_edge_key(a, b):
    return tuple(sorted((a, b)))


def skeleton_to_geom_graph(skel_bool, x_min, x_res, y_max, y_res):
    """
    Convert a 2D skeleton binary image to a NetworkX graph without sknw.

    Node pixels:
      - degree == 1: endpoints
      - degree >= 3: junctions
      - degree == 0: isolated pixels
      - degree == 2: ordinary line pixels

    Edges are traced between node pixels.
    """
    skel = np.ascontiguousarray(skel_bool.astype(bool))
    h, w = skel.shape

    # 计算每个 skeleton 像素的 8 邻域 degree
    padded = np.pad(skel, ((1, 1), (1, 1)), mode='constant', constant_values=False)
    degree = np.zeros_like(skel, dtype=np.uint8)

    for dr, dc in _NEIGHBOR_OFFSETS_8:
        degree += padded[
            1 + dr: 1 + dr + h,
            1 + dc: 1 + dc + w
        ]

    node_mask = skel & (degree != 2)

    graph = nx.Graph()
    graph.graph['approach'] = 'primal'

    visited_pixel_edges = set()

    def pixel_to_xy(pixel):
        row, col = pixel
        x = (col + 0.5) * x_res + x_min
        y = (row + 0.5) * y_res + y_max
        return x, y

    def add_path_as_edge(path):
        if len(path) < 2:
            return

        coords = [pixel_to_xy(p) for p in path]

        # 去掉连续重复坐标，避免 LineString 异常
        dedup_coords = []
        last = None
        for xy in coords:
            if xy != last:
                dedup_coords.append(xy)
                last = xy

        if len(dedup_coords) < 2:
            return

        line = geom.LineString(dedup_coords)
        if line.length <= 0:
            return

        start_pixel = path[0]
        end_pixel = path[-1]

        graph.add_edge(
            start_pixel,
            end_pixel,
            geometry=line,
            id=graph.number_of_edges(),
            mm_len=line.length
        )

    # 从所有 node pixel 出发，沿 skeleton 追踪到下一个 node pixel
    node_rows, node_cols = np.nonzero(node_mask)

    for sr, sc in zip(node_rows, node_cols):
        start = (int(sr), int(sc))

        for nb in _pixel_neighbors(start, skel):
            edge_key = _pixel_edge_key(start, nb)
            if edge_key in visited_pixel_edges:
                continue

            path = [start]
            prev = start
            cur = nb
            visited_pixel_edges.add(edge_key)

            while True:
                path.append(cur)

                # 到达另一个 node，结束一条边
                if node_mask[cur] and cur != start:
                    break

                next_pixels = [
                    p for p in _pixel_neighbors(cur, skel)
                    if p != prev and _pixel_edge_key(cur, p) not in visited_pixel_edges
                ]

                if not next_pixels:
                    break

                # 对 degree==2 的普通像素，理论上只有一个可前进方向
                nxt = next_pixels[0]
                visited_pixel_edges.add(_pixel_edge_key(cur, nxt))
                prev, cur = cur, nxt

            add_path_as_edge(path)

    # 处理纯环：整条环上所有像素 degree 都是 2，没有 node pixel
    skel_rows, skel_cols = np.nonzero(skel)

    for sr, sc in zip(skel_rows, skel_cols):
        start = (int(sr), int(sc))

        for nb in _pixel_neighbors(start, skel):
            if _pixel_edge_key(start, nb) in visited_pixel_edges:
                continue

            path = [start]
            prev = start
            cur = nb
            visited_pixel_edges.add(_pixel_edge_key(start, nb))

            while True:
                path.append(cur)

                next_pixels = [
                    p for p in _pixel_neighbors(cur, skel)
                    if p != prev and _pixel_edge_key(cur, p) not in visited_pixel_edges
                ]

                if not next_pixels:
                    break

                nxt = next_pixels[0]
                visited_pixel_edges.add(_pixel_edge_key(cur, nxt))
                prev, cur = cur, nxt

                if cur == start:
                    path.append(cur)
                    break

            # 闭环不要作为 self-loop 一次性加进去，拆成两段更安全
            if len(path) >= 4 and path[0] == path[-1]:
                mid = len(path) // 2
                add_path_as_edge(path[:mid + 1])
                add_path_as_edge(path[mid:])
            else:
                add_path_as_edge(path)

    return graph

def skeleton_to_geom_graph_skan(skel_bool, x_min, x_res, y_max, y_res):
    """
    Convert skeleton image to NetworkX graph using skan instead of sknw.

    skel_bool: 2D bool skeleton image.
    x_min, x_res, y_max, y_res: GDAL geotransform params.
    """
    skel_bool = np.ascontiguousarray(skel_bool.astype(bool))

    skeleton = SkanSkeleton(skel_bool)
    geom_graph = nx.Graph()
    geom_graph.graph['approach'] = 'primal'

    def pixel_to_xy(row, col):
        x = (col + 0.5) * x_res + x_min
        y = (row + 0.5) * y_res + y_max
        return x, y

    def add_linestring_from_coords(coords):
        if coords is None or len(coords) < 2:
            return

        # skan path_coordinates 返回的是 row, col
        xy_coords = []
        last_xy = None

        for row, col in coords:
            xy = pixel_to_xy(float(row), float(col))
            if xy != last_xy:
                xy_coords.append(xy)
                last_xy = xy

        if len(xy_coords) < 2:
            return

        line = geom.LineString(xy_coords)
        if line.is_empty or line.length <= 0:
            return

        start_node = xy_coords[0]
        end_node = xy_coords[-1]

        # 如果是闭环，nx.Graph 的 self-loop 后续可能不好处理，拆成两段
        if start_node == end_node and len(xy_coords) >= 4:
            mid = len(xy_coords) // 2

            line_1 = geom.LineString(xy_coords[:mid + 1])
            line_2 = geom.LineString(xy_coords[mid:])

            if line_1.length > 0:
                geom_graph.add_edge(
                    xy_coords[0],
                    xy_coords[mid],
                    geometry=line_1,
                    id=geom_graph.number_of_edges(),
                    mm_len=line_1.length
                )

            if line_2.length > 0:
                geom_graph.add_edge(
                    xy_coords[mid],
                    xy_coords[-1],
                    geometry=line_2,
                    id=geom_graph.number_of_edges(),
                    mm_len=line_2.length
                )
        else:
            geom_graph.add_edge(
                start_node,
                end_node,
                geometry=line,
                id=geom_graph.number_of_edges(),
                mm_len=line.length
            )

    # skan 新版本推荐用 n_paths + path_coordinates
    if hasattr(skeleton, 'n_paths') and hasattr(skeleton, 'path_coordinates'):
        for path_id in range(skeleton.n_paths):
            coords = skeleton.path_coordinates(path_id)
            add_linestring_from_coords(coords)
    else:
        # 兼容部分旧版本 skan
        for path in skeleton.paths_list():
            coords = skeleton.coordinates[path]
            add_linestring_from_coords(coords)

    return geom_graph

def graph_edges_to_gdf(geom_graph, crs="EPSG:3857"):
    rows = []
    for _, _, data in geom_graph.edges(data=True):
        rows.append(data.copy())
    if len(rows) == 0:
        return gpd.GeoDataFrame(columns=['geometry'], geometry='geometry', crs=crs)
    return gpd.GeoDataFrame(rows, geometry='geometry', crs=crs)


def rater_skeletonize(city_name: str):
    save_dir = CITY_DIR / city_name
    raster_path = save_dir / 'road_buffer.tif'
    output_path = save_dir / 'road_skeleton.gpkg'

    in_ds = gdal.Open(str(raster_path))
    if in_ds is None:
        print("Could not open %s" % raster_path)
        return

    x_min, x_res, x_skew, y_max, y_skew, y_res = in_ds.GetGeoTransform()
    in_band = in_ds.GetRasterBand(1)

    data_bytes = in_band.ReadRaster(
        0,
        0,
        in_ds.RasterXSize,
        in_ds.RasterYSize,
        buf_xsize=in_ds.RasterXSize,
        buf_ysize=in_ds.RasterYSize,
        buf_type=gdal.GDT_Byte
    )

    if data_bytes is None:
        print("Could not read raster band %s" % raster_path)
        return

    in_data = np.frombuffer(data_bytes, dtype=np.uint8).reshape(
        in_ds.RasterYSize,
        in_ds.RasterXSize
    )

    mask = in_data > 0
    skel = skeletonize(mask)

    geom_graph = skeleton_to_geom_graph_skan(
        skel_bool=skel,
        x_min=x_min,
        x_res=x_res,
        y_max=y_max,
        y_res=y_res
    )

    gdf = graph_edges_to_gdf(geom_graph, crs="EPSG:3857")
    gdf.to_file(output_path, driver='GPKG')


def buffered_road_2_raster(city_name: str):
    save_dir = CITY_DIR / city_name
    road_buffer_gpkg = save_dir / 'road_buffer.gpkg'
    in_ds = ogr.Open(road_buffer_gpkg, 0)
    if in_ds is None:
        print("Could not open %s" % road_buffer_gpkg)
        return
    in_lyr = in_ds.GetLayer()
    in_lyr_defn = in_lyr.GetLayerDefn()
    bounds = in_lyr.GetExtent()
    x_min, x_max, y_min, y_max = bounds
    x_res = (x_max - x_min) / 10240
    y_res = (y_max - y_min) / 10240

    road_raster_tif = save_dir / 'road_buffer.tif'
    if road_raster_tif.exists():
        road_raster_tif.unlink()
    out_ds = gdal.GetDriverByName('GTiff').Create(str(road_raster_tif), 10240, 10240, 1, gdal.GDT_Byte)
    out_ds.SetGeoTransform((x_min, x_res, 0, y_max, 0, -y_res))
    out_ds.SetProjection(in_lyr.GetSpatialRef().ExportToWkt())
    out_lyr = out_ds.GetRasterBand(1)
    out_lyr.SetNoDataValue(0)
    gdal.RasterizeLayer(out_ds, [1], in_lyr, burn_values=[1])
    out_ds = None
    in_ds = None

def remove_close_edge(city_name:str, tolerance: float = 50):
    save_dir = CITY_DIR / city_name
    road_path = save_dir / "road_skeleton.gpkg"
    output_path = save_dir / "city_road_no_joint.gpkg"

    sr.remove_short_roads(road_path, output_path, min_length=tolerance)



def remove_leaf_node(road_path: pathlib.Path, output_path: pathlib.Path):
    road_gdf = gpd.read_file(road_path)
    road_gdf = road_gdf.to_crs(epsg=3857)
    geom_graph = nx.Graph()
    for i, row in road_gdf.iterrows():
        row: gpd.GeoSeries
        geometry = row['geometry']
        if geometry.geom_type == 'LineString':
            geom_graph.add_edge(geometry.coords[0], geometry.coords[-1], **row.to_dict())

    # remove leaf node where degree is 1 and highway is not motorway or trunk
    leaf_nodes = [node for node, degree in geom_graph.degree() if degree == 1]
    last_leafs_num = -1
    while len(leaf_nodes)>0 and len(leaf_nodes)!= last_leafs_num:
        for node in leaf_nodes:
            edges = geom_graph.edges(node, data=True)
            for s, e, data in edges:
                highways = json.loads(data['highway'])
                if ('motorway' not in highways and 'trunk' not in highways) or data['geometry'].length < 150:
                    geom_graph.remove_edge(s, e)
                    geom_graph.remove_node(node)
                    break
        last_leafs_num = len(leaf_nodes)
        leaf_nodes = [node for node, degree in geom_graph.degree() if degree == 1]




    # rebuild road_gdf
    road_gdf = gpd.GeoDataFrame(columns=road_gdf.columns)
    for edge in geom_graph.edges(data=True):
        s, e, data = edge
        row = data.copy()
        road_gdf.loc[len(road_gdf)] = row

    road_gdf.set_crs(epsg=3857, inplace=True)
    road_gdf.to_file(output_path, driver='GPKG')


def match_segment_to_road(city_name:str):
    save_dir = CITY_DIR / city_name
    road_gdf = gpd.read_file(save_dir / 'city.gpkg',layer='city_road')
    simplified_gdf = gpd.read_file(save_dir / 'city_road_no_joint.gpkg')
    road_gdf = road_gdf.to_crs(epsg=3857)
    road_gdf = road_gdf.reset_index(names=['original_id'])
    road_buffer_gdf = road_gdf.copy()
    # road_buffer_gdf['geometry'] = road_buffer_gdf.geometry.buffer(50)
    road_buffer_gdf['geometry'] = road_buffer_gdf.apply(lambda row: row.geometry.buffer(HIGHWAY_BUFFER[row['highway']]), axis=1)
    road_buffer_gdf = road_buffer_gdf.dissolve(by=['highway','name:local'])
    simplified_gdf = simplified_gdf.to_crs(epsg=3857)
    simplified_gdf = simplified_gdf.reset_index(names=['simp_id'])
    # road_buffer_gdf.to_file(save_dir / 'road_buffer.gpkg', driver='GPKG')
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

    # joined_unfind_gdf = un_finded_simplified.sjoin_nearest(road_gdf[original_cols_for_join], max_distance=100, how='left',distance_col='distance')
    # joined_unfind_gdf = joined_unfind_gdf.sort_values(by='distance')
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

def gen_street_block(city_name: str):
    save_dir = CITY_DIR / city_name
    simplified_road_gpkg = save_dir / 'city_road_no_leaf.gpkg'
    street_block_gpkg = save_dir / 'city_street_block.gpkg'

    road_gdf = gpd.read_file(simplified_road_gpkg)
    road_gdf = road_gdf.to_crs(epsg=3857)

    merged_gdf = road_gdf.geometry.union_all()
    polygons:geom.MultiPolygon = list(ops.polygonize(merged_gdf))

    blocks = gpd.GeoDataFrame(crs=road_gdf.crs, geometry=polygons)

    blocks.to_file(street_block_gpkg, driver='GPKG')

def get_shape_width(shape: geom.Polygon) -> float:
    if not isinstance(shape, geom.Polygon):
        return float('nan')
    obb = shape.minimum_rotated_rectangle
    width = geom.LineString((obb.exterior.coords[0], obb.exterior.coords[1])).length
    height = geom.LineString((obb.exterior.coords[1], obb.exterior.coords[2])).length
    return min(width, height)



def merge_small_street_block(city_name: str, min_width: float = 400):
    save_dir = CITY_DIR / city_name
    road_path = save_dir / 'city_road_no_leaf.gpkg'
    street_block_path = save_dir / 'city_street_block.gpkg'
    road_gdf = gpd.read_file(road_path)
    road_gdf = road_gdf.to_crs(epsg=3857)
    road_gdf = road_gdf[road_gdf.geometry.notna()].copy().reset_index(drop=True)

    city_sector_path = save_dir / 'city_sector.gpkg'
    layers_list = fiona.listlayers(city_sector_path)
    city_sector_gdfs = [
        gpd.read_file(city_sector_path, layer=layer).to_crs(epsg=3857).reset_index(drop=True)
        for layer in layers_list
    ]

    sector_union = city_sector_gdfs[0].geometry.union_all()
    road_candidate_pos = road_gdf.sindex.query(sector_union, predicate='covers')
    road_gdf = road_gdf.iloc[np.unique(road_candidate_pos)].copy().reset_index(drop=True)
    road_sindex = road_gdf.sindex
    sector_sindexes = [sector_gdf.sindex for sector_gdf in city_sector_gdfs]

    street_block_gdf = gpd.GeoDataFrame(crs=road_gdf.crs, columns=['geometry','sector_id','sector_name'])

    def merge_small_street_block_by_sector(sector_level: int, sector_polygon:geom.Polygon, sector_id:int):
        nonlocal street_block_gdf

        if sector_level < len(layers_list):
            next_level_sector_gdf = city_sector_gdfs[sector_level]
            sub_sector_pos = sector_sindexes[sector_level].query(sector_polygon, predicate='covers')
            sub_sectors = next_level_sector_gdf.iloc[np.unique(sub_sector_pos)]
        else:
            sub_sectors = None
        if sub_sectors is not None and len(sub_sectors) > 0:
            for i, sub_sector in sub_sectors.iterrows():
                sub_sector: gpd.GeoSeries
                sub_sector_geom = sub_sector.geometry
                if sub_sector_geom is not None and sub_sector_geom.geom_type == 'Polygon':
                    merge_small_street_block_by_sector(sector_level+1, sub_sector_geom, i)
        else:
            road_pos = road_sindex.query(sector_polygon, predicate='covers')
            road_intersected_gdf = road_gdf.iloc[np.unique(road_pos)]
            if len(road_intersected_gdf) > 0:
                merged_geom = road_intersected_gdf.geometry.union_all()
                polygons:list[geom.Polygon] = list(ops.polygonize(merged_geom))
                for interior in sector_polygon.interiors:
                    #remove holes from polygons list
                    polygons = [p for p in polygons if not p.contains(interior.centroid)]

                if len(polygons)==0:
                    return
                street_blocks = gpd.GeoDataFrame(crs=road_gdf.crs, geometry=polygons)
                street_blocks['sector_lvl'] = sector_level - 1
                street_blocks['sector_id'] = sector_id
                merged_street_block_gdf = merge_street_block_by_width(street_blocks, min_width=min_width)
                street_block_gdf = gpd.GeoDataFrame(
                                    pd.concat([street_block_gdf, merged_street_block_gdf], ignore_index=True),
                                    geometry='geometry',
                                    crs=road_gdf.crs
                                )

    for i, row in city_sector_gdfs[0].iterrows():
        row: gpd.GeoSeries
        sector_geom = row.geometry
        if sector_geom is not None and sector_geom.geom_type == 'Polygon':
            sector_geom:geom.Polygon
            merge_small_street_block_by_sector(1, sector_geom,i)

    street_block_gdf.set_crs(epsg=3857, inplace=True)
    street_block_gdf.reset_index(drop=True, inplace=True)
    street_block_gdf['block_id'] = street_block_gdf.index
    street_block_gdf.to_file(street_block_path, driver='GPKG')
    road_in_street_block: gpd.GeoDataFrame = road_gdf.sjoin(street_block_gdf, how='inner', predicate='within')
    road_out_street_block: gpd.GeoDataFrame = road_gdf[~road_gdf.index.isin(road_in_street_block.index)].copy()
    road_out_street_block.reset_index(drop=True, inplace=True)
    road_out_street_block['road_id'] = road_out_street_block.index
    street_block_c_road = gpd.sjoin(street_block_gdf, road_out_street_block, how='inner', predicate='touches')
    # street_block_c_road.to_file(save_dir / 'city_street_block_c_road.gpkg', driver='GPKG')
    street_block_grouped = street_block_c_road.groupby(by='block_id')
    street_block_gdf['road_list'] = "[]"
    for block_id, group in street_block_grouped:
        group: gpd.GeoDataFrame
        road_list = group["index_right"].tolist()
        road_list_alt=[]
        for index in road_list:
            road_geom = road_out_street_block.loc[index].geometry
            block_geom = street_block_gdf.loc[block_id].geometry
            intersection_geom = block_geom.intersection(road_geom)
            if intersection_geom.geom_type == 'MultiLineString' or intersection_geom.geom_type == 'LineString'\
                or intersection_geom.geom_type == 'GeometryCollection':
                road_list_alt.append(index)
        road_list_str = json.dumps(road_list_alt)
        street_block_gdf.loc[block_id,'road_list'] = road_list_str
    street_block_gdf.to_file(save_dir / 'city_street_block_grouped.gpkg', driver='GPKG')

    road_out_street_block.to_file(save_dir / 'city_road_out_block.gpkg', driver='GPKG')

def filter_city_radius(city_name: str, radius: float):
    gpkg_path = CITY_DIR / city_name / "city_road_out_block.gpkg"
    filtered_path = CITY_DIR / city_name / "city_road_in_radius.gpkg"

    gdf = gpd.read_file(gpkg_path)

    # Reproject to a CRS with meter units
    gdf = gdf.to_crs(epsg=3857)

    # Center of the city road extent
    minx, miny, maxx, maxy = gdf.total_bounds
    center = geom.Point((minx + maxx) / 2, (miny + maxy) / 2)

    # Create the circle; radius is in meters
    circle = center.buffer(radius)

    # Keep roads that intersect the circle
    filtered_gdf = gdf[gdf.geometry.intersects(circle)].copy()

    filtered_gdf.to_file(filtered_path, driver="GPKG")

def gdf_2_network(city_name: str, tolerance: float = 100):
    gpkg_path = CITY_DIR / city_name / 'city_road_in_radius.gpkg'
    no2degree_gpkg_path = CITY_DIR / city_name / 'city_road_no_2degree.gpkg'
    json_path = CITY_DIR / city_name / 'city_road_network.json'
    remove_degree_2_node_gpkg(gpkg_path,no2degree_gpkg_path)
    gdf = gpd.read_file(no2degree_gpkg_path)
    gdf = gdf.to_crs(epsg=3857)
    gdf.reset_index(drop=True, inplace=True)

    nodes = []
    node_index = {}
    edges = []

    world_terrian = DATA_DIR /'world_terrain/worldmap_fill.tif'

    road_network = momepy.gdf_to_nx(gdf, approach='primal')

    convert_coords.load_base_gtiff(world_terrian)
    convert_coords.set_origin_point(MAP_OFFSET[0],MAP_OFFSET[1])
    convert_coords.set_pixel_scale(64)

    road_nodes = road_network.nodes(data=True)
    for node_id, node_data in road_nodes:
        x = node_data['x']
        y = node_data['y']
        pos = convert_coords.convert_coords(x=x, y=y, from_epsg=3857).astype(np.float32)

        _edges = road_network.edges(node_id, data=True)
        edges_data = [data for u,v,data in _edges]
        edges_index = [edge_data['road_id'] for edge_data in edges_data]
        nodes.append({'pos': pos.tolist(), 'edges': edges_index,'degree': road_network.degree(node_id)})
        node_index[node_id] = len(nodes)-1

    for u,v,k, edge_data in road_network.edges(data=True,keys=True):
        edge_geom = edge_data['geometry']
        road_util.simplify_geom(edge_geom, tolerance)
        coords = convert_coords.convert_linestring(edge_geom, with_height= False, from_epsg=3857)
        u_pos = nodes[node_index[u]]['pos']
        if not np.allclose(u_pos, coords[0], atol=1e-6):
            coords = coords[::-1]
        edge_dict = edge_data.copy()
        edge_dict['geom'] = [coord.tolist() for coord in coords]
        edge_dict['start'] = node_index[u]
        edge_dict['end'] = node_index[v]
        edge_dict['type'] = edge_dict['highway_code']
        edge_dict['name'] = edge_dict['name:zh'] if edge_dict['name:zh'] else edge_dict['name:en'] if edge_dict['name:en'] else edge_dict['name:local']
        edge_dict.pop('geometry')
        edges.append(edge_dict)
    edges.sort(key=lambda x: x['road_id'])
    with open(json_path, 'w') as f:
        json.dump({'version': 4,
                   'cityName': city_name,
                   'node_count': len(nodes),
                   'edge_count': len(edges),
                   'nodes': nodes,
                   'edges': edges}, f, ensure_ascii= False, indent=4)

def fill_street_block_with_voronoi(city_name:str):
    save_dir = CITY_DIR / city_name
    road_path = save_dir / 'city_road_out_block.gpkg'
    street_block_path = save_dir / 'city_street_block_grouped.gpkg'
    street_block_filled_path = save_dir / 'city_street_block_filled.gpkg'
    road_gdf = gpd.read_file(road_path)

    street_block_gdf = gpd.read_file(street_block_path)
    voronoi_polygons_gdf = gpd.GeoDataFrame(columns=street_block_gdf.columns, crs=street_block_gdf.crs)

    road_to_drop = set()
    road_to_add = gpd.GeoDataFrame(columns=road_gdf.columns, crs=road_gdf.crs)

    for i, row in street_block_gdf.iterrows():
        row: gpd.GeoSeries
        block_geom: geom.Polygon = row.geometry
        if block_geom.geom_type == 'Polygon' and block_geom.area > 5e6:
            outer_road_indexs = json.loads(row['road_list'])
            road_to_drop.update(outer_road_indexs)
            voronoi_polygons,fill_roads = gen_voroni.voronoi_edges_polygons(block_geom, 800)
            fill_roads_gdf = gpd.GeoDataFrame(geometry=fill_roads, crs=road_gdf.crs,columns=road_gdf.columns)
            fill_voronoi_gdf = gpd.GeoDataFrame(geometry=voronoi_polygons, crs=road_gdf.crs,columns=street_block_gdf.columns)
            voronoi_polygons_gdf:gpd.GeoDataFrame = pd.concat([voronoi_polygons_gdf,fill_voronoi_gdf])
            fill_roads_gdf['highway_code'] = 6
            fill_roads_gdf['name:local'] = r'["fill road"]'
            fill_roads_gdf['name:en'] = r'["fill road"]'
            fill_roads_gdf['name:zh'] = r'["fill road"]'
            fill_roads_gdf['highway'] = r'["unclassified"]'
            fill_roads_gdf.geometry = fill_roads_gdf.geometry.snap(voronoi_polygons_gdf.geometry.union_all(),5)
            road_to_add = pd.concat([road_to_add, fill_roads_gdf])
            street_block_gdf.loc[i,'geometry'] = None

    road_droped = road_gdf[road_gdf.index.isin(road_to_drop)].copy()
    road_gdf = road_gdf[~road_gdf.index.isin(road_to_drop)]
    voronoi_union = road_to_add.geometry.union_all()
    road_droped.geometry = road_droped.geometry.snap(voronoi_union,5)
    for i, row in road_droped.iterrows():
        row: gpd.GeoSeries
        row_geom: geom.LineString = row.geometry
        if row_geom.intersects(voronoi_union):
            new_row = row.to_dict()
            splitted = ops.split(row_geom, voronoi_union)
            for s in splitted.geoms:
                new_row['geometry'] = s
                road_to_add.loc[len(road_to_add)] = new_row.copy()
        else:
            road_to_add.loc[len(road_to_add)] = row.to_dict()

    road_gdf:gpd.GeoDataFrame = pd.concat([road_gdf, road_to_add])
    # voronoi_polygons_gdf.plot()
    # plt.show()
    street_block_gdf = pd.concat([street_block_gdf,voronoi_polygons_gdf])

    street_block_gdf.dropna(subset=['geometry'], inplace=True)
    street_block_gdf.set_crs(epsg=3857, inplace=True)
    new_polygons = list(ops.polygonize(road_gdf.geometry.union_all()))
    new_street_block_gdf = gpd.GeoDataFrame(geometry=new_polygons, crs=road_gdf.crs, columns=street_block_gdf.columns)
    new_street_block_gdf = new_street_block_gdf[new_street_block_gdf.geometry.area > 50]
    new_street_block_gdf.to_file(street_block_filled_path, driver='GPKG')
    # road_in_street_block: gpd.GeoDataFrame = road_gdf.sjoin(new_street_block_gdf, how='inner', predicate='within')
    # road_out_street_block: gpd.GeoDataFrame = road_gdf[~road_gdf.index.isin(road_in_street_block.index)]
    # road_out_street_block.reset_index(drop=True, inplace=True)
    # road_out_street_block['road_id'] = road_out_street_block.index
    road_gdf.reset_index(drop=True, inplace=True)
    road_gdf['road_id'] = road_gdf.index
    road_gdf.to_file(save_dir / 'city_road_filled.gpkg', driver='GPKG')

def extract_as_multipoint(g, deduplicate=True, precision=12):
    points = []

    def _collect(x):
        if x.is_empty:
            return

        gt = x.geom_type

        if gt == 'Point':
            points.append(x)

        elif gt == 'MultiPoint':
            points.extend(x.geoms)

        elif gt == 'LineString':
            coords = list(x.coords)
            if len(coords) == 1:
                points.append(geom.Point(coords[0]))
            elif len(coords) >= 2:
                points.append(geom.Point(coords[0]))
                points.append(geom.Point(coords[-1]))

        elif gt == 'MultiLineString':
            for line in x.geoms:
                _collect(line)

        elif gt == 'GeometryCollection':
            for subgeom in x.geoms:
                _collect(subgeom)

    _collect(g)

    if deduplicate:
        seen = set()
        unique_points = []
        for p in points:
            key = (round(p.x, precision), round(p.y, precision))
            if key not in seen:
                seen.add(key)
                unique_points.append(p)
        points = unique_points

    return geom.MultiPoint(points)

def extend_major_road_leaf(city_name: str, min_leaf_length: float = 450):
    road_gdf = gpd.read_file(CITY_DIR / city_name / 'city_road_no_leaf.gpkg')
    road_gdf = road_util.extend_leaf_road(road_gdf,2,min_leaf_length)
    road_gdf['to_remove'] = False
    major_road_gdf = road_gdf[road_gdf['highway_code'].isin(range(2+1))]
    outer_ring = major_road_gdf.dissolve().convex_hull.exterior[0]
    for i, row in major_road_gdf.iterrows():
        row:gpd.GeoSeries
        row_geom:geom.LineString = row.geometry
        if row_geom.intersects(outer_ring):
            new_row = row.to_dict()
            intersected_geom = row_geom.intersection(outer_ring)
            intersect_point = extract_as_multipoint(intersected_geom)

            splited = ops.split(row_geom, intersect_point)
            for line in splited.geoms:
                new_row['geometry'] = line
                road_gdf.loc[len(road_gdf)] = new_row
            road_gdf.loc[i,'to_remove'] = True
    outer_gdf = gpd.GeoDataFrame(columns=road_gdf.columns)
    for i in range(len(outer_ring.coords)-1):
        new_outer_row = {
            'geometry': geom.LineString([outer_ring.coords[i],outer_ring.coords[i+1]]),
            'highway_code': 0,
            'highway': r'["outer"]',
            'name:local': 'outer',
            'name:en': 'outer',
            'name:zh': 'outer',
            'to_remove' : False
        }
        outer_gdf.loc[len(outer_gdf)] = new_outer_row
    road_gdf = road_gdf[~road_gdf['to_remove']]
    road_gdf.drop(columns=['to_remove'], inplace=True)
    outer_poly = ops.polygonize(outer_ring)[0]
    road_gdf = road_gdf[road_gdf.geometry.apply(lambda x: outer_poly.covers(x))]
    for i, row in road_gdf.iterrows():
        row:gpd.GeoSeries
        row_geom:geom.LineString = row.geometry
        end_p0 = geom.Point(row_geom.coords[0])
        end_p1 = geom.Point(row_geom.coords[-1])
        distance_0 = outer_gdf.distance(end_p0)
        outer_index0 = distance_0.idxmin()
        distance_0 = distance_0.min()
        distance_1 = outer_gdf.distance(end_p1)
        outer_index1 = distance_1.idxmin()
        distance_1 = distance_1.min()
        if distance_0 < 100 or distance_1 < 100:
            if distance_0 < 100:
                ep_proj = outer_gdf.loc[outer_index0].geometry.project(end_p0)
                new_ep = outer_gdf.loc[outer_index0].geometry.interpolate(ep_proj)
                row_geom = geom.LineString([new_ep,*row_geom.coords[1:]])
                outer_gdf.loc[len(outer_gdf)] = {'geometry': geom.LineString([new_ep,outer_gdf.loc[outer_index0].geometry.coords[-1]]), \
                                        'highway_code': 0, 'highway': r'["outer"]', \
                                        'name:local': r'["outer"]', 'name:en': r'["outer"]', \
                                        'name:zh': r'["outer"]'}
                outer_gdf.loc[outer_index0,'geometry'] = geom.LineString([outer_gdf.loc[outer_index0].geometry.coords[0],new_ep])
            if distance_1 < 100:
                ep_proj = outer_gdf.loc[outer_index1].geometry.project(end_p1)
                new_ep = outer_gdf.loc[outer_index1].geometry.interpolate(ep_proj)
                row_geom = geom.LineString([*row_geom.coords[:-1],new_ep])
                outer_gdf.loc[len(outer_gdf)] = {'geometry': geom.LineString([new_ep,outer_gdf.loc[outer_index1].geometry.coords[-1]]), \
                                        'highway_code': 0, 'highway': r'["outer"]', \
                                        'name:local': r'["outer"]', 'name:en': r'["outer"]', \
                                        'name:zh': r'["outer"]'}
                outer_gdf.loc[outer_index1,'geometry'] = geom.LineString([outer_gdf.loc[outer_index1].geometry.coords[0],new_ep])
            road_gdf.loc[i,'geometry'] = row_geom


    road_gdf = pd.concat([road_gdf,outer_gdf])
    road_gdf.set_crs(epsg=3857, inplace=True)
    road_gdf.to_file(CITY_DIR / city_name / 'city_road_no_leaf_extended.gpkg', driver='GPKG')

def merge_street_block_by_width(sector_gdf:gpd.GeoDataFrame, min_width:float=1000)->gpd.GeoDataFrame:
    sector_gdf = sector_gdf.copy().reset_index(drop=True)
    sector_gdf['width'] = sector_gdf.geometry.apply(get_shape_width)
    small_sector_list = sector_gdf[sector_gdf['width'] < min_width].index.tolist()
    sindex = sector_gdf.sindex

    while len(small_sector_list) > 0:
        small_id = small_sector_list.pop(0)
        if small_id not in sector_gdf.index:
            continue
        row_geom: gpd.GeoSeries = sector_gdf.loc[small_id].geometry
        if row_geom is None or row_geom.is_empty:
            continue

        candidate_pos = sindex.query(row_geom, predicate='intersects')
        candidate_index = sector_gdf.iloc[candidate_pos].index
        candidate_index = [idx for idx in candidate_index if idx != small_id]

        merged_geometry = None
        merged_index = None
        max_square_score = -1.0

        for j in candidate_index:
            neighbor_geom = sector_gdf.loc[j].geometry
            if neighbor_geom is None or neighbor_geom.is_empty:
                continue

            new_geometry:geom.Polygon = ops.unary_union([row_geom, neighbor_geom])
            if new_geometry.geom_type != 'Polygon':
                continue
            if len(new_geometry.interiors) > len(neighbor_geom.interiors):
                continue

            new_obb = new_geometry.minimum_rotated_rectangle
            if new_obb.area <= 0 or new_geometry.length <= 0:
                continue

            area_ratio = new_geometry.area/new_obb.area
            compactness = (16 * new_geometry.area)/(new_geometry.length**2)
            square_score = 0.6*area_ratio + 0.4*compactness
            if square_score > max_square_score:
                max_square_score = square_score
                merged_geometry = new_geometry
                merged_index = j
        if merged_geometry is not None:
            sector_gdf.loc[merged_index,'geometry'] = merged_geometry
            sector_gdf.loc[merged_index,'width'] = get_shape_width(merged_geometry)
            sector_gdf.drop(index=small_id, inplace=True)
            if sector_gdf.loc[merged_index,'width'] < min_width:
                small_sector_list.append(merged_index)
            sindex = sector_gdf.sindex
    return sector_gdf.reset_index(drop=True)

def create_city_sector_gdf(road_gdf:gpd.GeoDataFrame,road_level:int)->gpd.GeoDataFrame:
    major_road_gdf = road_gdf[road_gdf['highway_code'].isin(range(road_level+1))]
    sector_gdf = gpd.GeoDataFrame(columns=['geometry','name'])

    major_road_union:geom.MultiLineString = major_road_gdf.geometry.union_all()

    sector_polygons = ops.polygonize(major_road_union)
    if len(sector_polygons)<2:
        return gpd.GeoDataFrame(columns=['geometry','name','area'])
    for polygon in sector_polygons:
        new_row = {'geometry': polygon,'name':f"sector_{len(sector_gdf)+1}"}
        sector_gdf.loc[len(sector_gdf)]=new_row
    sector_gdf.set_crs(road_gdf.crs, inplace=True)
    sector_gdf['width'] = sector_gdf.geometry.apply(get_shape_width)
    sector_gdf['small_sector'] = sector_gdf['width'] < 1000
    small_sector_list = sector_gdf[sector_gdf['small_sector']].index.to_list()
    sindex = sector_gdf.sindex
    while len(small_sector_list) > 0:
        small_id = small_sector_list.pop(0)
        if small_id not in sector_gdf.index:
            continue
        row_geom: gpd.GeoSeries = sector_gdf.loc[small_id].geometry
        if row_geom is None or row_geom.is_empty:
            continue
        candidate_pos = sindex.query(row_geom, predicate='intersects')
        candidate_index = sector_gdf.iloc[candidate_pos].index
        candidate_index = [idx for idx in candidate_index if idx != small_id]
        merged_geometry = None
        merged_index = None
        min_vert_cout = 999999
        max_avg_vert_distance = 0

        for j in candidate_index:
            neighbor = sector_gdf.loc[j]
            neighbor_geom = neighbor.geometry
            if neighbor_geom is None or neighbor_geom.is_empty:
                continue
            new_geometry:geom.Polygon = ops.unary_union([row_geom, neighbor_geom])
            # if new_geometry.geom_type == 'Polygon' and len(row_geom.interiors)>0 and len(new_geometry.interiors) >= len(row_geom.interiors):
            #     continue
            if new_geometry.geom_type == 'Polygon':
                origin_exterior = neighbor_geom.exterior
                exterior_line = new_geometry.exterior
                diff_line = exterior_line.difference(origin_exterior)
                if diff_line.geom_type == 'MultiLineString' or diff_line.geom_type == 'GeometryCollection':
                    diff_line = ops.linemerge(diff_line)
                if diff_line.is_empty:
                    avg_vert_distance = float('inf')
                else:
                    match(diff_line.geom_type):
                        case 'LineString':
                            diff_line = road_util.simplify_geom(diff_line,1)
                            vert_count = len(diff_line.coords)
                            avg_vert_distance = diff_line.length/len(diff_line.coords)
                        case 'MultiLineString':
                            sub_lines = [*diff_line.geoms]
                            longest_line = max(sub_lines, key=lambda x:x.length)
                            diff_line = road_util.simplify_geom(longest_line,1)
                            vert_count = len(diff_line.coords)
                            avg_vert_distance = diff_line.length/len(diff_line.coords)
                        case 'Point':
                            avg_vert_distance = float('inf')
                            vert_count = 0
                        case 'MultiPoint':
                            avg_vert_distance = float('inf')
                            vert_count = 0
                        case 'GeometryCollection':
                            sub_shape = [*diff_line.geoms]
                            longest_line = max(sub_shape, key=lambda x:x.length)
                            diff_line = road_util.simplify_geom(longest_line,10)
                            vert_count = len(diff_line.coords)
                            avg_vert_distance = diff_line.length/len(diff_line.coords)
                    if vert_count < min_vert_cout:
                        min_vert_cout = vert_count
                        merged_geometry = new_geometry
                        merged_index = j
                        max_avg_vert_distance = avg_vert_distance
                    elif vert_count == min_vert_cout and avg_vert_distance > max_avg_vert_distance:
                        max_avg_vert_distance = avg_vert_distance
                        merged_geometry = new_geometry
                        merged_index = j
        if merged_geometry is not None:
            sector_gdf.loc[merged_index,'geometry'] = merged_geometry
            sector_gdf.loc[merged_index,'width'] = get_shape_width(merged_geometry)
            sector_gdf.drop(index=small_id, inplace=True)
            if sector_gdf.loc[merged_index,'width'] < 1000:
                small_sector_list.append(merged_index)
            sindex = sector_gdf.sindex
    sector_gdf['have_hole'] = sector_gdf.geometry.apply(lambda x: len(x.interiors)>0)
    # sector_gdf['area'] = sector_gdf.geometry.area

    sector_w_hole_list = sector_gdf[sector_gdf['have_hole']].index.to_list()
    for i in sector_w_hole_list:
        if i in sector_gdf.index:
            row_geom: gpd.GeoSeries = sector_gdf.loc[i].geometry
            geom_no_hole = geom.Polygon(row_geom.exterior)
            new_row = sector_gdf.loc[i].to_dict()
            new_row['geometry'] = geom_no_hole
            sector_in_geom = sector_gdf[sector_gdf.geometry.apply(lambda x: geom_no_hole.contains(x))].index.to_list()
            sector_gdf.drop(index=sector_in_geom, inplace=True)
            sector_gdf.loc[i]=new_row
    if len(sector_gdf)<2:
        return gpd.GeoDataFrame(columns=['geometry','name','area'])

    # sector_gdf.plot()
    # plt.show()
    sector_gdf.set_crs(road_gdf.crs, inplace=True)
    sector_gdf['area'] = sector_gdf.geometry.area
    return sector_gdf

def create_city_sector(city_name:str):
    save_dir = CITY_DIR / city_name
    road_path = save_dir / 'city_road_no_leaf_extended.gpkg'
    sector_path = save_dir / 'city_sector.gpkg'
    # dbg_dir = save_dir / 'debug'
    if sector_path.exists():
        sector_path.unlink()
    # if not dbg_dir.exists():
    #     dbg_dir.mkdir()
    road_gdf = gpd.read_file(road_path)
    road_gdf = road_gdf.to_crs(epsg=3857)
    road_gdf = road_gdf[road_gdf.geometry.notna()].copy().reset_index(drop=True)
    road_sindex = road_gdf.sindex
    sector_gdf = create_city_sector_gdf(road_gdf,2)
    sector_lvl = 0
    sector_gdf.to_file(sector_path, driver='GPKG',layer=f'lvl_{sector_lvl}')
    sector_gdf.reset_index(inplace=True,drop=True)
    sector_large_gdf = sector_gdf[sector_gdf['area'] > 10**7]
    while len(sector_large_gdf) > 0:
        sector_lvl += 1
        sector_next_gdf = gpd.GeoDataFrame(columns=['geometry','name'])
        for i, row in sector_large_gdf.iterrows():
            row: gpd.GeoSeries
            candidate_pos = road_sindex.query(row.geometry, predicate='covers')
            road_in_sector = road_gdf.iloc[np.unique(candidate_pos)].copy()
            # road_in_sector.to_file(dbg_dir/f"sector_{sector_lvl}_{i}.gpkg",layer='road')
            if len(road_in_sector) > 0:
                road_in_sector = road_util.extend_leaf_road(road_in_sector,3+sector_lvl,450)
                sub_sector_gdf = create_city_sector_gdf(road_in_sector,3+sector_lvl)
                sub_sector_gdf['parent_id'] = i
                if sub_sector_gdf is not None and len(sub_sector_gdf) > 0:
                    sector_next_gdf = pd.concat([sector_next_gdf, sub_sector_gdf])
                    # sub_sector_gdf.to_file(dbg_dir/f"sector_{sector_lvl}_{i}.gpkg",layer='sub_sector')
        sector_next_gdf.set_crs(road_gdf.crs, inplace=True)
        sector_next_gdf['area'] = sector_next_gdf.geometry.area
        sector_next_gdf.reset_index(inplace=True,drop=True)
        if len(sector_next_gdf) >0:
            sector_next_gdf.to_file(sector_path, driver='GPKG',layer=f'lvl_{sector_lvl}')
        sector_large_gdf = sector_next_gdf[sector_next_gdf['area'] > 10**7]


def get_street_block_obb(city_name: str):
    save_dir = CITY_DIR / city_name
    street_block_gpkg = save_dir / 'city_street_block.gpkg'
    street_block_gdf = gpd.read_file(street_block_gpkg)
    street_block_gdf = street_block_gdf.to_crs(epsg=3857)
    block_obb_gdf = gpd.GeoDataFrame(columns=['geometry'], crs=street_block_gdf.crs)
    for i, row in street_block_gdf.iterrows():
        row: gpd.GeoSeries
        block_obb_gdf.loc[i] = {'geometry': geom.LineString(row.geometry.minimum_rotated_rectangle.exterior)}
    block_obb_gdf.crs = street_block_gdf.crs
    block_obb_gdf.to_file(save_dir / 'city_block_obb.gpkg',layer='block_obb', driver='GPKG')

def buffer_road_by_type(city_name: str):
    save_dir = CITY_DIR / city_name
    city_gpkg = save_dir / 'city.gpkg'
    road_gdf = gpd.read_file(city_gpkg, layer='city_road')
    # rename road to 'unnamed' if name:local is None
    road_gdf['name:local'] = road_gdf['name:local'].fillna('unnamed')
    road_gdf['highway'] = road_gdf['highway'].fillna('unclassified')
    road_gdf = road_gdf.dissolve(by = ['highway', 'name:local'], as_index=False)
    road_dissove = road_gdf.copy()
    road_dissove.to_crs(epsg=3857, inplace=True)
    buffer_distances = road_dissove['highway'].map(HIGHWAY_BUFFER).fillna(HIGHWAY_BUFFER[None])
    road_dissove['geometry'] = [
        geometry.buffer(distance)
        for geometry, distance in zip(road_dissove.geometry, buffer_distances)
    ]
    road_dissove.to_file(save_dir / 'road_buffer_alt.gpkg', driver='GPKG')
    road_buffer_geom = road_dissove.geometry.union_all()
    if road_buffer_geom.geom_type == 'Polygon':
        max_poly = road_buffer_geom
    else:
        max_poly = max(road_buffer_geom.geoms, key=lambda polygon: polygon.area)
    road_buffer = gpd.GeoDataFrame({'geometry': [max_poly]}, crs=road_dissove.crs)
    road_buffer.to_file(save_dir / 'road_buffer.gpkg', driver='GPKG')

def add_landmark_surrounding_road(city_name: str):
    save_dir = CITY_DIR / city_name
    land_mark_gpkg = save_dir / 'landmark_polygons.gpkg'
    land_mark_gdf = gpd.read_file(land_mark_gpkg)
    land_mark_gdf = land_mark_gdf[land_mark_gdf.geometry.notna()]
    land_mark_gdf = land_mark_gdf.to_crs(epsg=3857)
    road_gpkg = save_dir / 'city_road_out_block.gpkg'
    road_gdf = gpd.read_file(road_gpkg)
    road_gdf = road_gdf.to_crs(epsg=3857)
    land_mark_outer_gdf  = land_mark_gdf.copy()
    land_mark_outer_gdf.geometry = land_mark_outer_gdf.geometry.apply(lambda x: geom.LineString(x.exterior))
    land_mark_outer_splited_gdf = gpd.GeoDataFrame(columns=road_gdf.columns, crs=land_mark_outer_gdf.crs)
    road_union = road_gdf.union_all()
    for i, row in land_mark_outer_gdf.iterrows():
        row: gpd.GeoSeries
        row_dict = row.to_dict()
        row_dict['name:local'] = json.dumps(["landmark_outer"], ensure_ascii=False)
        row_dict['highway'] = json.dumps(["tertiary"], ensure_ascii=False)
        row_dict['name:en'] = json.dumps(["landmark_outer"], ensure_ascii=False)
        row_dict['name:zh'] = json.dumps([None], ensure_ascii=False)
        row_dict['highway_code'] = 5
        splited = ops.split(row.geometry, road_union)
        if len(splited.geoms) > 1:
            for j, sub_row in enumerate(splited.geoms):
                if sub_row.geom_type == 'LineString':
                    land_mark_outer_splited_gdf.loc[len(land_mark_outer_splited_gdf)] = row_dict.copy()
                    land_mark_outer_splited_gdf.loc[len(land_mark_outer_splited_gdf)-1, 'geometry'] = sub_row
    land_mark_outer_splited_gdf.set_crs(land_mark_outer_gdf.crs, inplace=True)
    land_mark_outer_splited_gdf.to_file(save_dir / 'landmark_outer_splited.gpkg', driver='GPKG')
    # split and remove roads that covered by landmark
    masked_lines = gpd.overlay(road_gdf, land_mark_gdf, how='difference')
    masked_lines = masked_lines.explode()
    masked_lines = masked_lines[masked_lines.geometry.length > 25]
    masked_lines.set_crs(road_gdf.crs, inplace=True)
    masked_lines.to_file(save_dir / 'city_road_out_landmark.gpkg', driver='GPKG')
    land_mark_outer_splited_gdf.geometry = land_mark_outer_splited_gdf.geometry.snap(masked_lines.union_all(),10)
    out_road_gdf = pd.concat([masked_lines, land_mark_outer_splited_gdf])
    out_road_gdf.reset_index(inplace=True,drop=True)
    out_road_gdf['road_id'] = out_road_gdf.index
    out_road_gdf.to_file(save_dir / 'city_road_w_landmark.gpkg', driver='GPKG')

    merged_gdf = out_road_gdf.geometry.union_all()
    polygons:geom.MultiPolygon = list(ops.polygonize(merged_gdf))

    blocks = gpd.GeoDataFrame(crs=road_gdf.crs, geometry=polygons)

    land_mark_gdf.geometry = land_mark_gdf.geometry.apply(lambda x: x.buffer(10))
    # remove street block that covered by landmark
    blocks_in_landmark = gpd.sjoin(blocks, land_mark_gdf, how='inner', predicate='within')
    blocks_out_landmark = blocks[~blocks.index.isin(blocks_in_landmark.index)]
    blocks_out_landmark.to_file(save_dir / 'city_block_out_landmark.gpkg', driver='GPKG')


def remove_degree_2_node(city_name: str):
    save_dir = CITY_DIR / city_name
    road_gpkg = save_dir / 'city_road_w_landmark.gpkg'
    road_gdf = gpd.read_file(road_gpkg)
    road_gdf = road_gdf.to_crs(epsg=3857)
    road_gdf = road_gdf[road_gdf.geometry.isna() == False]
    road_network = momepy.gdf_to_nx(road_gdf, approach='primal', multigraph=True)
    # remove degree 2 node and disslove 2 edges
    while True:
        node_to_remove = []
        edge_to_add = []
        for node in road_network.nodes:
            edges = list(road_network.edges(node))
            if len(edges) == 2:
                node_to_remove.append(node)
                edge0 = edges[0]
                edge1 = edges[1]

                node0 = edge0[0] if edge0[0] != node else edge0[1]
                node1 = edge1[0] if edge1[0] != node else edge1[1]
                data0 = road_network.get_edge_data(node,node0)[0]
                data1 = road_network.get_edge_data(node,node1)[0]
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

    #_prune_short_leaf_spurs(road_network)
    road_gdf = momepy.nx_to_gdf(road_network, points=False, lines=True)
    road_gdf.reset_index(inplace=True,drop=True)
    road_gdf['road_id'] = road_gdf.index
    road_gdf.to_file(save_dir / 'city_road_w_landmark_degree2.gpkg', driver='GPKG')

def gen_street_block_landmark(city_name:str):
    road_gpkg = CITY_DIR / city_name / 'city_road_w_landmark_degree2.gpkg'
    landmark_geojson = CITY_DIR / city_name / 'landmark_polygons.gpkg'
    road_gdf = gpd.read_file(road_gpkg)
    landmark_gdf = gpd.read_file(landmark_geojson)
    road_gdf = road_gdf.to_crs(epsg=3857)
    landmark_gdf = landmark_gdf.to_crs(epsg=3857)
    landmark_gdf = landmark_gdf[landmark_gdf.geometry.isna() == False]

    merged_gdf = road_gdf.geometry.union_all()
    polygons:geom.MultiPolygon = list(ops.polygonize(merged_gdf))

    blocks = gpd.GeoDataFrame(crs=road_gdf.crs, geometry=polygons)

    landmark_gdf.geometry = landmark_gdf.geometry.apply(lambda x: x.buffer(50))
    # remove street block that covered by landmark
    blocks_in_landmark = gpd.sjoin(blocks, landmark_gdf, how='inner', predicate='within')
    blocks_out_landmark = blocks[~blocks.index.isin(blocks_in_landmark.index)]
    blocks_out_landmark.to_file(CITY_DIR / city_name / 'city_block_out_landmark.gpkg', driver='GPKG')


def simplify_city_roads(city_name: str, simplify_tolerance: float = 50.0):
    save_dir = CITY_DIR / city_name

    buffer_road_by_type(city_name)
    buffered_road_2_raster(city_name)

    rater_skeletonize(city_name)

    remove_close_edge(city_name,simplify_tolerance*1.5)
    match_segment_to_road(city_name)
    simplified_road_gpkg = save_dir / 'city_road_simplified.gpkg'
    road_util.dP_simplification(save_dir / 'city_matched.gpkg',simplified_road_gpkg, simplify_tolerance)
    remove_leaf_node(simplified_road_gpkg, save_dir / 'city_road_no_leaf.gpkg')
    # sr.remove_line_cycle(save_dir / 'city_road_no_leaf.gpkg', save_dir / 'city_road_no_circle.gpkg', cycle_length=500, dangle_length=50)
    extend_major_road_leaf(city_name,600)
    create_city_sector(city_name)
    gen_street_block(city_name)
    merge_small_street_block(city_name)
    add_landmark_surrounding_road(city_name)
    remove_degree_2_node(city_name)
    gen_street_block_landmark(city_name)
    # fill_street_block_with_voronoi(city_name)

def city_2_simple_road(city_name: str, simplify_tolerance: float = 50.0):
    save_dir = CITY_DIR / city_name

    buffer_road_by_type(city_name)
    buffered_road_2_raster(city_name)

    rater_skeletonize(city_name)

    remove_close_edge(city_name,simplify_tolerance*1.5)
    match_segment_to_road(city_name)
    simplified_road_gpkg = save_dir / 'city_road_simplified.gpkg'
    road_util.dP_simplification(save_dir / 'city_matched.gpkg',simplified_road_gpkg, simplify_tolerance)
    remove_leaf_node(simplified_road_gpkg, save_dir / 'city_road_no_leaf.gpkg')
    # sr.remove_line_cycle(save_dir / 'city_road_no_leaf.gpkg', save_dir / 'city_road_no_circle.gpkg', cycle_length=500, dangle_length=50)
    extend_major_road_leaf(city_name,600)
    create_city_sector(city_name)
    gen_street_block(city_name)
    merge_small_street_block(city_name)

def city_add_landmark(city_name: str):

    add_landmark_surrounding_road(city_name)
    remove_degree_2_node(city_name)
    # gen_street_block_landmark(city_name)


def update_or_create_SimplifyTask(city, tolerance, result_path: pathlib.Path):
    session = SessionLocal()

    task = session.query(SimplyfyTask).filter(
        SimplyfyTask.city == city,
        SimplyfyTask.tolerance == tolerance
    ).first()

    if task is None:
        task = SimplyfyTask(
            city=city,
            tolerance=tolerance,
            status="finished",
            result_path=result_path.as_posix()
        )
        session.add(task)
    else:
        task.status = "finished"
        task.result_path = result_path.as_posix()

    session.commit()
    session.close()

def update_or_create_LandmarkTask(city, tolerance, result_path: pathlib.Path):
    session = SessionLocal()

    task = session.query(LandmarkTask).filter(
        LandmarkTask.city == city,
    ).first()

    if task is None:
        task = LandmarkTask(
            city=city,
            status="finished",
            result_path=result_path.as_posix()
        )
        session.add(task)
    else:
        task.status = "finished"
        task.result_path = result_path.as_posix()

    session.commit()
    session.close()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Simplify road network of a city')
    parser.add_argument('city_name', nargs='?', type=str, help='Name of the city')
    parser.add_argument('-s','--step', type=int, default=1, help='Simplify step')
    parser.add_argument('-t','--simplify_tolerance', type=float, default=50.0, help='Simplify tolerance')
    parser.add_argument('-r','--radius', type=float, default=50.0, help='Radius of the city Km')
    parser.add_argument('--batch-step3', action='store_true', help='Run step 3 for the default city list')
    gdal.DontUseExceptions()
    args = parser.parse_args()
    # when no argument run simplify_city_roads('Beijing')
    if args.batch_step3:
        failed_cities = []
        for city_name in BATCH_STEP3_CITY_NAMES:
            try:
                print(f'Run step 3 for {city_name}')
                gdf_2_network(city_name, args.simplify_tolerance)
            except Exception as e:
                failed_cities.append(city_name)
                print(f'Failed step 3 for {city_name}: {e}')
        print(f'Completed step 3: {len(BATCH_STEP3_CITY_NAMES) - len(failed_cities)}/{len(BATCH_STEP3_CITY_NAMES)} cities')
        if failed_cities:
            print(f"Failed cities: {', '.join(failed_cities)}")
    elif args.city_name is None:
        simplify_city_roads('Beijing')
    else:
        city_name = args.city_name
        simplify_tolerance = args.simplify_tolerance
        radius = args.radius * 1000
        if args.step == 1:
            try:
                city_2_simple_road(city_name, simplify_tolerance)
                update_or_create_SimplifyTask(city_name, simplify_tolerance, CITY_DIR / city_name / 'city_road_out_block.gpkg')
            except Exception as e:
                print(e)
                raise Exception(e)
        elif args.step == 2:
            try:
                city_add_landmark(city_name)
                update_or_create_LandmarkTask(city_name, simplify_tolerance, CITY_DIR / city_name / 'city_road_w_landmark_degree2.gpkg')
            except Exception as e:
                print(e)
                raise Exception(e)
        elif args.step == 3:
            filter_city_radius(city_name,radius)
            gdf_2_network(city_name, simplify_tolerance)

