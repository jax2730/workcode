import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
import osgeo.ogr as ogr
import osgeo.osr as osr
import osgeo.gdal as gdal
import geopandas as gpd
import pandas as pd
import sknw
import numpy as np
import shapely.geometry as geom
import shapely.ops as ops
import json
import tarfile
import matplotlib.pyplot as plt
import networkx as nx
import convert_coords.convert_coords as convert_coords
import fiona
import create_sector_mesh
import momepy
import simplify_road
import argparse

from common.db import SessionLocal
from common.model import SimplyfyTask,LandmarkTask, StreatBlockTask

CITY_DIR = DATA_DIR / 'city'

MAP_OFFSET = [-2097152.0, -1048576.0]; 

NO_BUILDING_HIGHWAY_TYPES = ['motorway']

HIGHWAY_BUFFER = {
    'motorway': 80,
    'trunk': 50,
    'primary': 30,
    'secondary': 25,
    'tertiary': 20,
    'unclassified': 20,
    'residential': 25,

    'track': 20,
    'path': 20,
    None:20
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
    'count': 10
}

def road_intersection(road_gdf:gpd.GeoDataFrame):
    intersection = gpd.GeoDataFrame(columns=['geometry', 'id_list'])

    itered_set = set()

    def process_intersection(intersection_geom: geom.MultiLineString, id_list: tuple):
        i, j = id_list
        match (intersection_geom.geom_type):
            case 'Point':
                intersection.loc[len(intersection)] = [intersection_geom, f"{i}, {j}"]
            case 'LineString':
                intersection.loc[len(intersection)] = [geom.MultiPoint([intersection_geom.coords[0], intersection_geom.coords[-1]]), f"{i}, {j}"]
            case 'MultiPoint':
                intersection_geom: geom.MultiPoint
                for point in intersection_geom.geoms:
                    intersection.loc[len(intersection)] = [point, f"{i}, {j}"]
            case 'MultiLineString':
                intersection_geom: geom.MultiLineString
                intersection_geom = ops.unary_union(intersection_geom)
                intersection_geom = ops.linemerge(intersection_geom)
                if intersection_geom.geom_type == 'LineString':
                    intersection.loc[len(intersection)] = [geom.MultiPoint([intersection_geom.coords[0], intersection_geom.coords[-1]]), f"{i}, {j}"]
                else:
                    for line in intersection_geom.geoms:
                        intersection.loc[len(intersection)] = [line, f"{i}, {j}"]
            case 'GeometryCollection':
                intersection_geom: geom.GeometryCollection
                for geometry in intersection_geom.geoms:
                    process_intersection(geometry, id_list)
    if len(road_gdf) == 1:
        intersection.loc[len(intersection)] = [road_gdf.geometry.iloc[0], '0, 0']
    for i,row in road_gdf.iterrows():
        row: gpd.GeoSeries = row
        itered_set.add(i)
        for j,other_row in road_gdf.iterrows():
            other_row: gpd.GeoSeries = other_row
            if j not in itered_set:
                intersection_geom = row.geometry.intersection(other_row.geometry)
                if not intersection_geom.is_empty:
                    process_intersection(intersection_geom, (i,j))
    return intersection

def explode_multipolygon(gdf_input):
    gdf_single_poly = []
    
    for index, row in gdf_input.iterrows():
        row: gpd.GeoSeries
        geometry = row.geometry
        
        if geometry.geom_type == 'MultiPolygon':
            geometry: geom.MultiPolygon
            for single_geom in geometry.geoms:
                new_row = row.copy()
                new_row.geometry = single_geom
                gdf_single_poly.append(new_row)
        else:
            gdf_single_poly.append(row)
            
    gdf_exploded = gpd.GeoDataFrame(gdf_single_poly, crs=gdf_input.crs)
    
    return gdf_exploded

def explode_LineString(line: geom.LineString)->list[geom.LineString]:
    coords = list(line.coords)
    exploded_lines = []
    for i in range(1,len(coords)):
        exploded_lines.append(geom.LineString([coords[i-1],coords[i]]))
    return exploded_lines

def process_base_line(b_row: gpd.GeoSeries, line:geom.LineString, exterior_coords:list
                      ,wkt:str)->list[dict]:
    b_coords0 = line.coords[0]
    b_coords1 = line.coords[1]
    # determine if the base line point sequence is clockwise or counterclockwise
    b_coords0_index = exterior_coords.index(b_coords0)
    b_coords1_index = exterior_coords.index(b_coords1)
    # makesure the base line is in the same direction as the exterior line
    if (b_coords1_index - b_coords0_index) % len(exterior_coords) < len(exterior_coords) / 2:
        b_line = geom.LineString(line.coords)
    else:
        b_line = geom.LineString(line.coords[::-1])
    res_list = []
    for segment in explode_LineString(b_line):
        if segment.length > 5:
            seg = convert_coords.convert_linestring(segment, from_wkt=wkt, with_height=True)
            res_list.append({
                'id': b_row['road_id'],
                'pointsNum': seg.shape[0],
                'line': seg.tolist(),
                'name:local': ['base_line'],
                'highway_code' : -1,
            })
    return res_list

