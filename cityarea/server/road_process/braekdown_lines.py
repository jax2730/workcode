import geopandas as gpd
import pandas as pd
import matplotlib.pyplot as plt
import shapely
import shapely.ops as ops
import shapely.geometry as geom
import networkx as nx
import numpy as np
if __name__ == "__main__" or __package__ is None or __package__ == '':
    import road_util
else:
    from . import road_util

import pathlib
import sys
sys.path.append(pathlib.Path(__file__).parent.parent.absolute().as_posix())
from constants import *
ROAD_DIR = DATA_DIR / 'road'

def snap_multi_lines(multi_lines: shapely.geometry.MultiLineString, tolerance: float = 200):
    """
    Snap multi-lines to each other.

    Parameters
    ----------
    multi_lines : shapely.geometry.MultiLineString
        Multi-lines to snap.
    tolerance : float, optional
        Tolerance for snapping, by default 500.0 / 111319.9

    Returns
    -------
    shapely.geometry.MultiLineString
        Snapped multi-lines.
    """
    # Create a new MultiLineString to store the snapped lines
    snapped_lines = shapely.geometry.MultiLineString()
    line_list = [*multi_lines.geoms]
    #sort the line_list by length
    line_list.sort(key=lambda x: x.length, reverse=True)
    snapped_list = []
    while len(line_list)>0:
        # Get the first two lines
        line1 = line_list.pop(0)
        snapped_line1 = shapely.ops.snap(line1, shapely.geometry.MultiLineString(line_list), tolerance)
        snapped_list.append(snapped_line1)
    snapped_lines = shapely.geometry.MultiLineString(snapped_list)
    return snapped_lines


