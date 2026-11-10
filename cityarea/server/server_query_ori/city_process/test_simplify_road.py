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

from common.db import SessionLocal
from common.model import SimplyfyTask,LandmarkTask

MAP_OFFSET = [-2097152.0, -1048576.0];
CITY_DIR = DATA_DIR / 'city'
HIGHWAY_BUFFER = {
    'motorway': 120,
    'trunk': 100,
    'primary': 75,
    'secondary': 50,
    'tertiary': 30,
    'unclassified': 25,
    'residential': 25,
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
    'residential': 7,
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


def _merge_degree_2_edge_data(data0, data1):
    geom0 = data0['geometry']
    geom1 = data1['geometry']
    new_geom = ops.linemerge([geom0, geom1])
    if new_geom.geom_type != 'LineString':
        print(f'Warning: merge two edges skipped, new_geom type is {new_geom.geom_type}')
        return None

    new_data = data0.copy() if geom0.length > geom1.length else data1.copy()
    new_data['geometry'] = new_geom
    new_data['mm_len'] = new_geom.length
    return new_data


def _iter_line_geometries(geometry):
    if geometry is None or geometry.is_empty:
        return
    if geometry.geom_type == 'LineString':
        yield geometry
    elif geometry.geom_type == 'MultiLineString':
        for line in geometry.geoms:
            if not line.is_empty:
                yield line
    elif geometry.geom_type == 'GeometryCollection':
        for sub_geom in geometry.geoms:
            yield from _iter_line_geometries(sub_geom)


def _candidate_source_indices(road_gdf, line):
    try:
        return list(road_gdf.sindex.query(line, predicate='intersects'))
    except Exception:
        try:
            return list(road_gdf.sindex.query(line))
        except Exception:
            return list(range(len(road_gdf)))


def _copy_best_matching_road_row(road_gdf, line):
    candidate_indices = _candidate_source_indices(road_gdf, line)
    if len(candidate_indices) == 0:
        candidate_indices = list(range(len(road_gdf)))

    best_pos = None
    best_overlap = -1.0
    best_distance = float('inf')
    for pos in candidate_indices:
        candidate_geom = road_gdf.iloc[pos].geometry
        try:
            overlap = candidate_geom.intersection(line).length
        except Exception:
            overlap = 0.0
        distance = candidate_geom.distance(line)
        if overlap > best_overlap or (overlap == best_overlap and distance < best_distance):
            best_pos = pos
            best_overlap = overlap
            best_distance = distance

    row = road_gdf.iloc[best_pos].copy()
    row.geometry = line
    return row


def _prune_dangling_degree_linework(road_gdf):
    if len(road_gdf) == 0:
        return road_gdf

    noded_linework = road_gdf.geometry.union_all()
    line_graph = nx.Graph()
    for line in _iter_line_geometries(noded_linework):
        if line.length <= 1e-6:
            continue
        start = tuple(line.coords[0])
        end = tuple(line.coords[-1])
        if start == end:
            continue
        line_graph.add_edge(start, end, geometry=line)

    _prune_short_leaf_spurs(line_graph)

    rows = []
    for _, _, data in line_graph.edges(data=True):
        rows.append(_copy_best_matching_road_row(road_gdf, data['geometry']))

    cleaned_gdf = gpd.GeoDataFrame(rows, columns=road_gdf.columns, crs=road_gdf.crs)
    return cleaned_gdf



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
    data_bytes = in_band.ReadRaster(0, 0, in_ds.RasterXSize, in_ds.RasterYSize,
                             buf_type=gdal.GDT_Byte)


    in_data = np.frombuffer(data_bytes, dtype=np.byte).reshape(in_ds.RasterYSize, in_ds.RasterXSize)
    skel = skeletonize(in_data)
    graph = sknw.build_sknw(skel)
    geom_graph = nx.Graph()
    for s,e in graph.edges():
        ps = graph[s][e]['pts']
        pt_list = []
        for pts in ps:
            x = (pts[1] + 0.5) * x_res + x_min
            y = (pts[0] + 0.5) * y_res + y_max
            pt_list.append(geom.Point(x, y))
        geometry = geom.LineString(pt_list)
        geom_graph.add_edge(s, e, geometry=geometry,id = len(geom_graph.edges()), mm_len=geometry.length)
        pass
    geom_graph.graph['approach'] = 'primal'
    gdf = momepy.nx_to_gdf(geom_graph, points=False, lines=True)
    gdf.set_crs(epsg=3857, inplace=True)
    gdf.to_file(output_path, driver='GPKG')
    pass


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
    city_sector_path = save_dir / 'city_sector.gpkg'
    layers_list = fiona.listlayers(city_sector_path)
    city_sector_gdfs = [gpd.read_file(city_sector_path, layer=layer) for layer in layers_list]
    sector_union = city_sector_gdfs[0].geometry.union_all()
    road_gdf = road_gdf[road_gdf.geometry.apply(lambda x: sector_union.covers(x))]
    street_block_gdf = gpd.GeoDataFrame(crs=road_gdf.crs, columns=['geometry','sector_id','sector_name'])

    def merge_small_street_block_by_sector(sector_level: int, sector_polygon:geom.Polygon, sector_id:int):
        if sector_level < len(layers_list):
            next_level_sector_gdf = city_sector_gdfs[sector_level]
            sub_sectors = next_level_sector_gdf[next_level_sector_gdf.geometry.covered_by(sector_polygon)]
        else:
            sub_sectors = None
        if sub_sectors is not None and len(sub_sectors) > 0:
            for i, sub_sector in sub_sectors.iterrows():
                sub_sector: gpd.GeoSeries
                sub_sector_geom = sub_sector.geometry
                if sub_sector_geom.geom_type == 'Polygon':
                    merge_small_street_block_by_sector(sector_level+1, sub_sector_geom, i)
        else:
            road_intersected = road_gdf.geometry.apply(lambda x: sector_polygon.covers(x))
            road_intersected_gdf = road_gdf[road_intersected]
            if len(road_intersected_gdf) > 0:
                merged_geom = road_intersected_gdf.geometry.union_all()
                polygons:list[geom.Polygon] = list(ops.polygonize(merged_geom))
                for interior in sector_geom.interiors:
                    #remove holes from polygons list
                    polygons = [p for p in polygons if not p.contains(interior.centroid)]

                if len(polygons)==0:
                    return
                street_blocks = gpd.GeoDataFrame(crs=road_gdf.crs, geometry=polygons)
                street_blocks['sector_lvl'] = sector_level - 1
                street_blocks['sector_id'] = sector_id
                merged_street_block_gdf = merge_street_block_by_width(street_blocks, min_width=min_width)
                nonlocal street_block_gdf
                street_block_gdf = pd.concat([street_block_gdf, merged_street_block_gdf])

    for i, row in city_sector_gdfs[0].iterrows():
        row: gpd.GeoSeries
        sector_geom = row.geometry
        if sector_geom.geom_type == 'Polygon':
            sector_geom:geom.Polygon
            merge_small_street_block_by_sector(1, sector_geom,i)

    street_block_gdf.set_crs(epsg=3857, inplace=True)
    street_block_gdf.reset_index(drop=True, inplace=True)
    street_block_gdf['block_id'] = street_block_gdf.index
    street_block_gdf.to_file(street_block_path, driver='GPKG')
    road_in_street_block: gpd.GeoDataFrame = road_gdf.sjoin(street_block_gdf, how='inner', predicate='within')
    road_out_street_block: gpd.GeoDataFrame = road_gdf[~road_gdf.index.isin(road_in_street_block.index)]
    road_out_street_block.reset_index(drop=True, inplace=True)
    road_out_street_block['road_id'] = road_out_street_block.index
    street_block_c_road = gpd.sjoin(street_block_gdf, road_out_street_block, how='inner', predicate='touches')
    street_block_c_road.to_file(save_dir / 'city_street_block_c_road.gpkg', driver='GPKG')
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

def extend_major_road_leaf(city_name: str, min_leaf_length: float = 450):
    road_gdf = gpd.read_file(CITY_DIR / city_name / 'city_road_no_leaf.gpkg')
    road_gdf = road_util.extend_leaf_road(road_gdf,2,min_leaf_length)
    road_gdf['to_remove'] = False
    major_road_gdf = road_gdf[road_gdf['highway_code'].isin(range(2+1))]
    outer_ring = major_road_gdf.dissolve().convex_hull.exterior[0]

    def _line_intersection_points(intersection):
        if intersection is None or intersection.is_empty:
            return None
        if intersection.geom_type == 'LineString':
            return geom.MultiPoint((intersection.coords[0], intersection.coords[-1]))
        if intersection.geom_type == 'MultiLineString':
            points = []
            for line in intersection.geoms:
                if line.is_empty:
                    continue
                points.append(geom.Point(line.coords[0]))
                points.append(geom.Point(line.coords[-1]))
            return geom.MultiPoint(points) if points else None
        if intersection.geom_type in ('Point', 'MultiPoint'):
            return intersection
        if intersection.geom_type == 'GeometryCollection':
            points = []
            for sub_geom in intersection.geoms:
                sub_points = _line_intersection_points(sub_geom)
                if sub_points is None or sub_points.is_empty:
                    continue
                if sub_points.geom_type == 'Point':
                    points.append(sub_points)
                elif sub_points.geom_type == 'MultiPoint':
                    points.extend(sub_points.geoms)
            return geom.MultiPoint(points) if points else None
        return None

    for i, row in major_road_gdf.iterrows():
        row:gpd.GeoSeries
        row_geom:geom.LineString = row.geometry
        if row_geom.intersects(outer_ring):
            new_row = row.to_dict()
            intersected_geom = row_geom.intersection(outer_ring)
            intersect_point = _line_intersection_points(intersected_geom)
            if intersect_point is None or intersect_point.is_empty:
                continue
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
    sector_gdf['width'] = sector_gdf.geometry.apply(lambda x: get_shape_width(x))
    # sector_gdf['area'] = sector_gdf.geometry.area
    small_sector_list = sector_gdf[sector_gdf['width'] < min_width].index.tolist()
    while len(small_sector_list) > 0:
        small_id = small_sector_list.pop(0)
        if small_id not in sector_gdf.index:
            continue
        row_geom: gpd.GeoSeries = sector_gdf.loc[small_id].geometry
        neighbors = sector_gdf[sector_gdf.geometry.intersects(row_geom)]
        neighbors = neighbors[neighbors.index != small_id]
        merged_geometry = None
        merged_index = None
        max_square_score = 0

        for j, neighbor in neighbors.iterrows():
            neighbor: gpd.GeoSeries
            new_geometry:geom.Polygon = ops.unary_union([row_geom, neighbor.geometry])
            if new_geometry.geom_type == 'Polygon' and (len(new_geometry.interiors)<=len(neighbor.geometry.interiors)) :
                new_obb = new_geometry.minimum_rotated_rectangle
                area_ratio = new_geometry.area/new_obb.area
                compactness = (16 * new_geometry.area)/(new_geometry.length**2)
                square_score = 0.6*area_ratio + 0.4*  compactness
                if square_score > max_square_score:
                    max_square_score = square_score
                    merged_geometry = new_geometry
                    merged_index = j
        if merged_geometry is not None:
            sector_gdf.loc[merged_index,'geometry'] = merged_geometry
            sector_gdf.drop(index=small_id, inplace=True)
            if get_shape_width(merged_geometry) < min_width:
                small_sector_list.append(merged_index)
    return sector_gdf

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
    sector_gdf['width'] = sector_gdf.geometry.apply(lambda x: get_shape_width(x))
    sector_gdf['small_sector'] = sector_gdf['width'] < 1000
    small_sector_list = sector_gdf[sector_gdf['small_sector']].index.to_list()
    while len(small_sector_list) > 0:
        small_id = small_sector_list.pop(0)
        if small_id not in sector_gdf.index:
            continue
        row_geom: gpd.GeoSeries = sector_gdf.loc[small_id].geometry
        neighbors = sector_gdf[sector_gdf.geometry.intersects(row_geom)]
        neighbors = neighbors[neighbors.index != small_id]
        merged_geometry = None
        merged_index = None
        min_vert_cout = 999999
        max_avg_vert_distance = 0

        for j, neighbor in neighbors.iterrows():
            neighbor: gpd.GeoSeries
            new_geometry:geom.Polygon = ops.unary_union([row_geom, neighbor.geometry])
            # if new_geometry.geom_type == 'Polygon' and len(row_geom.interiors)>0 and len(new_geometry.interiors) >= len(row_geom.interiors):
            #     continue
            if new_geometry.geom_type == 'Polygon':
                origin_exterior = neighbor.geometry.exterior
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
            sector_gdf.drop(index=small_id, inplace=True)
            if get_shape_width(merged_geometry) < 1000:
                small_sector_list.append(merged_index)
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
            road_in_sector = road_gdf[road_gdf.geometry.apply(lambda x: row.geometry.covers(x))]
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
    for i, row in road_dissove.iterrows():
        row: gpd.GeoSeries
        type = row['highway']
        row['geometry'] = row.geometry.buffer(HIGHWAY_BUFFER[type])
        road_dissove.loc[i] = row
    road_dissove.to_file(save_dir / 'road_buffer_alt.gpkg', driver='GPKG')
    road_buffer = road_dissove.dissolve()
    polygons:geom.MultiPolygon = road_buffer.geometry.union_all()
    max_area = 0
    max_poly = None
    for polygon in polygons.geoms:
        if polygon.area > max_area:
            max_area = polygon.area
            max_poly = polygon
    road_buffer = gpd.GeoDataFrame({'geometry': [max_poly]}, crs=road_buffer.crs)
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
                edge0 = edges[0]
                edge1 = edges[1]

                node0 = edge0[0] if edge0[0] != node else edge0[1]
                node1 = edge1[0] if edge1[0] != node else edge1[1]
                data0 = road_network.get_edge_data(node,node0)[0]
                data1 = road_network.get_edge_data(node,node1)[0]
                new_data = _merge_degree_2_edge_data(data0, data1)
                if new_data is None:
                    continue

                node_to_remove.append(node)
                edge_to_add.append((node1,node0,new_data))
                break
        if len(node_to_remove) == 0:
            break
        road_network.remove_nodes_from(node_to_remove)
        for u,v,data in edge_to_add:
            road_network.add_edge(u,v,**data)

    _prune_short_leaf_spurs(road_network)
    road_gdf = momepy.nx_to_gdf(road_network, points=False, lines=True)
    road_gdf = _prune_dangling_degree_linework(road_gdf)
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
    parser.add_argument('city_name', type=str, help='Name of the city')
    parser.add_argument('-s','--step', type=int, default=1, help='Simplify step')
    parser.add_argument('-t','--simplify_tolerance', type=float, default=50.0, help='Simplify tolerance')
    args = parser.parse_args()
    # when no argument run simplify_city_roads('Beijing')
    if args.city_name is None:
        simplify_city_roads('Beijing')
    else:
        city_name = args.city_name
        simplify_tolerance = args.simplify_tolerance
        if args.step == 1:
            try:
                city_2_simple_road(city_name, simplify_tolerance)
                update_or_create_SimplifyTask(city_name, simplify_tolerance, CITY_DIR / city_name / 'city_road_out_block.gpkg')
            except Exception as e:
                print(e)
        else:
            try:
                city_add_landmark(city_name)
                update_or_create_LandmarkTask(city_name, simplify_tolerance, CITY_DIR / city_name / 'city_road_w_landmark_degree2.gpkg')
            except Exception as e:
                print(e)
