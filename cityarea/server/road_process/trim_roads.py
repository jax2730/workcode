import geopandas as gpd
import shapely
import shapely.ops
import networkx as nx
import matplotlib.pyplot as plt
import json
import pathlib
import sys
sys.path.append(pathlib.Path(__file__).parent.parent.absolute().as_posix())
from constants import *
ROAD_DIR = DATA_DIR / 'road'


def get_intersections_endpoints(intersection: gpd.GeoDataFrame)->gpd.GeoDataFrame:
    """
    Given a GeoDataFrame with intersect point or overlaps lines, this function returns the endpoints of the intersection.
    """
    point_gdf = gpd.GeoDataFrame(columns=intersection.columns, crs=intersection.crs)
    for _,row in intersection.iterrows():
        if row.geometry.geom_type == 'Point':
            point_gdf.loc[len(point_gdf)] = row
        elif row.geometry.geom_type == 'LineString':
            endpoint1 = shapely.Point(row.geometry.coords[0])
            endpoint2 = shapely.Point(row.geometry.coords[-1])
            new_row = row.copy()
            new_row['geometry'] = endpoint1
            point_gdf.loc[len(point_gdf)] = new_row
            new_row = row.copy()
            new_row['geometry'] = endpoint2
            point_gdf.loc[len(point_gdf)] = new_row
        else:
            print(f"Invalid geometry type: {row.geometry.geom_type}")
    point_gdf.set_crs(epsg=4326, inplace=True)
    return point_gdf

def dissolve_intersection_endpoints(points_gdf: gpd.GeoDataFrame)->gpd.GeoDataFrame:
    """
    Given a GeoDataFrame with endpoints of intersection, this function dissolves the points at same location into a single point.
    """
    points_gdf.set_crs(epsg=4326, inplace=True)
    points_gdf.to_crs(epsg=3857, inplace=True)
    dissolved_points = gpd.GeoDataFrame(columns=['geometry', 'id', 'id_list'], crs=points_gdf.crs)
    for _,row in points_gdf.iterrows():
        coord = row.geometry
        if len(dissolved_points) == 0:
            dissolved_points.loc[len(dissolved_points)] = {'geometry': row.geometry, 'id': len(dissolved_points), 'id_list': row['id_list']}
            continue
        #cheack if the point is already in the dissolved_points
        mask = dissolved_points.apply(lambda x: x.geometry.distance(coord)<100, axis=1)
        if not mask.any():
            dissolved_points.loc[len(dissolved_points)] = {'geometry': row.geometry, 'id': len(dissolved_points), 'id_list': row['id_list']}
        else:
            id_lists = dissolved_points.loc[mask, 'id_list'].tolist()
            id_set = set()
            for id_list in id_lists:
                ids = id_list.split(',')
                id_set.update(ids)
            ids = row['id_list'].split(',')
            id_set.update(ids)
            new_id_list = ','.join(list(id_set))
            dissolved_points.at[mask.index[0], 'id_list'] = new_id_list
    dissolved_points.set_crs(epsg=3857, inplace=True)
    dissolved_points.to_crs(epsg=4326, inplace=True)
    return dissolved_points

def trim_roads(roads: gpd.GeoDataFrame, points_gdf: gpd.GeoDataFrame)->gpd.GeoDataFrame:
    """
    Given a GeoDataFrame with roads and a GeoDataFrame with endpoints of intersection, this function trims the roads at the endpoints of intersection.
    """
    roads.to_crs(epsg=3857, inplace=True)
    points_gdf.to_crs(epsg=3857, inplace=True)
    trimmed_roads = gpd.GeoDataFrame(columns=roads.columns, crs=roads.crs)
    multipoint = shapely.geometry.MultiPolygon([point.geometry.buffer(10) for _,point in points_gdf.iterrows()])
    for _,row in roads.iterrows():
        if row.geometry.intersects(multipoint):
            trimedLines = shapely.ops.split(row.geometry, multipoint)
            for line in trimedLines.geoms:
                if line.length > 50:
                    new_row = row.copy()
                    new_row['geometry'] = line
                    trimmed_roads.loc[len(trimmed_roads)] = new_row
        else:
            if row.geometry.geom_type == 'MultiLineString':
                for line in row.geometry.geoms:
                    if line.length > 50:
                        new_row = row.copy()
                        new_row['geometry'] = line
                        trimmed_roads.loc[len(trimmed_roads)] = new_row
            else:
                trimmed_roads.loc[len(trimmed_roads)] = row
    trimmed_roads.set_crs(epsg=3857, inplace=True)
    trimmed_roads.to_crs(epsg=4326, inplace=True)
    roads.to_crs(epsg=4326, inplace=True)
    points_gdf.to_crs(epsg=4326, inplace=True)
    return trimmed_roads

