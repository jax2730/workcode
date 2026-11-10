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

ogr.DontUseExceptions()

CITY_RASTER_SIZE = 10240

HIGHWAY_BUFFER = {
    'motorway': 120,
    'trunk': 100,
    'primary': 75,
    'secondary': 50,
    'tertiary': 30,
    'unclassified': 30,
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

        
def buffer_road_by_type(input_path: pathlib.Path, output_path: pathlib.Path, 
                        union_path: pathlib.Path, 
                        buffer_dict: dict = HIGHWAY_BUFFER):
    road_gdf = gpd.read_file(input_path, layer='city_road')
    road_gdf = road_gdf.dissolve(by = ['highway', 'name:local'], as_index=False)
    road_dissove = road_gdf.copy()
    road_dissove.to_crs(epsg=3857, inplace=True)
    for i, row in road_dissove.iterrows():
        row: gpd.GeoSeries
        type = row['highway']
        row['geometry'] = row.geometry.buffer(buffer_dict[type])
        road_dissove.loc[i] = row
    road_dissove.to_file(output_path, driver='GPKG')
    road_buffer = road_dissove.dissolve()
    polygons:geom.MultiPolygon = road_buffer.geometry.union_all()
    max_area = 0
    max_poly = None
    for polygon in polygons.geoms:
        if polygon.area > max_area:
            max_area = polygon.area
            max_poly = polygon
    road_buffer = gpd.GeoDataFrame(geometry=[max_poly], crs=road_buffer.crs)
    road_buffer.to_file(union_path, driver='GPKG')

def buffered_road_2_skeletons(input_path: pathlib.Path, output_path: pathlib.Path):
    global CITY_RASTER_SIZE
    in_ds = ogr.Open(str(input_path),0)
    if in_ds is None:
        raise Exception("Could not open %s" % input_path)
    in_lyr = in_ds.GetLayer()
    bounds = in_lyr.GetExtent()
    x_min, x_max, y_min, y_max = bounds
    x_min, x_max, y_min, y_max = bounds
    x_res = (x_max - x_min) / CITY_RASTER_SIZE
    y_res = (y_min - y_max) / CITY_RASTER_SIZE

    raster_mem = gdal.GetDriverByName('MEM').Create('', CITY_RASTER_SIZE, CITY_RASTER_SIZE, 1, gdal.GDT_Byte)
    raster_mem.SetGeoTransform((x_min, x_res, 0, y_max, 0, y_res))
    raster_mem.SetProjection(in_lyr.GetSpatialRef().ExportToWkt())
    raster_lyr = raster_mem.GetRasterBand(1)
    raster_lyr.SetNoDataValue(0)
    gdal.RasterizeLayer(raster_mem, [1], in_lyr, burn_values=[1])
    raster_mem.FlushCache()
    data_bytes = raster_lyr.ReadRaster(0, 0, CITY_RASTER_SIZE, CITY_RASTER_SIZE, buf_type=gdal.GDT_Byte)
    raster_data = np.frombuffer(data_bytes, dtype=np.byte).reshape(CITY_RASTER_SIZE, CITY_RASTER_SIZE)
    ske = skeletonize(raster_data)
    graph = sknw.build_sknw(ske)
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
    gdf.reset_index(inplace=True,drop=True,names=['road_id'])
    gdf['road_id'] = gdf.index
    gdf.set_crs(crs=in_lyr.GetSpatialRef().ExportToWkt(), inplace=True)
    gdf.to_crs(epsg=3857, inplace=True)
    gdf.to_file(output_path)

def remove_leaf_node(input_path: pathlib.Path, output_path: pathlib.Path):
    road_gdf = gpd.read_file(input_path)
    road_gdf = road_gdf.to_crs(epsg=3857)

    geom_graph = momepy.gdf_to_nx(road_gdf, approach='primal', multigraph=True)
    
    # remove leaf node where degree is 1 and highway is not motorway or trunk
    leaf_nodes = [node for node, degree in geom_graph.degree() if degree == 1]
    last_leafs_num = -1
    while len(leaf_nodes)>0 and len(leaf_nodes)!= last_leafs_num:
        for node in leaf_nodes:
            edges = geom_graph.edges(node)
            for s, e in edges:
                data = geom_graph.get_edge_data(s, e)
                data = data[0]
                highways = json.loads(data['highway'])
                if ('motorway' not in highways and 'trunk' not in highways) or data['geometry'].length < 150:
                    geom_graph.remove_edge(s, e)
                    geom_graph.remove_node(node)
                    break
        last_leafs_num = len(leaf_nodes)
        leaf_nodes = [node for node, degree in geom_graph.degree() if degree == 1]

    # rebuild road_gdf
    road_gdf = momepy.nx_to_gdf(geom_graph, points=False, lines=True)
    road_gdf = momepy.remove_false_nodes(road_gdf)
    road_gdf.drop(columns=['mm_len'], inplace=True)
    road_gdf.set_crs(epsg=3857, inplace=True)
    road_gdf.to_file(output_path)

def match_segment_to_road(road_buffered_path: pathlib.Path, city_path: pathlib.Path, output_path: pathlib.Path):
    global HIGHWAY_CODE
    city_gdf = gpd.read_file(city_path)
    city_gdf = city_gdf.to_crs(epsg=3857)
    road_gdf = gpd.read_file(road_buffered_path)
    road_gdf = road_gdf.to_crs(epsg=3857)

    city_gdf = city_gdf.dissolve(by=['highway','name:local'],as_index=False)
    city_gdf.reset_index(inplace=True, drop=True, names=['original_id'])
    city_gdf['original_id'] = city_gdf.index
    road_gdf.reset_index(inplace=True, drop=True, names=['road_id'])
    road_gdf['road_id'] = np.arange(len(road_gdf))

    original_cols_for_join = ['original_id', city_gdf.geometry.name]

    joined_gdf = road_gdf.sjoin(city_gdf[original_cols_for_join],how='left',predicate='within')

    finded_roads = joined_gdf[joined_gdf['original_id'].notnull()]
    un_finded_roads = road_gdf[~road_gdf.index.isin(finded_roads.road_id)]
    joined_unfind_gdf = gpd.GeoDataFrame(columns=['road_id', 'id', 'geometry', 'right_id', 'original_id'])
    for i, row in un_finded_roads.iterrows():
        row: gpd.GeoSeries
        geometry:geom.LineString = row['geometry']
        start_p = geom.Point(geometry.coords[0])
        end_p = geom.Point(geometry.coords[-1])
        centroid = geometry.centroid
        nearest_id = (city_gdf.distance(centroid) + city_gdf.distance(start_p) + city_gdf.distance(end_p)).idxmin()
        original_id = city_gdf.loc[nearest_id]['original_id']
        row_dict = {'road_id': row['road_id'], 
                    'id': len(joined_unfind_gdf)+1, 
                    'geometry': geometry, 
                    'right_id': nearest_id, 
                    'original_id': original_id}
        joined_unfind_gdf.loc[len(joined_unfind_gdf)] = row_dict
    
    joined_unfind_gdf = joined_unfind_gdf.drop_duplicates(subset=['road_id'], keep='first')
    joined_unfind_gdf.set_crs(epsg=3857, inplace=True)
    joined_gdf = pd.concat([finded_roads, joined_unfind_gdf])
    joined_gdf = joined_gdf.sort_values(
        by=['road_id']
    )

    final_gdf = gpd.GeoDataFrame(columns=['geometry', 'name:local', 'name:en', 'name:zh', 'highway'])
    road_groups = joined_gdf.groupby('road_id')
    for road_id, group in road_groups:
        group: gpd.GeoDataFrame

        original_ids = group['original_id'].tolist()

        data = [{
                    'highway':city_gdf.loc[id]['highway'],
                    'name:local':city_gdf.loc[id]['name:local'],
                    'name:en':city_gdf.loc[id]['name:en'],
                    'name:zh':city_gdf.loc[id]['name:zh']
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

        geometry = road_gdf.loc[road_gdf['road_id'] == road_id, 'geometry'].iloc[0]
        row = {'geometry': geometry, 'name:local': names_local, 'name:en': name_en, 'name:zh': name_zh, 'highway': highway}
        final_gdf.loc[len(final_gdf)] = row
    
    final_gdf['highway_code'] = final_gdf['highway'].apply(lambda x: HIGHWAY_CODE[x[0]])
    final_gdf['name:local'] = final_gdf['name:local'].apply(lambda x: json.dumps(x,ensure_ascii=False))
    final_gdf['name:en'] = final_gdf['name:en'].apply(lambda x: json.dumps(x,ensure_ascii=False))
    final_gdf['name:zh'] = final_gdf['name:zh'].apply(lambda x: json.dumps(x,ensure_ascii=False))
    final_gdf['highway'] = final_gdf['highway'].apply(lambda x: json.dumps(x,ensure_ascii=False))
    final_gdf.set_crs(epsg=3857, inplace=True)
    final_gdf['road_id'] = np.arange(len(final_gdf))
    final_gdf.to_file(output_path)

def extend_road_leaf(road_gdf:gpd.GeoDataFrame, road_level:int, min_leaf_length:float):
    road_decode_gdf = road_gdf.copy()
    major_road_gdf = road_gdf[road_decode_gdf['highway_code'].isin(range(road_level+1))]
    major_road_nx = momepy.gdf_to_nx(major_road_gdf, approach='primal', multigraph=True)
    road_nx = momepy.gdf_to_nx(road_gdf, approach='primal', multigraph=True)
    leaf_nodes = [node for node, degree in major_road_nx.degree() if degree == 1]
    while len(leaf_nodes)>0:
        node = leaf_nodes.pop(0)
        major_neighbor = list(major_road_nx.neighbors(node))[0]
        data = major_road_nx.get_edge_data(node, major_neighbor)[0]
        last_dir = np.array(node) - np.array(major_neighbor)
        last_dir = last_dir / np.linalg.norm(last_dir)
        neighbours = list(road_nx.neighbors(node))
        if len(neighbours) < 2:
            continue

        max_dot_product = -1
        node_to_extend = None
        for neighbour in neighbours:
            if neighbour == major_neighbor:
                continue
            cur_dir = np.array(neighbour) - np.array(node)
            cur_dir = cur_dir / np.linalg.norm(cur_dir)
            dot_product = np.dot(last_dir, cur_dir)
            if dot_product > max_dot_product:
                max_dot_product = dot_product
                node_to_extend = neighbour

        if node_to_extend is None:
            continue

        if data['mm_len'] < min_leaf_length:
            neighbour_data = road_nx.get_edge_data(node, node_to_extend)[0]
            new_row_geom = data['geometry']
            new_data = neighbour_data.copy()
            new_data['geometry'] = new_row_geom
            new_data['road_id'] = data['road_id']
            new_data['mm_len'] = new_row_geom.length
            road_nx.remove_edge(node, major_neighbor)
            road_nx.add_edge(node, major_neighbor, **new_data)
            major_road_nx.remove_node(node)
            neighbor_degree = major_road_nx.degree(major_neighbor)
            if neighbor_degree == 1:
                leaf_nodes.append(major_neighbor)
        else:
            neighbour_data = road_nx.get_edge_data(node, node_to_extend)[0]
            new_row_geom = neighbour_data['geometry']
            new_data = data.copy()
            new_data['geometry'] = new_row_geom
            new_data['road_id'] = neighbour_data['road_id']
            new_data['mm_len'] = new_row_geom.length
            road_nx.remove_edge(node, node_to_extend)
            road_nx.add_edge(node, node_to_extend, **new_data)
            new_data = new_data.copy()
            new_data['mm_len'] = data['mm_len'] + neighbour_data['mm_len']
            major_road_nx.add_edge(node, node_to_extend, **new_data)
            # check if node_to_extend is in major_road_nx
            neighbor_degree = major_road_nx.degree(node_to_extend)
            if neighbor_degree == 1:
                leaf_nodes.append(node_to_extend)

    road_gdf = momepy.nx_to_gdf(road_nx, points=False, lines=True)
    road_gdf.set_crs(epsg=3857, inplace=True)
    return road_gdf

def extend_highway_road(input_path: pathlib.Path, output_path: pathlib.Path, min_leaf_length:float):
    road_gdf = gpd.read_file(input_path)
    road_gdf = road_gdf.to_crs(epsg=3857)
    extended_gdf = extend_road_leaf(road_gdf, 2, min_leaf_length)
    extended_gdf.to_file(output_path)

def get_shape_width(shape: geom.Polygon) -> float:
    if not isinstance(shape, geom.Polygon):
        return float('nan')
    obb = shape.minimum_rotated_rectangle
    width = geom.LineString((obb.exterior.coords[0], obb.exterior.coords[1])).length
    height = geom.LineString((obb.exterior.coords[1], obb.exterior.coords[2])).length
    return min(width, height)

def create_city_sector_gdf(road_gdf:gpd.GeoDataFrame):
    sector_gdf = gpd.GeoDataFrame(columns=['geometry','name'])

    major_road_union:geom.MultiLineString = road_gdf.geometry.union_all()

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

    sector_gdf.set_crs(road_gdf.crs, inplace=True)
    sector_gdf['area'] = sector_gdf.geometry.area
    return sector_gdf

def create_city_sector(road_path: pathlib.Path, sector_path: pathlib.Path):
    if sector_path.exists():
        sector_path.unlink()
    road_gdf = gpd.read_file(road_path)
    road_gdf = road_gdf.to_crs(epsg=3857)
    road_gdf = extend_road_leaf(road_gdf,2,450)
    major_road_gdf = road_gdf[road_gdf['highway_code'].isin(range(2+1))].copy()
    major_road_union = major_road_gdf.geometry.union_all()
    outer_geom = geom.LineString(major_road_union.convex_hull.exterior)
    outer_segments = [geom.LineString((outer_geom.coords[i],outer_geom.coords[i+1])) for i in range(len(outer_geom.coords)-1)]
    outer_gdf = gpd.GeoDataFrame(crs=major_road_gdf.crs, geometry=outer_segments, columns=major_road_gdf.columns)
    outer_gdf['highway_code'] = 1
    major_road_gdf = pd.concat([major_road_gdf, outer_gdf])
    road_gdf = pd.concat([road_gdf, outer_gdf])
    major_road_gdf.set_crs(road_gdf.crs, inplace=True)
    sector_gdf = create_city_sector_gdf(major_road_gdf)
    sector_lvl = 0
    sector_gdf.reset_index(inplace=True,drop=True)
    sector_large_gdf = sector_gdf[sector_gdf['area'] > 10**7]
    sector_gdf.to_file(sector_path,layer=f'lvl_{sector_lvl}')
    while len(sector_large_gdf)>0:
        sector_lvl += 1
        sector_next_gdf = gpd.GeoDataFrame(columns=['geometry','name'])
        for i, row in sector_large_gdf.iterrows():
            row: gpd.GeoSeries
            road_in_sector = road_gdf[road_gdf.geometry.apply(lambda x: row.geometry.buffer(50).covers(x))]
            if len(road_in_sector) > 0:
                road_in_sector = extend_road_leaf(road_in_sector,2+sector_lvl,50)
                major_road_in_sector = road_in_sector[road_in_sector['highway_code'].isin(range(2+sector_lvl+1))]
                if(sector_lvl == 2 and i == 131):
                    major_road_in_sector.plot()
                    plt.show()
                sub_sector_gdf = create_city_sector_gdf(major_road_in_sector)
                sub_sector_gdf['parent_id'] = i
                if sub_sector_gdf is not None and len(sub_sector_gdf) > 0:
                    sector_next_gdf = pd.concat([sector_next_gdf, sub_sector_gdf])
        sector_next_gdf.set_crs(road_gdf.crs, inplace=True)
        sector_next_gdf['area'] = sector_next_gdf.geometry.area
        sector_next_gdf.reset_index(inplace=True,drop=True)
        if len(sector_next_gdf) >0:
            sector_next_gdf.to_file(sector_path,layer=f'lvl_{sector_lvl}')
        sector_large_gdf = sector_next_gdf[sector_next_gdf['area'] > 10**7]

def gen_street_block(input_path: pathlib.Path, output_path: pathlib.Path):
    road_gdf = gpd.read_file(input_path)
    road_gdf = road_gdf.to_crs(epsg=3857)

    merged_gdf = road_gdf.geometry.union_all()
    polygons:geom.MultiPolygon = list(ops.polygonize(merged_gdf))
    
    blocks = gpd.GeoDataFrame(crs=road_gdf.crs, geometry=polygons)
    blocks.reset_index(inplace=True, drop=True)
    blocks['block_id'] = blocks.index
    blocks.to_file(output_path, driver='GPKG')

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

def merge_small_street_block(input_path: pathlib.Path, output_path: pathlib.Path,
                             sector_path: pathlib.Path, min_width:float = 400):
    road_gdf = gpd.read_file(input_path)
    road_gdf = road_gdf.to_crs(epsg=3857)
    layers_list = fiona.listlayers(sector_path)
    city_sector_gdfs = [gpd.read_file(sector_path, layer=layer) for layer in layers_list]
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
            road_intersected = road_gdf.geometry.apply(lambda x: sector_polygon.buffer(10).covers(x))
            road_intersected_gdf = road_gdf[road_intersected]
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
    street_block_gdf.to_file(output_path, driver='GPKG')

def filter_road_by_street_block(input_path: pathlib.Path, output_path: pathlib.Path,
                                street_block_path: pathlib.Path):
    road_gdf = gpd.read_file(input_path)
    street_block_gdf = gpd.read_file(street_block_path)
    road_in_street_block: gpd.GeoDataFrame = road_gdf.sjoin(street_block_gdf, how='inner', predicate='within')
    road_out_street_block: gpd.GeoDataFrame = road_gdf[~road_gdf.index.isin(road_in_street_block.index)]
    road_out_street_block.reset_index(drop=True, inplace=True)
    road_out_street_block['road_id'] = road_out_street_block.index
    street_block_c_road = gpd.sjoin(street_block_gdf, road_out_street_block, how='inner', predicate='touches')
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
    street_block_gdf.to_file(output_path, driver='GPKG')

    road_out_street_block.to_file(output_path, driver='GPKG')

def add_landmark_surrounding_road(road_path: pathlib.Path, landmark_path: pathlib.Path, output_path: pathlib.Path):
    road_gdf = gpd.read_file(road_path)
    landmark_gdf = gpd.read_file(landmark_path)
    road_gdf = road_gdf.to_crs(epsg=3857)
    landmark_gdf = landmark_gdf.to_crs(epsg=3857)
    landmark_outer_gdf  = landmark_gdf.copy()
    landmark_outer_gdf.geometry = landmark_outer_gdf.geometry.apply(lambda x: geom.LineString(x.exterior))
    landmark_outer_splited_gdf = gpd.GeoDataFrame(columns=road_gdf.columns, crs=landmark_outer_gdf.crs)
    road_union = road_gdf.union_all()
    for i, row in landmark_outer_gdf.iterrows():
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
                    landmark_outer_splited_gdf.loc[len(landmark_outer_splited_gdf)] = row_dict.copy()
                    landmark_outer_splited_gdf.loc[len(landmark_outer_splited_gdf)-1, 'geometry'] = sub_row
    landmark_outer_splited_gdf.set_crs(landmark_outer_gdf.crs, inplace=True)
    masked_lines = gpd.overlay(road_gdf, landmark_gdf, how='difference')
    masked_lines = masked_lines.explode()
    masked_lines = masked_lines[masked_lines.geometry.length > 25]
    masked_lines.set_crs(road_gdf.crs, inplace=True)
    masked_lines.to_file(output_path)

def remove_degree_2_node(input_path: pathlib.Path, output_path: pathlib.Path):
    road_gdf = gpd.read_file(input_path)
    road_gdf = road_gdf.to_crs(epsg=3857)
    road_gdf = road_gdf[road_gdf.geometry.isna() == False]
    road_gdf = momepy.remove_false_nodes(road_gdf)
    road_network = momepy.gdf_to_nx(road_gdf, approach='primal', multigraph=True)
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
    road_gdf = momepy.nx_to_gdf(road_network, points=False, lines=True)
    road_gdf.reset_index(inplace=True,drop=True)
    road_gdf['road_id'] = road_gdf.index
    road_gdf.to_file(output_path)

def gen_street_block_landmark(road_path: pathlib.Path, landmark_path: pathlib.Path, output_path: pathlib.Path,):
    road_gdf = gpd.read_file(road_path)
    landmark_gdf = gpd.read_file(landmark_path)

    merged_gdf = road_gdf.geometry.union_all()
    polygons:geom.MultiPolygon = list(ops.polygonize(merged_gdf))
    
    blocks = gpd.GeoDataFrame(crs=road_gdf.crs, geometry=polygons)

    landmark_gdf.geometry = landmark_gdf.geometry.apply(lambda x: x.buffer(50))
    blocks_in_landmark = gpd.sjoin(blocks, landmark_gdf, how='inner', predicate='within')
    blocks_out_landmark = blocks[~blocks.index.isin(blocks_in_landmark.index)]
    blocks_out_landmark.reset_index(drop=True, inplace=True)
    blocks_out_landmark['block_id'] = blocks_out_landmark.index
    blocks_out_landmark.to_file(output_path)