def match_road_2_street_block(road_path, street_block_path, output_path):
    road_gdf = gpd.read_file(road_path)
    # merged_gdf = road_gdf.geometry.union_all()
    # polygons:geom.MultiPolygon = list(ops.polygonize(merged_gdf))
    
    # blocks = gpd.GeoDataFrame(crs=road_gdf.crs, geometry=polygons)

    # blocks.to_file(street_block_path, driver='GPKG')
    street_block_gdf = gpd.read_file(street_block_path)
    road_gdf = road_gdf.to_crs(epsg=3857)
    street_block_gdf = street_block_gdf.to_crs(epsg=3857)
    street_block_gdf.reset_index(inplace=True,names=['street_block_id'])
    street_block_boundary_gdf = street_block_gdf.copy()
    street_block_boundary_gdf['geometry'] = street_block_boundary_gdf.geometry.apply(lambda x: x.boundary)
    road_gdf.geometry = road_gdf.geometry.snap(street_block_gdf.geometry.union_all(), 5)
    join_gdf = street_block_gdf.sjoin(road_gdf, 
                                               how='inner', 
                                               predicate='intersects',
                                               lsuffix='block',
                                               rsuffix='road')
    #street_block_with_road = gpd.GeoDataFrame()
    street_block_bdg_gdf = gpd.GeoDataFrame(columns=['street_block_id','road_id', 'intersection_type','geometry'])

    for id,group in join_gdf.groupby('street_block_id'):
        road_ids = []
        road_names = []
        intersection_type = []
        block_geometry = group['geometry'].iloc[0]
        geoms = []
        for i,row in group.iterrows():
            road_id = row['index_road']
            road_geometry = road_gdf.loc[road_id,'geometry']
            road_name = road_gdf.loc[road_id,'name:local']
            road_geometry = ops.snap(road_geometry,block_geometry,5)
            intersection_geometry = block_geometry.intersection(road_geometry)
            if intersection_geometry.geom_type == 'LineString' or intersection_geometry.geom_type == 'MultiLineString':
                road_ids.append(road_id)
                road_names.append(road_name)
                geoms.append(road_geometry)
                intersection_type.append(intersection_geometry.geom_type)
            # if intersection_geometry.geom_type == 'MultiLineString':
            #     merged_line = ops.linemerge(intersection_geometry)
            #     road_ids.append(road_id)
            #     road_names.append(road_name)
            #     geoms.append(merged_line)
        street_block_bdg_gdf_temp = gpd.GeoDataFrame(columns=['street_block_id','road_id', 'intersection_type','geometry'],geometry=geoms)
        street_block_bdg_gdf_temp['street_block_id'] = id
        street_block_bdg_gdf_temp['road_id'] = road_ids
        street_block_bdg_gdf_temp['intersection_type'] = intersection_type
        street_block_bdg_gdf_temp.set_crs(road_gdf.crs, inplace=True)
        street_block_bdg_gdf = pd.concat([street_block_bdg_gdf,street_block_bdg_gdf_temp])
        street_block_bdg_gdf.set_crs(road_gdf.crs, inplace=True)
        multi_line = ops.unary_union(geoms)
        street_block_gdf.loc[id,'geometry'] = multi_line
        street_block_gdf.loc[id,'road_names'] = json.dumps(road_names,ensure_ascii=False)
        street_block_gdf.loc[id,'road_ids'] = json.dumps(road_ids,ensure_ascii=False)
        street_block_gdf.loc[id,'intersection_type'] = json.dumps(intersection_type,ensure_ascii=False)
        pass
    pass

    street_block_gdf.to_file(output_path)
    street_block_bdg_gdf.to_file(CITY_DIR/'Test'/'street_block_dbg.gpkg')