def breakdown_multi_lines(line_gdf: gpd.GeoDataFrame):
    """
    Break down multi-lines into single lines.

    Parameters
    ----------
    line_gdf : GeoDataFrame
        GeoDataFrame containing multi-lines.

    Returns
    -------
    GeoDataFrame
        GeoDataFrame containing single lines.
    """
    # Create a new GeoDataFrame to store the single lines
    single_line_gdf = gpd.GeoDataFrame(columns=line_gdf.columns,crs= line_gdf.crs)
    #disslove the line_gdf by name
    line_gdf = line_gdf.to_crs(epsg=3857)
    line_gdf.dissolve(by='name', aggfunc='first') if 'name' in line_gdf.columns else line_gdf.dissolve(by='id') if 'id' in line_gdf.columns else None
    line_copy = line_gdf.copy()

    point_set = set()
    # Loop through each row in the original GeoDataFrame
    for index, row in line_gdf.iterrows():
        # If the geometry is a multi-line, loop through each part and add it to the new GeoDataFrame
        if type(row.geometry) == shapely.geometry.MultiLineString:
            gdf_without_row = line_copy.drop(index)
            #snapped = snap_multi_lines(row.geometry)
            snapped = row.geometry
            if type(snapped) == shapely.geometry.LineString:
                merged_lines = snapped
            else:
                merged_lines = shapely.ops.linemerge(snapped)
            if type(merged_lines) == shapely.geometry.LineString:
                    new_row = row.copy()
                    new_row['geometry'] = merged_lines
                    single_line_gdf.loc[len(single_line_gdf)] = new_row
            else:
                merged_lines = shapely.ops.unary_union(merged_lines)
                G = nx.Graph()
                for line in merged_lines.geoms:
                    for i in range(len(line.coords)-1):
                        u = line.coords[i]
                        v = line.coords[i+1]
                        segment = shapely.geometry.LineString([u,v])
                        G.add_edge(u,v,weight = segment.length,geometry = segment)
                # get path with max length
                components = nx.connected_components(G)
                subgraphs = [G.subgraph(c) for c in components]
                #sort component by subgraph's area
                subgraphs.sort(key=lambda x: x.number_of_nodes(), reverse=True)

                for subgraph in subgraphs:
                    if len(subgraph.nodes) < 2:
                        continue
                    
                    endpoints = [n for n in subgraph.nodes if subgraph.degree[n] == 1]
                    endpoints_in_set = [n in point_set for n in endpoints]
                    
                    path = []
                    max_length = 0
                    for i in range(len(endpoints)):
                        for j in range(i+1, len(endpoints)):
                            try:
                                # new_path = nx.shortest_path(G, endpoints[i], endpoints[j],weight='weight')
                                # length = sum(subgraph[u][v]['weight'] for u,v in zip(new_path[:-1],new_path[1:]))
                                # if length > max_length:
                                #     max_length = length
                                #     path = new_path
                                new_path = nx.shortest_path(G, endpoints[i], endpoints[j],weight='weight')
                                distance = geom.Point(endpoints[i]).distance(geom.Point(endpoints[j]))
                                length = sum(subgraph[u][v]['weight'] for u,v in zip(new_path[:-1],new_path[1:]))
                                if distance < 0.5 * length:
                                    continue
                                if length > max_length:
                                    max_length = length
                                    path = new_path

                            except nx.exception.NetworkXNoPath:
                                print(f"No path found between {endpoints[i]} and {endpoints[j]}")
                                continue
                    if len(endpoints_in_set) >1:
                        max_length_in_set = 0
                        for i in range(len(endpoints_in_set)):
                            for j in range(i+1, len(endpoints_in_set)):
                                try:
                                    new_path_in_set = nx.shortest_path(G, endpoints[i], endpoints[j],weight='weight')
                                    length = sum(subgraph[u][v]['weight'] for u,v in zip(new_path[:-1],new_path[1:]))
                                    if distance < 0.5 * length:
                                        continue
                                    if length > max_length:
                                        max_length = length
                                        path = new_path
                                except nx.exception.NetworkXNoPath:
                                    print(f"No path found between {endpoints[i]} and {endpoints[j]}")
                                    continue
                        if max_length_in_set > max_length * 0.7:
                            path = new_path_in_set
                    # create new line
                    segments = [subgraph[u][v]['geometry'] for u,v in zip(path[:-1],path[1:])]
                    new_line = geom.MultiLineString(segments)
                    new_line:geom.LineString = shapely.ops.linemerge(new_line)
                    # check if line is a folded line
                    if new_line.is_empty or new_line.geom_type != 'LineString':
                        continue
                    # if new_line.geom_type == 'LineString':
                    #     distance_0 = geom.Point(new_line.coords[0]).distance(geom.Point(new_line.coords[-1]))
                    #     distance_1 = geom.Point(new_line.coords[1]).distance(geom.Point(new_line.coords[-2]))
                    #     distance_2 = geom.Point(new_line.coords[2]).distance(geom.Point(new_line.coords[-3]))
                    #     if distance_0 + distance_1 + distance_2 < new_line.length * 0.05:
                    #         # this line is folded
                    #         start_p = geom.Point(new_line.coords[0])
                    #         max_length = 0
                    #         max_index = 0
                    #         for i in range(1,len(new_line.coords)):
                    #             distance = start_p.distance(geom.Point(new_line.coords[i]))
                    #             if distance > max_length:
                    #                 max_index = i
                    #                 max_length = distance
                    #         segments = []
                    #         for i in range(max_index-1):
                    #             segments.append((new_line.coords[i],new_line.coords[i+1]))
                    #         new_line = geom.MultiLineString(segments)
                    #         new_line:geom.LineString = shapely.ops.linemerge(new_line)
                    # else:
                    #     continue
                    new_row = row.copy()
                    new_row['geometry'] = new_line
                    single_line_gdf.loc[len(single_line_gdf)] = new_row
                    for p in new_line.coords:
                        point_set.add(p)
        # If the geometry is a single line, add it to the new GeoDataFrame
        else:
                single_line_gdf.loc[len(single_line_gdf)] = row.copy()
    single_line_gdf.crs = line_gdf.crs
    single_line_gdf = single_line_gdf.to_crs(epsg=4326)
    return single_line_gdf

