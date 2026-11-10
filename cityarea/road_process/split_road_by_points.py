import shapely.geometry
import geopandas as gpd
import matplotlib.pyplot as plt
import shapely
import shapely.ops

def split_road_by_points(road_gdf, points_gdf):
    """
    Split a road by points.

    Parameters
    ----------
    road_gdf : GeoDataFrame
        The road to be split.
    points_gdf : GeoDataFrame
        The points to split the road by.

    Returns
    -------
    GeoDataFrame
        The road with the points split.
    """
    road_gdf = road_gdf.copy()
    points_gdf = points_gdf.copy()
    trimmed_road = gpd.GeoDataFrame(columns=road_gdf.columns, crs=road_gdf.crs)
    for _,row in road_gdf.iterrows():
        if row.geometry is None:
            continue
        close_points = points_gdf[points_gdf.geometry.apply(lambda x: row.geometry.distance(x) < 100)]
        point_on_roads = []
        for _,point_row in close_points.iterrows():
            point_on_road = shapely.ops.nearest_points(row.geometry, point_row.geometry)[0]
            point_on_roads.append(point_on_road)
        if len(point_on_roads) > 0:
            multipoint_c = shapely.geometry.MultiPoint(point_on_roads)
            snapped_line = shapely.ops.snap(row.geometry, multipoint_c, 100)
            trimedLines = shapely.ops.split(snapped_line, multipoint_c )
            for line in trimedLines.geoms:
                new_row = row.copy()
                new_row['geometry'] = line
                trimmed_road.loc[len(trimmed_road)] = new_row
        else:
            trimmed_road.loc[len(trimmed_road)] = row
    trimmed_road.crs = road_gdf.crs
    return trimmed_road

# Example usage:
if __name__ == '__main__':
    import pathlib
    road_path = pathlib.Path(__file__).parent.joinpath(r'../out/road/road_snapped_single_lines.geojson')
    points_path = pathlib.Path(__file__).parent.joinpath(r'../out/road/bridges_with_intersection.geojson')

    road_gdf = gpd.read_file(str(road_path))
    points_gdf = gpd.read_file(str(points_path))
    trimmed_road = split_road_by_points(road_gdf, points_gdf)
    trimmed_road.to_file(str(pathlib.Path(__file__).parent.joinpath(r'../out/road/road_split_by_points.geojson')), driver='GeoJSON')