def create_building_base_line(road_path, street_block_path, output_path):
    road_gdf = gpd.read_file(road_path)
    #road_gdf = road_gdf[road_gdf.geometry.notna()]
    street_block_gdf = gpd.read_file(street_block_path)
    street_block_gdf = street_block_gdf.to_crs(epsg=3857)

    street_block_building_gdf = street_block_gdf.copy()
    buliding_base_line_gdf: gpd.GeoDataFrame = gpd.GeoDataFrame(columns=['street_block_id','road_id','geometry'])
    buffered_road_gdf = gpd.GeoDataFrame(columns=['geometry','highway'])
    for i, row in street_block_gdf.iterrows():
        road_ids = json.loads(row['road_ids'])
        row_polygon_list = [*ops.polygonize(row.geometry)]
        if len(row_polygon_list) == 0:
            continue
        largest_polygon = max(row_polygon_list, key=lambda x: x.area)
        row_polygon = largest_polygon
        sb_roads_gdf = road_gdf.loc[road_ids]
        sb_roads_gdf = sb_roads_gdf[sb_roads_gdf.geometry.notna()]
        sb_roads_gdf = sb_roads_gdf.dissolve(by =['highway','name:local'],as_index=False)
        sb_roads_gdf['highway'] = sb_roads_gdf['highway'].apply(lambda x: json.loads(x))
        sb_roads_gdf['name:local'] = sb_roads_gdf['name:local'].apply(lambda x: json.loads(x))
        sb_roads_gdf['name:en'] = sb_roads_gdf['name:en'].apply(lambda x: json.loads(x))
        sb_roads_gdf['name:zh'] = sb_roads_gdf['name:zh'].apply(lambda x: json.loads(x))
        if len(sb_roads_gdf) == 0:
            continue
        sb_roads_buffer_gdf = sb_roads_gdf.copy()
        # sb_roads_highway_buffer_gdf: gpd.GeoDataFrame = sb_roads_gdf.copy()
        # sb_roads_highway_buffer_gdf = sb_roads_highway_buffer_gdf.dissolve(by='highway',as_index=False)
        for j, road_row in sb_roads_buffer_gdf.iterrows():
            highway = road_row['highway'][0]
            road_row.geometry = road_row.geometry.buffer(HIGHWAY_BUFFER[highway])
            sb_roads_buffer_gdf.loc[j,'geometry'] = road_row.geometry
        # if i == 917:
        #     sb_roads_gdf.plot()
        #     sb_roads_buffer_gdf.plot()
        #     plt.show()
        dissolved = sb_roads_buffer_gdf.dissolve(as_index=False)
        sb_roads_buffer_gdf = explode_multipolygon(sb_roads_buffer_gdf)
        if dissolved.geometry.tolist()[0].geom_type == 'Polygon' and dissolved.geometry.tolist()[0].interiors:
            interior = dissolved.geometry.tolist()[0].interiors[0]
            new_row = row.copy()
            line_geom = geom.LineString(interior.coords)
            new_row['geometry'] = line_geom
            street_block_building_gdf.loc[i] = new_row
            sb_base_line_gdf = gpd.GeoDataFrame(columns=['street_block_id','road_id','geometry'])
            for j, road_row in sb_roads_buffer_gdf.iterrows():
                road_geom = geom.LineString(road_row.geometry.exterior)
                road_geom = ops.snap(road_geom,row_polygon,5)
                intersection_geometry = row_polygon.intersection(road_geom)
                if intersection_geometry.is_empty and len(road_row.geometry.interiors)>0:
                    road_geom = geom.LineString(road_row.geometry.interiors[0])
                    road_geom = ops.snap(road_geom,row_polygon,5)
                    intersection_geometry = row_polygon.intersection(road_geom)
                # plt.plot(*road_geom.xy,color='red')
                # plt.plot(*row_polygon.exterior.xy,color='blue')
                # plt.show()
                if intersection_geometry.geom_type == 'GeometryCollection':
                    multilines = [ elem for elem in intersection_geometry.geoms if elem.geom_type == 'LineString' ]
                    intersection_geometry = ops.linemerge(multilines) if len(multilines) > 1 else multilines[0]
                intersection_geometry = ops.linemerge(intersection_geometry) if intersection_geometry.geom_type == 'MultiLineString' else intersection_geometry
                sb_base_line_gdf.loc[len(sb_base_line_gdf)]=\
                    {
                        'street_block_id': row['street_block_id'],
                        'road_id': road_row['road_id'],
                        'geometry': intersection_geometry
                    }
            # sb_base_line_gdf.plot()
            # plt.show()
            if len(sb_base_line_gdf) > 1:
                intersection_geometry = road_intersection(sb_base_line_gdf).geometry
                if intersection_geometry.empty:
                    continue
                intersections = intersection_geometry.union_all() if len(intersection_geometry) > 1 else intersection_geometry[0]
                G = nx.Graph()
                for j, inter_row in sb_base_line_gdf.iterrows():
                    geometry = inter_row['geometry']
                    geometry = ops.snap(geometry,intersections,tolerance=1)
                    if geometry.intersects(intersections):
                        lines_geom = ops.split(geometry,intersections)
                        lines_geom = [*lines_geom.geoms]
                        for line_geom in lines_geom:
                            if line_geom.length > 1000:
                                #slipt line in two parts
                                line_geom1 = ops.substring(line_geom,0,line_geom.length/2)
                                line_geom2 = ops.substring(line_geom,line_geom.length/2,line_geom.length)
                                lines_geom.remove(line_geom)
                                lines_geom.append(line_geom1)
                                lines_geom.append(line_geom2)
                        for line_geom in lines_geom:
                                G.add_edge(line_geom.coords[0],line_geom.coords[-1], geometry=line_geom, length=line_geom.length,weight = j)
                
                G = nx.Graph(G)
                basis_cycles = list(nx.cycle_basis(G))
                if len(basis_cycles) == 0:
                    print (f"Street Block ID: {row['street_block_id']}, have no circle basis, skipped." )
                    if len(sb_roads_buffer_gdf.union_all().interiors)>0:
                        largest_interior = sb_roads_buffer_gdf.union_all().interiors[0]
                        line_geom = geom.LineString(largest_interior.coords)
                        line_geom = ops.snap(line_geom,row_polygon,5)
                        street_block_geometry = ops.unary_union(ops.polygonize(line_geom))
                        sb_base_line_gdf_alt = gpd.GeoDataFrame(columns=['street_block_id','road_id','geometry'])
                        sb_base_line_gdf_alt.loc[len(sb_base_line_gdf)]=\
                            {
                                'street_block_id': row['street_block_id'],
                                'road_id': sb_base_line_gdf.iloc[0]['road_id'],
                                'geometry': line_geom
                            }
                        buliding_base_line_gdf = pd.concat([buliding_base_line_gdf,sb_base_line_gdf_alt])
                        street_block_building_gdf.loc[i,'geometry'] = street_block_geometry
                    continue

                largest_cycle = max(basis_cycles, key=len)
                not_in_cycles = [node for node in G.nodes if node not in largest_cycle]
                #remove not in cycles nodes
                for node in not_in_cycles:
                    G.remove_node(node)
                
                sb_base_line_gdf_alt = gpd.GeoDataFrame(columns=['street_block_id','road_id','geometry'])
                for edge in G.edges(data=True):
                    s,e,data = edge
                    line_geom = data['geometry']
                    id = data['weight']
                    base_line_dict = sb_base_line_gdf.loc[id].to_dict()
                    base_line_dict['geometry'] = line_geom
                    sb_base_line_gdf_alt.loc[len(sb_base_line_gdf_alt)]=base_line_dict
                sb_base_line_gdf_alt = sb_base_line_gdf_alt.dissolve(by='road_id',as_index=False) if len(sb_base_line_gdf_alt) > 1 else sb_base_line_gdf_alt
                
                buliding_base_line_gdf = pd.concat([buliding_base_line_gdf,sb_base_line_gdf_alt])
                polygon_list = list(ops.polygonize(sb_base_line_gdf_alt.geometry))
                polygon = polygon_list[0] if len(polygon_list) > 0 else ops.unary_union(polygon_list)
                street_block_building_gdf.loc[i,'geometry'] = polygon
            else:
                buliding_base_line_gdf = pd.concat([buliding_base_line_gdf,sb_base_line_gdf])
                polygon_list = list(ops.polygonize(sb_base_line_gdf.geometry))
                polygon = polygon_list[0] if len(polygon_list) > 0 else ops.unary_union(polygon_list)
                street_block_building_gdf.loc[i,'geometry'] = polygon
        else:
            street_block_building_gdf.drop(index=i,inplace=True)
        
        sb_roads_buffer_gdf.geometry = sb_roads_buffer_gdf.geometry.apply(lambda x: geom.LineString(x.exterior))
        buffered_road_gdf = pd.concat([buffered_road_gdf,sb_roads_buffer_gdf])
    buliding_base_line_gdf = buliding_base_line_gdf.dissolve(by=['street_block_id','road_id'],as_index=False)
    buliding_base_line_gdf['name:local'] = buliding_base_line_gdf['road_id'].apply(lambda x: road_gdf.loc[x,'name:local'])
    buliding_base_line_gdf['highway'] = buliding_base_line_gdf['road_id'].apply(lambda x: road_gdf.loc[x,'highway'])
    buliding_base_line_gdf['name:en'] = buliding_base_line_gdf['road_id'].apply(lambda x: road_gdf.loc[x,'name:en'])
    buliding_base_line_gdf['name:zh'] = buliding_base_line_gdf['road_id'].apply(lambda x: road_gdf.loc[x,'name:zh'])
    buliding_base_line_gdf['allow_building'] = buliding_base_line_gdf['highway'].apply(lambda x: json.loads(x)[0] not in NO_BUILDING_HIGHWAY_TYPES)
    street_block_gdf.set_crs(road_gdf.crs, inplace=True)
    street_block_building_gdf.set_crs(road_gdf.crs, inplace=True)
    buliding_base_line_gdf.set_crs(road_gdf.crs, inplace=True)
    buliding_base_line_gdf['highway'] = buliding_base_line_gdf['highway'].apply(lambda x: json.dumps(x,ensure_ascii=False))
    buliding_base_line_gdf['name:local'] = buliding_base_line_gdf['name:local'].apply(lambda x: json.dumps(x,ensure_ascii=False))
    buliding_base_line_gdf['name:en'] = buliding_base_line_gdf['name:en'].apply(lambda x: json.dumps(x,ensure_ascii=False))
    buliding_base_line_gdf['name:zh'] = buliding_base_line_gdf['name:zh'].apply(lambda x: json.dumps(x,ensure_ascii=False))
    street_block_building_gdf.to_file(output_path,layer='street_block')
    buliding_base_line_gdf.to_file(output_path,layer='building_base_line')
    buffered_road_gdf.to_file(output_path,layer='buffered_road')




