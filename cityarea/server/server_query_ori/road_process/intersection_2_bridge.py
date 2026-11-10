import geopandas as gpd
import shapely
import matplotlib.pyplot as plt
import shapely.ops
import pathlib
import sys
sys.path.append(pathlib.Path(__file__).parent.parent.absolute().as_posix())
from constants import *
ROAD_DIR = DATA_DIR / 'road'
def intersaction_line_2_point(intersetion:gpd.GeoDataFrame)->gpd.GeoDataFrame:
    """
    This function takes a GeoDataFrame with a single line geometry and a GeoDataFrame with a single point geometry and returns a GeoDataFrame with the intersection point.
    """
    intersetion_point = gpd.GeoDataFrame(columns=['name', 'geometry'], crs=intersetion.crs, geometry='geometry')
    for _, row in intersetion.iterrows():
        row: gpd.GeoSeries
        id_list = row['id_list'].split(',')
        idr = id_list[0]
        idl = id_list[1]
        if row.geometry.geom_type == 'Point':
            new_row = {'name': f'{idr}-{idl}', 'geometry': row['geometry']}
            intersetion_point.loc[len(intersetion_point)] = new_row
        else:
            line_geom = row.geometry
            new_row = {'name': f'{idr}-{idl}', 'geometry':shapely.geometry.Point(line_geom.coords[0])}
            intersetion_point.loc[len(intersetion_point)] = new_row
            if line_geom.length > 200:
                new_row = {'name': f'{idr}-{idl}-a', 'geometry': shapely.geometry.Point(line_geom.coords[-1])}
                intersetion_point.loc[len(intersetion_point)] = new_row
    intersetion_point.crs = intersetion.crs
    return intersetion_point

def get_endpoints(road:gpd.GeoDataFrame)->gpd.GeoDataFrame:
    """
    This function takes a GeoDataFrame with a single line geometry and returns a GeoDataFrame with the endpoints of the road.
    """
    road_endpoints = gpd.GeoDataFrame(columns=['id', 'geometry'], crs=road.crs, geometry='geometry')
    for _, row in road.iterrows():
        row: gpd.GeoSeries
        if row.geometry is None:
            continue
        if row.geometry.geom_type == 'LineString':
            id = row['id']
            new_row = {'id': f'{id}-a', 'geometry': shapely.geometry.Point(row.geometry.coords[0])}
            road_endpoints.loc[len(road_endpoints)] = new_row
            new_row = {'id': f'{id}-b', 'geometry': shapely.geometry.Point(row.geometry.coords[-1])}
            road_endpoints.loc[len(road_endpoints)] = new_row
    road_endpoints.crs = road.crs
    return road_endpoints

def remove_close_bridges(bridges:gpd.GeoDataFrame, intersections:gpd.GeoDataFrame ,threshold:float)->gpd.GeoDataFrame:
    """
    This function takes a GeoDataFrame with bridges and a GeoDataFrame with intersections and removes the bridges that are closer than the threshold to an intersection.
    """
    bridges_to_remove = set()
    bridges_with_intersection = gpd.GeoDataFrame(columns=['id', 'geometry'], crs=intersections.crs, geometry='geometry')
    for i, row in bridges.iterrows():
        row: gpd.GeoSeries
        if row.geometry is None:
            continue
        too_close = False
        for j, intersection_row in intersections.iterrows():
            intersection_row: gpd.GeoSeries
            if row.geometry.distance(intersection_row.geometry) < threshold:
                too_close = True
                break
        if not too_close:
            bridges_with_intersection.loc[len(bridges_with_intersection)] = row
        
    for _,row in intersections.iterrows():
        row: gpd.GeoSeries
        bridges_with_intersection.loc[len(bridges_with_intersection)] = row
    bridges_with_intersection.crs = intersections.crs
    return bridges_with_intersection

