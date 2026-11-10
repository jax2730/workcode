from osgeo import ogr
from osgeo import gdal
from shapely.geometry import shape, mapping, MultiLineString, LineString, Point, GeometryCollection
from shapely.validation import explain_validity, make_valid
from shapely.ops import snap, split
from rtree import index
import geopandas as gpd
import pandas as pd
import json
import pathlib
import networkx as nx
import numpy as np

def snap_layer_to_self(layer_path, output_path, tolerance):
    """
    snap a layer to itself, with a given tolerance.
    """
    
    """# 1. 打开数据源
    input_ds = ogr.Open(layer_path, 0)
    if input_ds is None:
        print(f"无法打开输入文件: {layer_path}")
        return
    input_layer = input_ds.GetLayer(0)
    
    # 检查输出文件是否存在，如果存在则删除
    if os.path.exists(output_path):
        driver = ogr.GetDriverByName("GPKG")
        driver.DeleteDataSource(output_path)
    
    # 创建输出数据源
    driver = ogr.GetDriverByName("GPKG")
    output_ds = driver.CreateDataSource(output_path)
    if output_ds is None:
        print(f"无法创建输出文件: {output_path}")
        return
    
    # 创建输出图层
    output_layer = output_ds.CreateLayer(
        'snapped_self_output', 
        srs=input_layer.GetSpatialRef(), 
        geom_type=input_layer.GetGeomType()
    )
    
    # 复制属性字段
    input_layer_defn = input_layer.GetLayerDefn()
    for i in range(input_layer_defn.GetFieldCount()):
        field_defn = input_layer_defn.GetFieldDefn(i)
        output_layer.CreateField(field_defn)

    # 2. 预处理：加载所有几何图形并建立 Rtree 索引
    print("加载几何图形并建立空间索引...")
    features_data = {}
    r_index = index.Index()
    
    input_layer.ResetReading()
    for i, feature in enumerate(input_layer):
        ogr_geom = feature.GetGeometryRef()
        if ogr_geom:
            # 转换为 Shapely 几何图形
            shp_json = ogr.CreateGeometryFromWkt(ogr_geom.ExportToWkt()).ExportToJson()
            geom_shapely = shape(json.loads(shp_json))
            #geom_shapely = shape(ogr.CreateGeometryFromWkt(ogr_geom.ExportToWkt()).ExportToJson())
            
            # 存储 feature ID、Shapely 几何图形和原始 OGR feature
            features_data[i] = {
                'geom': geom_shapely, 
                'feature': feature.Clone()
            }
            
            # 插入到 Rtree：使用 ID 和要素的边界框
            r_index.insert(i, geom_shapely.bounds)

    # 3. 遍历并执行捕捉操作
    print(f"开始执行自吸附操作，容差: {tolerance}...")
    
    for current_id, data in features_data.items():
        input_geom_shapely = data['geom']
        original_ogr_feature = data['feature']

        # 1. 查询 Rtree，找出当前要素附近的所有邻居（边界框相交）
        # 我们将容差加到边界框上，以确保捕捉目标在搜索范围内。
        bounds_with_tolerance = (
            input_geom_shapely.bounds[0] - tolerance, 
            input_geom_shapely.bounds[1] - tolerance, 
            input_geom_shapely.bounds[2] + tolerance, 
            input_geom_shapely.bounds[3] + tolerance
        )
        neighbor_ids = list(r_index.intersection(bounds_with_tolerance))
        
        # 2. 收集所有邻居的几何图形 (作为捕捉目标)
        snap_targets = []
        for neighbor_id in neighbor_ids:
            # 忽略自身
            if neighbor_id != current_id:
                snap_targets.append(features_data[neighbor_id]['geom'])

        # 3. 将所有捕捉目标合并成一个统一的几何图形 (GeometryCollection)
        # 这样 snap 函数就可以一次性将输入要素捕捉到所有邻居上
        if snap_targets:
            from shapely.geometry import GeometryCollection
            target_collection = GeometryCollection(snap_targets)
            
            # 4. 执行 Shapely 捕捉操作
            snapped_geom_shapely = snap(input_geom_shapely, target_collection, tolerance)
        else:
            # 没有邻居，几何图形保持不变
            snapped_geom_shapely = input_geom_shapely
        
        # 5. 转换捕捉后的 Shapely 对象回 OGR 几何对象
        json_string = json.dumps(mapping(snapped_geom_shapely))
        snapped_geom_ogr = ogr.CreateGeometryFromJson(json_string)
        
        # 6. 写入新的要素
        out_feature = ogr.Feature(output_layer.GetLayerDefn())
        out_feature.SetGeometry(snapped_geom_ogr)
        
        # 复制属性（从原始 OGR Feature 复制）
        for i in range(input_layer_defn.GetFieldCount()):
            out_feature.SetField(i, original_ogr_feature.GetField(i))
        
        output_layer.CreateFeature(out_feature)
        
        # 清理
        out_feature = None

    print(f"自吸附完成！结果已保存到 {output_path}")
    
    # 7. 关闭数据源
    input_ds = None
    output_ds = None"""

    # Open the input layer
    gdf = gpd.read_file(layer_path)

    # project to a projection crs
    original_crs = gdf.crs
    gdf = gdf.to_crs(epsg=3857)

    # break multilines into separate lines
    features = []
    id_counter = 0
    #flat_gdf = gpd.GeoDataFrame()
    for i, row in gdf.iterrows():
        row:gpd.GeoSeries
        if row.geometry.geom_type == 'MultiLineString':
            for line in row.geometry.geoms:
                line:LineString
                new_feature = {
                    'id': id_counter,
                    'geometry': line,
                    'members': row['members'] if 'members' in row else None,
                    'original_id': row['id'] if 'id' in row else None,
                    'members_count':row['members_count'] if'members_count' in row else None,
                    'name:local': row['name:local'] if 'name:local' in row else None,
                    'name:en': row['name:en'] if 'name:en' in row else None,
                    'name:zh': row['name:zh'] if 'name:zh' in row else None,
                    'name': row['name'] if 'name' in row else None,
                    'highway': row['highway'] if 'highway' in row else None,
                    'geometry_length': line.length,
                }
                id_counter += 1
                features.append(new_feature)
        elif row.geometry.geom_type == 'LineString':
            new_feature = {
                'id': id_counter,
                'geometry':  row.geometry,
                'members': row['members'] if 'members' in row else None,
                'original_id': row['id'] if 'id' in row else None,
                'members_count':row['members_count'] if'members_count' in row else None,
                'name:local': row['name:local'] if 'name:local' in row else None,
                'name:en': row['name:en'] if 'name:en' in row else None,
                'name:zh': row['name:zh'] if 'name:zh' in row else None,
                'name': row['name'] if 'name' in row else None,
                'highway': row['highway'] if 'highway' in row else None,
                'geometry_length':  row.geometry.length,
            }
            id_counter += 1
            features.append(new_feature)
    
    flat_gdf = gpd.GeoDataFrame(features, crs=gdf.crs)
    # sort by length
    flat_gdf = flat_gdf.sort_values(by='geometry_length')
    #flat_gdf['original_index'] = flat_gdf.index

    rtree = index.Index()
    set_index = set()
    for i, row in flat_gdf.iterrows():
        # get the bounds of the geometry
        bounds = row.geometry.bounds
        search_bounds = (bounds[0] - tolerance, bounds[1] - tolerance, bounds[2] + tolerance, bounds[3] + tolerance)
        # find all the other geometries that intersect with the search bounds
        intersecting_ids = list(rtree.intersection(search_bounds))
        
        # add the current geometry to the list of snap targets
        snap_targets = flat_gdf.loc[intersecting_ids].copy()
        if not snap_targets.empty:
            # create a geometry collection of the snap targets
            target_collection = snap_targets.geometry.union_all()
            # snap the current geometry to the collection
            snapped_geom = snap(row.geometry, target_collection, tolerance)
            # update the geometry of the current feature
            row.geometry = snapped_geom
            flat_gdf.loc[i, 'geometry'] = snapped_geom

        bounds = row.geometry.bounds
        rtree.insert(i, bounds)

    flat_gdf = flat_gdf.drop(columns=['geometry_length'])
    flat_gdf = flat_gdf.dissolve(by = 'original_id',method='coverage', as_index=False)
    flat_gdf['id'] = flat_gdf['original_id']
    flat_gdf = flat_gdf.drop(columns=['original_id'])
    flat_gdf.to_crs(original_crs, inplace=True)
    flat_gdf.to_file(output_path)