def street_block_to_json_by_sector(city_name:str):
    city_dir = CITY_DIR / city_name
    landmark_path = city_dir / 'landmark_polygons.gpkg'
    landmark_gdf = gpd.read_file(landmark_path)
    landmark_info_path = city_dir / 'landmark_info.json'
    landmark_info = json.load(open(landmark_info_path))
    road_path = city_dir / 'city_road_w_landmark_degree2.gpkg'
    street_block_path = city_dir / 'city_building_base_line.gpkg'
    street_block_area_path = city_dir / 'city_block_out_landmark.gpkg'
    street_block_json_path = city_dir / 'city_street_block.json'
    city_sector_path = city_dir / 'city_sector.gpkg'
    # sector_mesh_dir = city_dir /'sector_mesh'
    # if sector_mesh_dir.exists():
    #     for f in sector_mesh_dir.iterdir():
    #         f.unlink()
    #     sector_mesh_dir.rmdir()
    # sector_mesh_dir.mkdir()

    road_gdf = gpd.read_file(road_path)
    street_block_gdf = gpd.read_file(street_block_path,layer='street_block')
    street_block_area_gdf = gpd.read_file(street_block_area_path)
    # street_block_gdf.drop(columns=['street_block_id'],inplace=True)
    street_block_gdf = street_block_gdf[street_block_gdf.geometry.geom_type == 'Polygon']
    street_block_gdf['old_id'] = street_block_gdf['street_block_id']
    street_block_gdf['street_block_id'] = np.arange(len(street_block_gdf))
    buliding_base_line_gdf = gpd.read_file(street_block_path,layer='building_base_line')
    layers_list = fiona.listlayers(city_sector_path)
    city_sector_gdfs = [gpd.read_file(city_sector_path, layer=layer) for layer in layers_list]

    world_terrian = DATA_DIR /'world_terrain/worldmap_fill.tif'


    # terrian_ds = gdal.Open(str(world_terrian))
    # if terrian_ds is None:
    #     print("Could not open %s" % world_terrian)
    #     return
    # x_min, x_res, x_skew, y_max, y_skew, y_res = terrian_ds.GetGeoTransform()
    # terrian_crs = osr.SpatialReference()
    # terrian_crs.ImportFromWkt(terrian_ds.GetProjectionRef())
    crs_wkt = road_gdf.crs.to_wkt()

    road_gdf.to_crs(crs_wkt, inplace=True)
    road_bound = road_gdf.total_bounds
    street_block_gdf.to_crs(crs_wkt, inplace=True)
    buliding_base_line_gdf.to_crs(crs_wkt, inplace=True)
    sector_groupeds = []
    
    for lvl in range(1,len(city_sector_gdfs)):
        city_sector_gdf = city_sector_gdfs[lvl]
        for i in range(lvl):
            upper_sector = city_sector_gdfs[i]
            upper_sector.set_crs(city_sector_gdf.crs, inplace=True)
            joined = city_sector_gdf.sjoin(upper_sector, how='inner', predicate='within')
            city_sector_gdf.loc[joined.index, f'parent_id_{i}'] = joined['index_right']
        sector_grouped = city_sector_gdf.groupby([f'parent_id_{i}' for i in range(lvl)])
        sector_groupeds.append(sector_grouped)
    city_sector_gdf_top = city_sector_gdfs[0]
    city_sector_gdf_top['parent_id'] = 0
    sector_grouped = city_sector_gdf_top.groupby('parent_id')
    sector_groupeds.insert(0, sector_grouped)
    road_groupeds = []
    main_road_gdf = road_gdf.copy()
    for lvl in range(len(city_sector_gdfs)-1,-1,-1):
        city_sector_gdf = city_sector_gdfs[lvl]
        joined = main_road_gdf.sjoin(city_sector_gdf, how='inner', predicate='within')
        road_grouped = joined.groupby(by='index_right')
        road_groupeds.insert(0, road_grouped)
        main_road_gdf = main_road_gdf[~main_road_gdf.index.isin(joined.index)]
    street_block_groupeds = []
    street_block_gdf_alt = street_block_gdf.copy()

    for lvl in range(len(city_sector_gdfs)-1,-1,-1):
        city_sector_gdf = city_sector_gdfs[lvl]
        joined = street_block_gdf_alt.sjoin(city_sector_gdf, how='inner', predicate='within')
        street_block_grouped = joined.groupby(by='index_right')
        street_block_groupeds.insert(0, street_block_grouped)
        street_block_gdf_alt = street_block_gdf_alt[~street_block_gdf_alt.index.isin(joined.index)]
    street_block_area_groupeds = []
    for lvl in range(len(city_sector_gdfs)-1,-1,-1):
        city_sector_gdf = city_sector_gdfs[lvl]
        street_block_area_gdf_cpy = street_block_area_gdf.copy()
        street_block_area_gdf_cpy.geometry = street_block_area_gdf_cpy.geometry.representative_point()
        joined_points = gpd.sjoin(street_block_area_gdf_cpy, city_sector_gdf, how='inner', predicate='covered_by')
        joined = street_block_area_gdf.merge(
            joined_points.drop(columns='geometry'),
            left_index=True,
            right_index=True
        )
        joined_points.to_file(city_dir/f'city_street_block_area_points.gpkg',layer=f'street_block_area_points_lvl{lvl}')
        joined.to_file(city_dir / f'city_street_block_area_.gpkg',layer=f'street_block_area_lvl{lvl}')
        street_block_area_grouped = joined.groupby(by='index_right')
        street_block_area_groupeds.insert(0, street_block_area_grouped)

    convert_coords.load_base_gtiff(world_terrian)
    convert_coords.load_x_y_range_data((road_bound[0],road_bound[2]), (road_bound[1],road_bound[3]),from_wkt=crs_wkt)
    convert_coords.set_origin_point(MAP_OFFSET[0],MAP_OFFSET[1])
    convert_coords.set_pixel_scale(32)
    
    city_main_road_list = []
    for i, row in main_road_gdf.iterrows():
        row: gpd.GeoDataFrame
        if row.geometry is not None and row.geometry.geom_type == 'LineString':
            line = convert_coords.convert_linestring(row.geometry,with_height=True,from_wkt=crs_wkt)
            highway_list = ['none' if x is None else x for x in json.loads(row['highway'])]
            highway_codes = [HIGHWAY_CODE[x] for x in highway_list]
            city_main_road_list.append({
                'id': i,
                'pointsNum': line.shape,
                'line': line.tolist(),
                'highway': highway_list,
                'highway_code': highway_codes,
                'name:local': json.loads(row['name:local']),
                'name:en': json.loads(row['name:en']),
                'name:zh': json.loads(row['name:zh'])
            })

    city_sector_list = []

    for lvl, sector_grouped in enumerate(sector_groupeds):
        road_group = road_groupeds[lvl]
        street_block_group = street_block_groupeds[lvl]
        for i in sector_grouped.groups:
            if lvl == 0:
                sub_sector_list = city_sector_list
            elif lvl ==1:
                for sub_sector in city_sector_list:
                    if sub_sector['id'] == i:
                        sub_sector_list = sub_sector['sub_sectors']
            else:
                i:tuple
                sub_sector_list= city_sector_list
                for index in i:
                    for sub_sector in sub_sector_list:
                        if sub_sector['id'] == index:
                            sub_sector_list = sub_sector['sub_sectors']
            contained_sector = sector_grouped.get_group(i,)
            for j, sector_row in contained_sector.iterrows():
                road_list = []
                sector_vb= np.ndarray((0,3))
                sector_ib= np.ndarray((0,3))
                if j in road_group.groups:
                    contained_road = road_group.get_group(j,)
                    for k, road_row in contained_road.iterrows():
                        road_row: gpd.GeoDataFrame
                        if road_row.geometry.geom_type == 'LineString':
                            line = convert_coords.convert_linestring(road_row.geometry,with_height=True,from_wkt=crs_wkt)
                            highway_list = ['none' if x is None else x for x in json.loads(road_row['highway'])]
                            highway_codes = [HIGHWAY_CODE[x] for x in highway_list]
                            road_list.append({
                                'id': k,
                                'pointsNum': line.shape[0],
                                'line': line.tolist(),
                                'highway': highway_list,
                                'highway_code': highway_codes,
                                'name:local': json.loads(road_row['name:local']),
                                'name:en': json.loads(road_row['name:en']),
                                'name:zh': json.loads(road_row['name:zh'])
                            })
                if j in street_block_group.groups:
                    contained_blocks = street_block_group.get_group(j,)
                    contained_blocks_list = contained_blocks['street_block_id'].tolist()
                else:
                    contained_blocks_list = []
                # if j in street_block_area_groupeds[lvl].groups:
                #     contained_blocks = street_block_area_groupeds[lvl].get_group(j)
                #     for _,block_row in contained_blocks.iterrows():
                #         block_row: gpd.GeoSeries
                #         block_geom: geom.Polygon = block_row.geometry
                #         vb,ib = create_sector_mesh.polygon3d_2_triangles(block_geom,crs_wkt,False)
                #         sector_vb,sector_ib = create_sector_mesh.appen_vbib(sector_vb,sector_ib,vb,ib)
                center = sector_row.geometry.centroid
                radius = max([center.distance(geom.Point(x)) for x in sector_row.geometry.exterior.coords])
                center_p = convert_coords.convert_coords_with_height(np.array(*center.coords),from_wkt=crs_wkt)
                
                sector_dict = {
                'id': j,
                'name': f'sector_{lvl}_{j+1}',
                'center': center_p.tolist(),
                'radius': radius/32,
                'contained_roads': road_list,
                'contained_blocks': contained_blocks_list,
                'sub_sectors':[]
                }

                # create sector mesh
                # sector_vb -= np.array([center_p[0],0,center_p[2]]).reshape(1,3)
                # create_sector_mesh.save_mesh(sector_vb,sector_ib,sector_mesh_dir/f'sector_{lvl}_{j+1}.obj')
                # create_sector_mesh.obj2fbx(sector_mesh_dir/f'sector_{lvl}_{j+1}.obj')

                sub_sector_list.append(sector_dict)
    
    street_block_list = []
    for i, row in street_block_gdf.iterrows():
        row: gpd.GeoSeries
        # if not row.geometry.geom_type == 'Polygon':
        #     continue
        exterior = geom.LineString(row.geometry.exterior.coords)
        coords_list = [*exterior.coords]
        center = convert_coords.convert_coords_with_height(coord=np.array(*row.geometry.centroid.coords),from_wkt=crs_wkt)
        line = convert_coords.convert_linestring(exterior,with_height=True,from_wkt=crs_wkt)
        building_base_lines = buliding_base_line_gdf[buliding_base_line_gdf['street_block_id']==row['old_id']]
        building_base_lines_list=[]
        for j, b_row in building_base_lines.iterrows():
            b_row: gpd.GeoDataFrame
            if b_row['allow_building']:
                if b_row.geometry.geom_type == 'LineString':
                    building_base_lines_list.extend(process_base_line(b_row, b_row.geometry, coords_list,crs_wkt))
                elif b_row.geometry.geom_type == 'MultiLineString':
                    for sub_line in b_row.geometry.geoms:
                        building_base_lines_list.extend(process_base_line(b_row, sub_line, coords_list,crs_wkt))
        street_block_list.append({
            'id': row['street_block_id'],
            'center': center.tolist(),
            'pointsNum': line.shape[0],
            'exterior': line.tolist(),
            'base_lines': building_base_lines_list
        })
    
    landmark_list=[]
    landmark_gdf = landmark_gdf[landmark_gdf.geometry.notna()]
    for i, row in landmark_gdf.iterrows():
        landmark = {}
        landmark['actor_index'] = i
        landmark['name'] = row['name:local'] if row['name:local'] else row['name:en'] if row['name:en'] else row['name:zh']
        point0 = np.array(row.geometry.exterior.coords[0])
        point1 = np.array(row.geometry.exterior.coords[1])
        midPoint = point0 + (point1 - point0)/2
        pos = convert_coords.convert_coords(coord=midPoint,from_wkt=crs_wkt)
        landmark['pos'] = pos.tolist()
        landmark['dir'] = [1,0]
        landmark_list.append(landmark)

    with open(street_block_json_path, 'w') as f:
        city_street_block_data={
            'version' : 3,
            'cityName': city_name,
            'landmarkInfos': landmark_info,
            'landmarks':landmark_list,
            'mainLanes': city_main_road_list,
            'sectors': city_sector_list,
            'streetBlocks': street_block_list
        } 
        json.dump(city_street_block_data, f, ensure_ascii=False, indent=4)

    # with tarfile.open(sector_mesh_dir.parent / f'{city_name}_sector_mesh.tar.gz', "w:gz") as tar:
    #     for sector_mesh_file in sector_mesh_dir.glob('*.fbx'):
    #         tar.add(sector_mesh_file, arcname=sector_mesh_file.name)
    