def get_isolation_endpoints(roads:gpd.GeoDataFrame, bridges:gpd.GeoDataFrame)->gpd.GeoDataFrame:
    """
    This function takes a GeoDataFrame with a single line geometry and a GeoDataFrame with bridges and
      returns a GeoDataFrame with the endpoints of the road that are far enough from the bridges to be considered as isolation points.
    """
    road_endpoints = gpd.GeoDataFrame(columns=['id', 'geometry'], crs=roads.crs, geometry='geometry')
    def snap_point_to_road(point:shapely.geometry.Point, road:gpd.GeoSeries)->shapely.geometry.Point:
        other_roads = roads[roads.index != road.name]
        snapped_point = shapely.ops.nearest_points(point, other_roads.union_all())[1]
        distance = point.distance(snapped_point)
        if distance > 1000:
            return point
        return snapped_point
    def far_enough_from_bridges(point:shapely.geometry.Point, bridges:gpd.GeoDataFrame)->bool:
        for _, bridge_row in bridges.iterrows():
            bridge_row: gpd.GeoSeries
            if point.distance(bridge_row.geometry) < 2000:
                return False
        return True
    for _, row in roads.iterrows():
        row: gpd.GeoSeries
        if row.geometry is None:
            continue
        if row.geometry.geom_type == 'LineString':
            endpoint_a = shapely.geometry.Point(row.geometry.coords[0])
            if far_enough_from_bridges(endpoint_a, bridges):
                snapped_a = snap_point_to_road(endpoint_a, row)
                new_row = {'NAME': f'{row.name}-a', 'geometry': snapped_a}
                road_endpoints.loc[len(road_endpoints)] = new_row
            endpoint_b = shapely.geometry.Point(row.geometry.coords[-1])
            if far_enough_from_bridges(endpoint_b, bridges):
                snapped_b = snap_point_to_road(endpoint_b, row)
                new_row = {'NAME': f'{row.name}-b', 'geometry': snapped_b}
                road_endpoints.loc[len(road_endpoints)] = new_row
    road_endpoints.crs = roads.crs
    return road_endpoints

if __name__ == '__main__':
    import pathlib
    #bridge_path = pathlib.Path(__file__).parent.joinpath('../out/road/snapped_bridge.geojson')
    intersections_path = ROAD_DIR/'DE/roads_interasection.gpkg'
    road_path = ROAD_DIR/'DE/roads_single_lines.gpkg'
    bridges = gpd.GeoDataFrame()
    intersections = gpd.read_file(intersections_path)
    roads = gpd.read_file(road_path)
    
    intersections_point = intersaction_line_2_point(intersections)
    road_endpoints = get_endpoints(roads)
    bridges_with_intersection = remove_close_bridges(bridges, intersections_point, 450)
    bridges_with_intersection_path = ROAD_DIR/'DE/bridges_with_intersection.gpkg'
    bridges_with_intersection.to_file(bridges_with_intersection_path, driver='GPKG')
    #bridges_with_intersection_path = pathlib.Path(__file__).parent.joinpath('../out/road/bridges_with_intersection.geojson')
    #bridges_with_intersection = gpd.read_file(bridges_with_intersection_path)
    endpoint_isolation = get_isolation_endpoints(roads, bridges_with_intersection)
    endpoint_isolation_path = ROAD_DIR/'DE/bridges_with_intersection.gpkg'
    endpoint_isolation.to_file(endpoint_isolation_path, driver='GPKG')

    # endpoint_isolation = gpd.read_file(endpoint_isolation_path)

    # bridges_with_intersection = remove_close_bridges(endpoint_isolation, bridges_with_intersection, 450)
    bridges_with_intersection = remove_close_bridges(road_endpoints, bridges_with_intersection, 2000)
    bridges_with_intersection.to_file(ROAD_DIR/'DE/bridges_with_intersection.gpkg', driver='GPKG')

    #endpoint_isolation.to_file(pathlib.Path(__file__).parent.joinpath('../out/road/bridges_with_intersection.geojson'), driver='GeoJSON')