def simplify_geom(geom:LineString, tolerance:float=1000)->LineString:
    max_distance = tolerance
    index = 0
    first_last_line = LineString([geom.coords[0], geom.coords[-1]])
    for i in range(1, len(geom.coords)-1):
        point = Point(geom.coords[i])
        distance = first_last_line.distance(point)
        if distance > max_distance:
            index = i
            max_distance = distance
    if max_distance > tolerance:
        left = simplify_geom(LineString(geom.coords[:index+1]),tolerance)
        right = simplify_geom(LineString(geom.coords[index:]),tolerance)
        simplified = LineString([*left.coords, *right.coords[1:]])
    else:
        if first_last_line.length < tolerance * 0.01:
            simplified = Point(first_last_line.coords[0])
        else:
            simplified = first_last_line
    return simplified

def simplify_geom_multipart(geom:MultiLineString|LineString, tolerance:float=1000)->MultiLineString|LineString:
    if geom.geom_type == 'MultiLineString':
        res = [simplify_geom(line, tolerance) for line in geom.geoms]
        res = [line for line in res if line.geom_type == 'LineString']
        simplified_geom = MultiLineString(res)
    else:
        simplified_geom = simplify_geom(geom, tolerance)
    return simplified_geom

def dP_simplification(layer_path:pathlib.Path, output_path, tolerance,layer = 0):
    """
    simplify lines or multilines to reduce the number of vertices by Douglas-Peucker algorithm.
    """
    
    # Open the input layer
    input_ds = ogr.Open(layer_path, 0)
    if input_ds is None:
        print(f"Cannot open input file: {layer_path}")
        return
    input_layer = input_ds.GetLayer(layer)
    # Create the output layer
    driver = ogr.GetDriverByName("GPKG")
    output_ds = driver.CreateDataSource(output_path)
    if output_ds is None:
        print(f"Cannot create output file: {output_path}")
        return
    output_layer = output_ds.CreateLayer(
        'dp_simplified_output', 
        srs=input_layer.GetSpatialRef(), 
        geom_type=input_layer.GetGeomType()
    )
    # Copy the attribute fields
    input_layer_defn = input_layer.GetLayerDefn()
    for i in range(input_layer_defn.GetFieldCount()):
        field_defn = input_layer_defn.GetFieldDefn(i)
        output_layer.CreateField(field_defn)
    # Simplify the geometry
    input_layer.ResetReading()
    for i, feature in enumerate(input_layer):
        ogr_geom = feature.GetGeometryRef()
        if ogr_geom:
            # Convert to Shapely geometry
            shp_json = ogr.CreateGeometryFromWkt(ogr_geom.ExportToWkt()).ExportToJson()
            geom_shapely = shape(json.loads(shp_json))
            #geom_shapely = shape(ogr.CreateGeometryFromWkt(ogr_geom.ExportToWkt()).ExportToJson())
            if geom_shapely.geom_type == 'LineString':
                geom_shapely:LineString
                simplified_geom = simplify_geom(geom_shapely, tolerance)
            elif geom_shapely.geom_type == 'MultiLineString':
                geom_shapely:MultiLineString
                res = [simplify_geom(line, tolerance) for line in geom_shapely.geoms]
                res = [line for line in res if line.geom_type == 'LineString']
                simplified_geom = MultiLineString(res)
                simplified_geom = make_valid(simplified_geom)
                if not simplified_geom.geom_type == 'MultiLineString':
                    lines = [line  for line in simplified_geom.geoms if line.is_valid and line.geom_type == 'LineString']
                    if len(lines) == 0:
                        continue
                    simplified_geom = MultiLineString(lines)
            else:
                simplified_geom = geom_shapely
            # Write the new feature
            if simplified_geom.geom_type == 'LineString' or simplified_geom.geom_type == 'MultiLineString':
                out_feature = ogr.Feature(output_layer.GetLayerDefn())
                out_feature.SetGeometry(ogr.CreateGeometryFromWkt(simplified_geom.wkt).MakeValid())
                
                for j in range(input_layer_defn.GetFieldCount()):
                    out_feature.SetField(j, feature.GetField(j))
                output_layer.CreateFeature(out_feature)
                # Clean up
                out_feature = None
    # Close the data sources
    input_ds = None
    output_ds = None

