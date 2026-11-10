import geopandas as gpd
import pandas as pd
import shapely.geometry as geom
import shapely.ops as ops

import pathlib
import sys
sys.path.append(pathlib.Path(__file__).parent.parent.absolute().as_posix())
from constants import *
if __name__ == "__main__" or __package__ is None:
    import braekdown_lines
else:
    from . import braekdown_lines

ROAD_DIR = DATA_DIR / 'road'

def road_intersection(road_gdf:gpd.GeoDataFrame):
    intersection = gpd.GeoDataFrame(columns=['geometry', 'id_list'],crs=road_gdf.crs)

    itered_set = set()

    def process_intersection(intersection_geom: geom.MultiLineString, id_list: tuple):
        i, j = id_list
        match (intersection_geom.geom_type):
            case 'Point':
                intersection.loc[len(intersection)] = [intersection_geom, f"{i}, {j}"]
            case 'LineString':
                intersection.loc[len(intersection)] = [intersection_geom, f"{i}, {j}"]
            case 'MultiPoint':
                intersection_geom: geom.MultiPoint
                for point in intersection_geom.geoms:
                    intersection.loc[len(intersection)] = [point, f"{i}, {j}"]
            case 'MultiLineString':
                intersection_geom: geom.MultiLineString
                intersection_geom = ops.unary_union(intersection_geom)
                intersection_geom = ops.linemerge(intersection_geom)
                if intersection_geom.geom_type == 'LineString':
                    intersection.loc[len(intersection)] = [intersection_geom, f"{i}, {j}"]
                else:
                    for line in intersection_geom.geoms:
                        intersection.loc[len(intersection)] = [line, f"{i}, {j}"]
            case 'GeometryCollection':
                intersection_geom: geom.GeometryCollection
                for geometry in intersection_geom.geoms:
                    process_intersection(geometry, id_list)
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

def road_intersection_country(country_code:str):
    country_dir = ROAD_DIR / country_code
    road_path = country_dir/'roads_single_lines.gpkg'
    output_path = country_dir/'roads_interasection.gpkg'

    road_gdf = gpd.read_file(road_path, encoding='utf-8')

    intersection = road_intersection(road_gdf)
    intersection.set_crs(epsg=4326, inplace=True)
    intersection.to_file(country_dir/'raw_intersection.gpkg')

    intersection_lines = intersection[intersection.geometry.geom_type == 'LineString']
    intersection_multilines = intersection[intersection.geometry.geom_type == 'MultiLineString']
    intersection_points = intersection[intersection.geometry.geom_type == 'Point']
    intersection_multipoints = intersection[intersection.geometry.geom_type == 'MultiPoint']

    intersection_lines = pd.concat([intersection_lines, intersection_multilines])
    intersection_points = pd.concat([intersection_points, intersection_multipoints])

    intersection_lines = intersection_lines.dissolve(by='id_list', as_index=False)
    intersection_lines = braekdown_lines.breakdown_multi_lines(intersection_lines)

    intersection:gpd.GeoDataFrame = pd.concat([intersection_lines, intersection_points])

    intersection.to_file(output_path)
    road_gdf.to_file(road_path)



if __name__ == '__main__':
    # road_path = ROAD_DIR/'DE/roads_single_lines.gpkg'
    # output_path = ROAD_DIR/'DE/roads_interasection.gpkg'

    # road_gdf = gpd.read_file(road_path, encoding='utf-8')

    # intersection = road_intersection(road_gdf)
    # intersection.set_crs(epsg=4326, inplace=True)

    # intersection_lines = intersection[intersection.geometry.geom_type == 'LineString']
    # intersection_points = intersection[intersection.geometry.geom_type == 'Point']

    # intersection_lines = intersection_lines.dissolve(by='id_list', as_index=False)
    # intersection_lines = braekdown_lines.breakdown_multi_lines(intersection_lines)

    # intersection:gpd.GeoDataFrame = pd.concat([intersection_lines, intersection_points])

    # intersection.to_file(output_path)

    road_intersection_country('KR')