def street_block_to_json(city_name:str):
    city_dir = CITY_DIR / city_name
    road_path = city_dir / 'city_road_out_block.gpkg'
    street_block_path = city_dir / 'city_building_base_line.gpkg'
    road_json_path = city_dir / 'city_road.json'
    street_block_json_path = city_dir / 'city_street_block.json'
    city_sector_path = city_dir / 'city_sector.gpkg'


    road_gdf = gpd.read_file(road_path)
    street_block_gdf = gpd.read_file(street_block_path,layer='street_block')
    buliding_base_line_gdf = gpd.read_file(street_block_path,layer='building_base_line')
    city_sector_gdf = gpd.read_file(city_sector_path)
    road_in_sector_gdf = road_gdf.sjoin(city_sector_gdf, how='inner', predicate='within')
    city_main_road_gdf = road_gdf[~road_gdf.index.isin(road_in_sector_gdf.index)]

    world_terrian = DATA_DIR /'world_terrain/worldmap_fill_001_001.tif'
    convert_coords.load_base_gtiff(world_terrian)
    convert_coords.set_origin_point(MAP_OFFSET[0], MAP_OFFSET[1])
    convert_coords.set_pixel_scale(32)

    terrian_ds = gdal.Open(str(world_terrian))
    if terrian_ds is None:
        print("Could not open %s" % world_terrian)
        return
    # x_min, x_res, x_skew, y_max, y_skew, y_res = terrian_ds.GetGeoTransform()
    # terrian_crs = osr.SpatialReference()
    # terrian_crs.ImportFromWkt(terrian_ds.GetProjectionRef())
    # crs_wkt = terrian_crs.ExportToWkt()
    # road_gdf.to_crs(crs_wkt, inplace=True)
    road_gdf = road_gdf[road_gdf.geometry.notna()]
    # street_block_gdf.to_crs(crs_wkt, inplace=True)
    # buliding_base_line_gdf.to_crs(crs_wkt, inplace=True)
    street_block_list = []
    for i, row in street_block_gdf.iterrows():
        row: gpd.GeoDataFrame
        if not row.geometry.geom_type == 'Polygon':
            continue
        exterior = geom.LineString(row.geometry.exterior.coords)
        coords_list = [*exterior.coords]
        centroid = geom.Point(row.geometry.centroid)
        center = convert_coords.convert_coords_with_height(coord=centroid.coords[0], with_height=True,from_wkt=road_gdf.crs.to_wkt())
        line = convert_coords.convert_linestring(exterior, with_height= True,from_wkt=road_gdf.crs.to_wkt())
        building_base_lines = buliding_base_line_gdf[buliding_base_line_gdf['street_block_id']==row['street_block_id']]
        building_base_lines_list=[]
        for j, b_row in building_base_lines.iterrows():
            b_row: gpd.GeoDataFrame
            if b_row['allow_building']:
                if b_row.geometry.geom_type == 'LineString':
                    building_base_lines_list.extend(process_base_line(b_row, b_row.geometry, coords_list,road_gdf.crs.to_wkt()))
                elif b_row.geometry.geom_type == 'MultiLineString':
                    for sub_line in b_row.geometry.geoms:
                        building_base_lines_list.extend(process_base_line(b_row, sub_line, coords_list,road_gdf.crs.to_wkt()))
        street_block_list.append({
            'id': row['street_block_id'],
            'center': center.tolist(),
            'pointsNum': line.shape[0],
            'exterior': line.tolist(),
            'base_lines': building_base_lines_list
        })
    with open(street_block_json_path, 'w') as f:
        json.dump(street_block_list, f, ensure_ascii=False, indent=4)
    
    road_list = []
    for i, row in road_gdf.iterrows():
        row: gpd.GeoDataFrame
        if row.geometry.geom_type == 'LineString':
            line = convert_coords.convert_linestring(row.geometry, with_height= True,from_wkt=road_gdf.crs.to_wkt())
            highway_list = ['none' if x is None else x for x in json.loads(row['highway'])]
            highway_codes = [HIGHWAY_CODE[x] for x in highway_list]
            road_list.append({
                'id': i,
                'pointsNum': line.shape[0],
                'line': line.tolist(),
                'highway': highway_list,
                'highway_code': highway_codes,
                'name:local': json.loads(row['name:local']),
                'name:en': json.loads(row['name:en']),
                'name:zh': json.loads(row['name:zh'])
            })
    with open(road_json_path, 'w') as f:
        json.dump(road_list, f, ensure_ascii=False, indent=4)