def extend_leaf_road(road_gdf:gpd.GeoDataFrame, road_level:int, min_leaf_length:float):
    road_decode_gdf = road_gdf.copy()
    major_road_gdf = road_gdf[road_decode_gdf['highway_code'].isin(range(road_level+1))]
    major_road_network = nx.Graph()
    for i, row in major_road_gdf.iterrows():
        row: gpd.GeoSeries
        length = row['geometry'].length
        id = i
        major_road_network.add_edge(row['geometry'].coords[0], row['geometry'].coords[-1], **{'id': id, 'length': length})
    leaf_nodes = [node for node, degree in major_road_network.degree() if degree == 1]
    leaf_node_keep = set()
    for node in leaf_nodes:
        connected_roads = road_gdf[road_gdf.geometry.intersects(Point(node))]
        if len(connected_roads) == 1:
            leaf_node_keep.add(node)
    leaf_nodes = [node for node in leaf_nodes if node not in leaf_node_keep]
    while len(leaf_nodes)>0:
        node = leaf_nodes.pop(0)
        edges = major_road_network.edges(node, data=True)
        if len(edges) == 0:
            continue
        edges = list(edges)
        s,e,data = edges[0]
        #find connected roads to the leaf node from road_gdf
        last_geom:LineString = road_gdf.loc[data['id']].geometry
        if node == last_geom.coords[0]:
            last_dir = np.array(last_geom.coords[0],dtype=np.float64) - np.array(last_geom.coords[1],dtype=np.float64)
        else:
            last_dir = np.array(last_geom.coords[-1],dtype=np.float64) - np.array(last_geom.coords[-2],dtype=np.float64)
        last_dir = last_dir/np.linalg.norm(last_dir)
        connected_roads = road_gdf[road_gdf.geometry.intersects(Point(node))]
        connected_roads = connected_roads[connected_roads.index != data['id']]
        connected_to_network = False
        max_dot_product = -1
        row_to_insert = None
        for j, connected_road in connected_roads.iterrows():
            connected_road: gpd.GeoSeries
            start = connected_road.geometry.coords[0]
            end = connected_road.geometry.coords[-1]
            if node == start:
                cur_dir = np.array(connected_road.geometry.coords[1],dtype=np.float64)-np.array(start,dtype=np.float64)
                new_leaf_node = end
            else:
                cur_dir = np.array(connected_road.geometry.coords[-2],dtype=np.float64)-np.array(end,dtype=np.float64)
                new_leaf_node = start
            cur_dir = cur_dir/np.linalg.norm(cur_dir)
            dot_product = np.dot(last_dir, cur_dir)
            #check if new_leaf_node is already in the graph
            if new_leaf_node in major_road_network.nodes:
                if connected_to_network:
                    if max_dot_product < dot_product:
                        max_dot_product = dot_product
                        row_to_insert = j
                else:
                    connected_to_network = True
                    max_dot_product = dot_product
                    row_to_insert = j
            else:
                if connected_to_network:
                    continue
                else:
                    if max_dot_product < dot_product:
                        max_dot_product = dot_product
                        row_to_insert = j
        if row_to_insert is not None:
            if data['length'] < min_leaf_length:
                new_row_geom = road_gdf.loc[data['id']].geometry
                new_row = road_gdf.loc[row_to_insert].to_dict()
                new_row['id'] = data['id']
                new_row['geometry'] = new_row_geom
                road_gdf.loc[data['id']] = new_row
                major_road_network.remove_edge(s,e)
            else:
                new_row_geom = road_gdf.loc[row_to_insert].geometry
                new_row:dict = road_gdf.loc[data['id']].to_dict()
                new_row['id'] = row_to_insert
                new_row['geometry'] = new_row_geom
                road_gdf.loc[row_to_insert] = new_row
                major_road_network.add_edge(new_row_geom.coords[0], new_row_geom.coords[-1], **{'id': row_to_insert, 'length': new_row_geom.length+data['length']})
        else:
            leaf_node_keep.add(node)
        leaf_nodes = [node for node, degree in major_road_network.degree() if degree == 1]
        leaf_nodes = [node for node in leaf_nodes if node not in leaf_node_keep]
    return road_gdf

if __name__ == '__main__':
    import pathlib
    import sys
    sys.path.append(pathlib.Path(__file__).parent.parent.absolute().as_posix())
    from constants import *
    # ROAD_DIR = DATA_DIR / 'road'

    # road_path = ROAD_DIR/'DE/roads_simplified.gpkg'
    # snapped_path = ROAD_DIR/'DE/roads_snapped.gpkg'
    # # road_path = ROAD_DIR/'test.geojson'
    # # snapped_path = ROAD_DIR/'test_snapped.geojson'
    # road_gdf = snap_layer_to_self(road_path, snapped_path, 1000)

    # road_path = ROAD_DIR/'DE/roads_filtered_proj.gpkg'
    # simplified_path = ROAD_DIR/'DE/roads_simplified.gpkg'
    # dP_simplification(road_path, simplified_path, 500)
    CITY_DIR = DATA_DIR / 'city'

    road_path = CITY_DIR/'Beijing'/'debug'/'sector_1_136.gpkg'

    road_gdf = gpd.read_file(road_path,layer='road')

    extend_leaf_road(road_gdf, 4, 1000)
    pass