def create_graph(trimmed_roads: gpd.GeoDataFrame)->nx.Graph:
    """
    Given a GeoDataFrame with trimmed roads, this function creates a networkx graph.
    """
    graph = nx.MultiGraph()
    for _,row in trimmed_roads.iterrows():
        # find if edge already exists
        if graph.has_edge(row.geometry.coords[0], row.geometry.coords[-1]):
            # if yes, check if the edge has the same geometry
            found = False
            for i,data in graph[row.geometry.coords[0]][row.geometry.coords[-1]].items():
                if row.geometry.centroid.distance(data['geometry'].centroid) < 2000.0 / 111319.9:
                    found = True
                    graph[row.geometry.coords[0]][row.geometry.coords[-1]][i]['id'].append(row.id)
                    graph[row.geometry.coords[0]][row.geometry.coords[-1]][i]['name'].append(row['name'])
                    break
            if not found:
                # if no, create a new edge with the same id and name
                graph.add_edge(row.geometry.coords[0], row.geometry.coords[-1], id=[row.id], name=[row['name']], geometry=row.geometry)
        else:
            graph.add_edge(row.geometry.coords[0], row.geometry.coords[-1], id=[row.id], name=[row['name']], geometry=row.geometry)
    return graph

def recreate_roads(graph: nx.MultiGraph, crs: str)->tuple[gpd.GeoDataFrame, gpd.GeoDataFrame]:
    """
    Given a networkx graph, this function recreates the roads with the trimmed parts.
    """
    roads = gpd.GeoDataFrame(columns=['geometry', 'id', 'name', 'line_count', 'index'], crs=crs)
    points = gpd.GeoDataFrame(columns=['geometry', 'ids', 'names', 'cross_pattern','line_index'], crs=crs)
    railway_lines = {}
    for u,v,data in graph.edges(data=True):
        new_row = {'geometry': data['geometry'], 'id': data['id'], 'name': data['name'], 'line_count': len(data['id']), 'index': len(roads)}
        for name in data['name']:
            if name not in railway_lines:
                railway_lines[name] = [len(roads)]
            railway_lines[name].append(len(roads))
        data['index'] = len(roads)
        roads.loc[len(roads)] = new_row
    graph.nodes(data=True)
    for node in graph.nodes:
        new_row = {'ids':[], 'names':[], 'cross_pattern':[],'geometry':shapely.geometry.Point(node),'line_index':[]}
        for u,v,data in graph.edges(node, data=True):
            new_row['ids'].extend(data['id'])
            new_row['names'].extend(data['name'])
            new_row['line_index'].append(data['index'])
            new_row['cross_pattern'].append(len(data['id']))
        points.loc[len(points)] = new_row
    roads.set_crs(crs, inplace=True)
    points.set_crs(crs, inplace=True)
    return roads, points

def sort_roads_w_same_name(roads: gpd.GeoDataFrame)->tuple[dict[str, list[int]], gpd.GeoDataFrame]:
    """
    Given a GeoDataFrame with roads, this function will return a dictionary,
    where the keys are the names of the roads and the values are the
    indices of the roads with the same name, sequence of the railway make sure
    the railway can be connected end to end.
    """
    railway_lines = {}
    for i,row in roads.iterrows():
        for name in row['name']:
            if name not in railway_lines:
                railway_lines[name] = [i]
            else:
                railway_lines[name].append(i)
    # sort the roads with the same name
    for name in railway_lines:
        graph = nx.Graph()
        lines = railway_lines[name]
        if len(lines) == 1:
            continue
        for i in lines:
            line = roads.at[i, 'geometry']
            graph.add_edge(line.coords[0], line.coords[-1], weight=line.length, index=i)
        railway_lines[name] = []
        components = nx.connected_components(graph)
        subgraphs = [graph.subgraph(c) for c in components]
        for subgraph in subgraphs:
            if len(subgraph.nodes) < 2:
                
                continue
            endpoints = [n for n in subgraph.nodes if subgraph.degree[n] == 1]
            path = []
            max_length = 0
            for i in range(len(endpoints)):
                for j in range(i+1, len(endpoints)):
                    try:
                        new_path = nx.shortest_path(subgraph, endpoints[i], endpoints[j], weight='weight')
                        length = sum([subgraph[u][v]['weight'] for u,v in zip(new_path[:-1], new_path[1:])])
                        if length > max_length:
                            max_length = length
                            path = new_path
                    except:
                        pass
            if len(path) > 0:
                new_line_list = []
                for u,v in zip(path[:-1], path[1:]):
                    edge = subgraph[u][v]
                    index = edge['index']
                    geometry = roads.at[index, 'geometry']
                    new_line_list.append(index) if u == geometry.coords[0] and v == geometry.coords[-1] else new_line_list.append(-index)
                railway_lines[name].append(None) if len(railway_lines[name]) != 0 and railway_lines[name][-1] != None else None
                railway_lines[name].extend(new_line_list)
    return railway_lines, roads