def process_street_block(city_name:str):
    city_dir = CITY_DIR / city_name
    road_path = city_dir / 'city_road_w_landmark_degree2.gpkg'
    street_block_path = city_dir / 'city_block_out_landmark.gpkg'
    street_block_with_road_path = city_dir / 'city_road_filled_matched.gpkg'
    buliding_base_line_path = city_dir / 'city_building_base_line.gpkg'

    simplify_road.gen_street_block_landmark(city_name)
    print(f"{city_name} step 1: generate street block and landmark successfully")
    match_road_2_street_block(road_path, street_block_path, street_block_with_road_path)
    print(f"{city_name} step 2: match road to street block successfully")
    create_building_base_line(road_path, street_block_with_road_path, buliding_base_line_path)
    print(f"{city_name} step 3: create building base line successfully")
    street_block_to_json_by_sector(city_name)
    print(f"{city_name} step 4: create street block json by sector successfully")

def update_or_create_street_block_task(city:str, result_path:pathlib.Path):
    session = SessionLocal()

    task = session.query(StreatBlockTask).filter(
        StreatBlockTask.city == city,
    ).first()

    if task is None:
        task = StreatBlockTask(
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

def update_or_create_street_block_task_failed(city:str):
    session = SessionLocal()

    task = session.query(StreatBlockTask).filter(
        StreatBlockTask.city == city,
    ).first()

    if task is None:
        task = StreatBlockTask(
            city=city,
            status="failed",
            result_path=None
        )
        session.add(task)
    else:
        task.status = "failed"
        task.result_path = None

    session.commit()
    session.close()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process city street block')
    parser.add_argument('city_name', type=str, help='city name')
    args = parser.parse_args()
    city_name = args.city_name
    if city_name == None:
        city_name = "Beijing"
    try:
        process_street_block(city_name)
        print(f"Process {city_name} street block successfully")
        update_or_create_street_block_task(city_name, CITY_DIR / city_name / 'city_street_block.json')
    except Exception as e:
        print(f"Process {city_name} street block failed: {e}")
        update_or_create_street_block_task_failed(city_name)
        raise
        