def remove_close_lines(line_gdf: gpd.GeoDataFrame, tolerance: float = 500.0):
    line_gdf = line_gdf.to_crs(epsg=3857)
    for i, row in line_gdf.iterrows():
        if row['geometry'] is None or row['geometry'].geom_type == 'LineString':
            continue
        multiline = geom.MultiLineString(row['geometry'])
        snapped = ops.snap(multiline, multiline.centroid, tolerance=tolerance)
        multiline = ops.unary_union(snapped)
        if multiline.geom_type == 'MultiLineString':
            multiline = ops.linemerge(multiline)
        if multiline.geom_type == 'MultiLineString':
            line_list = [line for line in multiline.geoms]
            #sort line by length
            line_list.sort(key=lambda x: x.length,reverse=True)
        elif multiline.is_empty:
            continue
        else:
            line_list = [multiline]
        new_line_list: list[geom.LineString] = []
        for j, line in enumerate(line_list):
                
            if j == 0:
                new_line_list.append(line)
            else:
                too_close = False
                relative_distance = line.length/5 if line.length/5<6000 else 6000 
                for new_line in new_line_list:
                    if line.centroid.distance(new_line.centroid) < tolerance\
                        or (line.distance(geom.Point(new_line.coords[0]))< relative_distance and line.distance(geom.Point(new_line.coords[-1]))< relative_distance and line.distance(new_line.centroid)< relative_distance)\
                        or (new_line.distance(geom.Point(line.coords[0]))< relative_distance\
                        and new_line.distance(geom.Point(line.coords[-1]) )< relative_distance):
                        too_close = True
                        break
                if not too_close:
                    new_line_list.append(line)
            # if row['NAME'] == 'Ｓ２０':
            #     #plot line
            #     a = plt.plot(*line.xy, color='black')
            #     plt.show()
            #     print(too_close)

        new_multiline = geom.MultiLineString(new_line_list)
        line_gdf.at[i, 'geometry'] = new_multiline
    #remove empty geometry
    line_gdf = line_gdf[line_gdf['geometry'].apply(lambda x: x is not None and x.is_valid)]
    line_gdf = line_gdf.to_crs(epsg=4326)
    return line_gdf

def process_roads_country(country_code:str):
    country_dir = ROAD_DIR / country_code
    road_path = country_dir/'roads_filtered.gpkg'
    road_filter_proj_path = ROAD_DIR/'DE/roads_filtered_proj.gpkg'
    road_simplified_path = country_dir/'roads_simplified.gpkg'
    snappde_path = country_dir/'roads_snapped.gpkg'
    output_path = country_dir/'roads_single_lines.gpkg'
    road_gdf = gpd.read_file(road_path, encoding='utf-8')
    road_gdf = road_gdf.to_crs(epsg=3857)
    road_gdf.to_file(road_filter_proj_path, driver='GPKG')
    road_util.dP_simplification(road_filter_proj_path, road_simplified_path, tolerance=500.0)
    road_util.snap_layer_to_self(road_simplified_path, snappde_path, tolerance=1000.0)
    road_gdf = gpd.read_file(snappde_path, encoding='utf-8')


    road_gdf['length'] = road_gdf.geometry.length
    road_gdf=road_gdf.sort_values(by='length')
    road_gdf = breakdown_multi_lines(road_gdf)
    road_gdf = road_gdf.dissolve(by = 'id',method='coverage', as_index=False)
    road_gdf = remove_close_lines(road_gdf)

    road_gdf.to_file(output_path, driver='GPKG')

def process_roads_without_snape_country(country_code:str):
    country_dir = ROAD_DIR / country_code
    road_path = country_dir/'roads_snapped.gpkg'
    road_simplified_path = country_dir/'roads_simplified.gpkg'
    output_path = country_dir/'roads_single_lines.gpkg'
    road_util.dP_simplification(road_path, road_simplified_path, tolerance=500.0)
    road_gdf = gpd.read_file(road_simplified_path, encoding='utf-8')
    road_gdf['length'] = road_gdf.geometry.length
    road_gdf=road_gdf.sort_values(by='length')
    road_gdf = breakdown_multi_lines(road_gdf)
    road_gdf = road_gdf.dissolve(by = 'id',method='coverage', as_index=False)
    road_gdf = remove_close_lines(road_gdf)

    road_gdf.to_file(output_path, driver='GPKG')

# Example usage
if __name__ == '__main__':
    # Load the data
    road_path = ROAD_DIR/'DE/roads_filtered.gpkg'
    road_gdf = gpd.read_file(road_path, encoding='utf-8')
    road_gdf = road_gdf.to_crs(epsg=3857)
    road_gdf = road_util.my_snap(road_gdf,road_gdf, threshold=1000.0)
    road_gdf = road_gdf.to_crs(epsg=4326)
    #sort gdf by distance
    road_gdf['length'] = road_gdf.geometry.length
    road_gdf=road_gdf.sort_values(by='length')
    # road_gdf = remove_close_lines(road_gdf)
    road_gdf = breakdown_multi_lines(road_gdf)
    road_gdf = road_gdf.dissolve(by = 'id',method='coverage', as_index=False)
    road_gdf = remove_close_lines(road_gdf)
    
    # Output:
    output_path = ROAD_DIR/'DE/roads_single_lines.gpkg'
    road_gdf.to_file(output_path, driver='GPKG')
    