def extend_roads_to_endpoints(roads: gpd.GeoDataFrame, points_gdf: gpd.GeoDataFrame)->gpd.GeoDataFrame:
    """
    Given a GeoDataFrame with roads and a GeoDataFrame with endpoints of intersection, this function extends the roads to the endpoints of intersection.
    """
    roads.to_crs(epsg=3857, inplace=True)
    points_gdf.to_crs(epsg=3857, inplace=True)
    for i,row in roads.iterrows():
        if row.geometry.geom_type == 'LineString':
            row_geometry: shapely.geometry.LineString = row.geometry
            ep1 = shapely.geometry.Point(row_geometry.coords[0])
            ep2 = shapely.geometry.Point(row_geometry.coords[-1])
            distance1 = points_gdf.distance(ep1).min()
            distance1_index = points_gdf.distance(ep1).idxmin()
            distance2 = points_gdf.distance(ep2).min()
            distance2_index = points_gdf.distance(ep2).idxmin()
            if distance1 < 100:
                point = points_gdf.at[distance1_index, 'geometry']
                new_geometry = shapely.geometry.LineString([point, *row_geometry.coords])
                row_geometry = new_geometry
            if distance2 < 100:
                point = points_gdf.at[distance2_index, 'geometry']
                new_geometry = shapely.geometry.LineString([*row_geometry.coords, point])
                row_geometry = new_geometry
            roads.at[i, 'geometry'] = row_geometry
        if row.geometry.geom_type == 'MultiLineString':
            new_geometry = []
            for line in row.geometry.geoms:
                row_geometry: shapely.geometry.LineString = line
                ep1 = shapely.geometry.Point(row_geometry.coords[0])
                ep2 = shapely.geometry.Point(row_geometry.coords[-1])
                distance1 = points_gdf.distance(ep1).min()
                distance1_index = points_gdf.distance(ep1).idxmin()
                distance2 = points_gdf.distance(ep2).min()
                distance2_index = points_gdf.distance(ep2).idxmin()
                if distance1 < 100:
                    point = points_gdf.at[distance1_index, 'geometry']
                    new_line = shapely.geometry.LineString([point, *row_geometry.coords])
                    row_geometry = new_line
                if distance2 < 100:
                    point = points_gdf.at[distance2_index, 'geometry']
                    new_line = shapely.geometry.LineString([*row_geometry.coords, point])
                    row_geometry = new_line
                new_geometry.append(row_geometry)
            roads.at[i, 'geometry'] = shapely.geometry.MultiLineString(new_geometry)
    roads.set_crs(epsg=3857, inplace=True)
    roads.to_crs(epsg=4326, inplace=True)
    points_gdf.to_crs(epsg=4326, inplace=True)
    return roads

def trim_roads_country(country_code:str):
    country_dir = ROAD_DIR / country_code
    road_path = country_dir/'roads_single_lines.gpkg'
    roads_intersections_path = country_dir/'roads_interasection.gpkg'
    output_path = country_dir/'road_trimmed.gpkg'
    point_output_path = country_dir / 'road_intersections_endpoints.gpkg'
    road_topology_path = country_dir / 'road_topology.json'

    roads = gpd.read_file(road_path)
    roads_intersections = gpd.read_file(roads_intersections_path)

    points_gdf = get_intersections_endpoints(roads_intersections)
    points_gdf = dissolve_intersection_endpoints(points_gdf)
    trimmed_roads = trim_roads(roads, points_gdf)
    extended_roads = extend_roads_to_endpoints(trimmed_roads, points_gdf)
    # Create the graph
    graph = create_graph(extended_roads)
    extended_roads, points_gdf = recreate_roads(graph, roads.crs)
    j,extended_roads = sort_roads_w_same_name(extended_roads)

    points_gdf.to_file(point_output_path, driver='GPKG')
    extended_roads.to_file(output_path, driver='GPKG')

    with open(road_topology_path,'w', encoding='utf-8') as f:
        json.dump(j, f, ensure_ascii=False, indent=4)


if __name__ == '__main__':

    trim_roads_country('US')