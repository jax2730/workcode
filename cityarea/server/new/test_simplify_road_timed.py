import argparse
import pathlib
import sys
import time

sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
from test_simplify_road import (
    add_landmark_surrounding_road,
    buffer_road_by_type,
    buffered_road_2_raster,
    create_city_sector_gdf,
    extend_major_road_leaf,
    gen_street_block,
    match_segment_to_road,
    merge_small_street_block,
    rater_skeletonize,
    remove_close_edge,
    remove_degree_2_node,
    remove_leaf_node,
    road_util,
    update_or_create_LandmarkTask,
    update_or_create_SimplifyTask,
)

import geopandas as gpd
import networkx as nx
import numpy as np
import pandas as pd
import shapely.geometry as geom
import shapely.ops as ops


CITY_DIR = DATA_DIR / 'city'


def _run_stage(name: str, fn, *args, **kwargs):
    start = time.perf_counter()
    print(f'-- start {name}')
    result = fn(*args, **kwargs)
    elapsed = time.perf_counter() - start
    print(f'-- done  {name}: {elapsed:.2f}s')
    return result


def _roads_covered_by_polygon(road_gdf: gpd.GeoDataFrame, polygon) -> gpd.GeoDataFrame:
    try:
        candidate_idx = road_gdf.sindex.query(polygon, predicate='intersects')
        candidates = road_gdf.iloc[candidate_idx]
    except Exception:
        candidates = road_gdf

    if len(candidates) == 0:
        return candidates
    return candidates[candidates.geometry.covered_by(polygon)]


def _shape_width(shape: geom.Polygon) -> float:
    if not isinstance(shape, geom.Polygon):
        return float('nan')
    obb = shape.minimum_rotated_rectangle
    width = geom.LineString((obb.exterior.coords[0], obb.exterior.coords[1])).length
    height = geom.LineString((obb.exterior.coords[1], obb.exterior.coords[2])).length
    return min(width, height)


def create_city_sector_gdf_limited(
    road_gdf: gpd.GeoDataFrame,
    road_level: int,
    min_width: float = 1000,
    max_merge_steps: int = 300,
) -> gpd.GeoDataFrame:
    major_road_gdf = road_gdf[road_gdf['highway_code'].isin(range(road_level + 1))]
    sector_gdf = gpd.GeoDataFrame(columns=['geometry', 'name'])
    if len(major_road_gdf) == 0:
        return gpd.GeoDataFrame(columns=['geometry', 'name', 'area'])

    major_road_union = major_road_gdf.geometry.union_all()
    sector_polygons = list(ops.polygonize(major_road_union))
    print(f'         sector_gdf_limited road_level={road_level}, major_roads={len(major_road_gdf)}, polygons={len(sector_polygons)}')
    if len(sector_polygons) < 2:
        return gpd.GeoDataFrame(columns=['geometry', 'name', 'area'])

    rows = [{'geometry': polygon, 'name': f'sector_{idx + 1}'} for idx, polygon in enumerate(sector_polygons)]
    sector_gdf = gpd.GeoDataFrame(rows, crs=road_gdf.crs)
    sector_gdf['width'] = sector_gdf.geometry.apply(_shape_width)
    small_sector_list = sector_gdf[sector_gdf['width'] < min_width].index.to_list()

    merge_steps = 0
    while len(small_sector_list) > 0:
        merge_steps += 1
        if merge_steps > max_merge_steps:
            print(f'         stop small sector merge: max_merge_steps={max_merge_steps}, remaining={len(small_sector_list)}')
            break

        small_id = small_sector_list.pop(0)
        if small_id not in sector_gdf.index:
            continue

        row_geom = sector_gdf.loc[small_id].geometry
        try:
            neighbor_idx = sector_gdf.sindex.query(row_geom, predicate='intersects')
            neighbors = sector_gdf.iloc[neighbor_idx]
        except Exception:
            neighbors = sector_gdf[sector_gdf.geometry.intersects(row_geom)]
        neighbors = neighbors[neighbors.index != small_id]

        merged_geometry = None
        merged_index = None
        max_shared_boundary = 0.0
        for j, neighbor in neighbors.iterrows():
            new_geometry = ops.unary_union([row_geom, neighbor.geometry])
            if new_geometry.geom_type != 'Polygon':
                continue
            shared_boundary = row_geom.boundary.intersection(neighbor.geometry.boundary).length
            if shared_boundary > max_shared_boundary:
                max_shared_boundary = shared_boundary
                merged_geometry = new_geometry
                merged_index = j

        if merged_geometry is not None:
            sector_gdf.loc[merged_index, 'geometry'] = merged_geometry
            sector_gdf.drop(index=small_id, inplace=True)
            if _shape_width(merged_geometry) < min_width:
                small_sector_list.append(merged_index)

    sector_gdf['have_hole'] = sector_gdf.geometry.apply(lambda x: len(x.interiors) > 0)
    sector_w_hole_list = sector_gdf[sector_gdf['have_hole']].index.to_list()
    for i in sector_w_hole_list:
        if i in sector_gdf.index:
            row_geom = sector_gdf.loc[i].geometry
            geom_no_hole = geom.Polygon(row_geom.exterior)
            new_row = sector_gdf.loc[i].to_dict()
            new_row['geometry'] = geom_no_hole
            sector_in_geom = sector_gdf[sector_gdf.geometry.apply(lambda x: geom_no_hole.contains(x))].index.to_list()
            sector_gdf.drop(index=sector_in_geom, inplace=True)
            sector_gdf.loc[i] = new_row

    if len(sector_gdf) < 2:
        return gpd.GeoDataFrame(columns=['geometry', 'name', 'area'])

    sector_gdf.set_crs(road_gdf.crs, inplace=True)
    sector_gdf['area'] = sector_gdf.geometry.area
    return sector_gdf


def extend_leaf_road_limited(
    road_gdf: gpd.GeoDataFrame,
    road_level: int,
    min_leaf_length: float,
    max_iterations: int = 500,
) -> gpd.GeoDataFrame:
    road_gdf = road_gdf.copy()
    major_road_gdf = road_gdf[road_gdf['highway_code'].isin(range(road_level + 1))]
    major_road_network = nx.Graph()
    for i, row in major_road_gdf.iterrows():
        geometry = row['geometry']
        major_road_network.add_edge(geometry.coords[0], geometry.coords[-1], **{'id': i, 'length': geometry.length})

    leaf_nodes = [node for node, degree in major_road_network.degree() if degree == 1]
    leaf_node_keep = set()
    for node in leaf_nodes:
        connected_roads = road_gdf[road_gdf.geometry.intersects(geom.Point(node))]
        if len(connected_roads) == 1:
            leaf_node_keep.add(node)
    leaf_nodes = [node for node in leaf_nodes if node not in leaf_node_keep]

    iterations = 0
    seen_states = set()
    while len(leaf_nodes) > 0:
        iterations += 1
        state = tuple(sorted((round(node[0], 3), round(node[1], 3)) for node in leaf_nodes))
        if iterations > max_iterations or state in seen_states:
            print(
                f'         stop extend_leaf_road: iterations={iterations}, '
                f'leaf_nodes={len(leaf_nodes)}, repeated_state={state in seen_states}'
            )
            break
        seen_states.add(state)

        node = leaf_nodes.pop(0)
        edges = list(major_road_network.edges(node, data=True))
        if len(edges) == 0:
            continue

        s, e, data = edges[0]
        last_geom = road_gdf.loc[data['id']].geometry
        if node == last_geom.coords[0]:
            last_dir = np.array(last_geom.coords[0], dtype=np.float64) - np.array(last_geom.coords[1], dtype=np.float64)
        else:
            last_dir = np.array(last_geom.coords[-1], dtype=np.float64) - np.array(last_geom.coords[-2], dtype=np.float64)
        norm = np.linalg.norm(last_dir)
        if norm == 0:
            leaf_node_keep.add(node)
            continue
        last_dir = last_dir / norm

        connected_roads = road_gdf[road_gdf.geometry.intersects(geom.Point(node))]
        connected_roads = connected_roads[connected_roads.index != data['id']]
        connected_to_network = False
        max_dot_product = -1
        row_to_insert = None
        for j, connected_road in connected_roads.iterrows():
            connected_geom = connected_road.geometry
            start = connected_geom.coords[0]
            end = connected_geom.coords[-1]
            if node == start:
                cur_dir = np.array(connected_geom.coords[1], dtype=np.float64) - np.array(start, dtype=np.float64)
                new_leaf_node = end
            else:
                cur_dir = np.array(connected_geom.coords[-2], dtype=np.float64) - np.array(end, dtype=np.float64)
                new_leaf_node = start
            norm = np.linalg.norm(cur_dir)
            if norm == 0:
                continue
            cur_dir = cur_dir / norm
            dot_product = np.dot(last_dir, cur_dir)
            if new_leaf_node in major_road_network.nodes:
                if (not connected_to_network) or max_dot_product < dot_product:
                    connected_to_network = True
                    max_dot_product = dot_product
                    row_to_insert = j
            elif not connected_to_network and max_dot_product < dot_product:
                max_dot_product = dot_product
                row_to_insert = j

        if row_to_insert is not None:
            if data['length'] < min_leaf_length:
                new_row_geom = road_gdf.loc[data['id']].geometry
                new_row = road_gdf.loc[row_to_insert].to_dict()
                new_row['id'] = data['id']
                new_row['geometry'] = new_row_geom
                road_gdf.loc[data['id']] = new_row
                if major_road_network.has_edge(s, e):
                    major_road_network.remove_edge(s, e)
            else:
                new_row_geom = road_gdf.loc[row_to_insert].geometry
                new_row = road_gdf.loc[data['id']].to_dict()
                new_row['id'] = row_to_insert
                new_row['geometry'] = new_row_geom
                road_gdf.loc[row_to_insert] = new_row
                major_road_network.add_edge(
                    new_row_geom.coords[0],
                    new_row_geom.coords[-1],
                    **{'id': row_to_insert, 'length': new_row_geom.length + data['length']},
                )
        else:
            leaf_node_keep.add(node)

        leaf_nodes = [node for node, degree in major_road_network.degree() if degree == 1]
        leaf_nodes = [node for node in leaf_nodes if node not in leaf_node_keep]

    return road_gdf


def create_city_sector_timed(city_name: str, max_levels: int = 8):
    save_dir = CITY_DIR / city_name
    road_path = save_dir / 'city_road_no_leaf_extended.gpkg'
    sector_path = save_dir / 'city_sector.gpkg'
    if sector_path.exists():
        sector_path.unlink()

    road_gdf = gpd.read_file(road_path).to_crs(epsg=3857)
    print(f'   create_city_sector input roads={len(road_gdf)}')

    sector_gdf = _run_stage('create_city_sector.lvl_0.create_gdf', create_city_sector_gdf_limited, road_gdf, 2)
    sector_lvl = 0
    sector_gdf.to_file(sector_path, driver='GPKG', layer=f'lvl_{sector_lvl}')
    sector_gdf.reset_index(inplace=True, drop=True)
    sector_large_gdf = sector_gdf[sector_gdf['area'] > 10**7]
    print(
        f'   lvl={sector_lvl}, sectors={len(sector_gdf)}, '
        f'large={len(sector_large_gdf)}, max_area={sector_gdf["area"].max() if len(sector_gdf) else 0:.0f}'
    )

    while len(sector_large_gdf) > 0:
        sector_lvl += 1
        if sector_lvl > max_levels:
            print(f'   stop create_city_sector: max_levels={max_levels}, remaining_large={len(sector_large_gdf)}')
            break

        lvl_start = time.perf_counter()
        sector_next_gdf = gpd.GeoDataFrame(columns=['geometry', 'name'], crs=road_gdf.crs)
        for i, row in sector_large_gdf.iterrows():
            row_start = time.perf_counter()
            road_in_sector = _roads_covered_by_polygon(road_gdf, row.geometry)
            print(
                f'   lvl={sector_lvl}, parent={i}, parent_area={row.geometry.area:.0f}, '
                f'roads={len(road_in_sector)}'
            )
            if len(road_in_sector) > 0:
                extend_start = time.perf_counter()
                print(f'      start extend_leaf_road parent={i}, road_level={3 + sector_lvl}')
                road_in_sector = extend_leaf_road_limited(road_in_sector, 3 + sector_lvl, 450)
                print(f'      done  extend_leaf_road parent={i}: {time.perf_counter() - extend_start:.2f}s')

                create_start = time.perf_counter()
                print(f'      start create_city_sector_gdf parent={i}, roads={len(road_in_sector)}')
                sub_sector_gdf = create_city_sector_gdf_limited(road_in_sector, 3 + sector_lvl)
                print(f'      done  create_city_sector_gdf parent={i}: {time.perf_counter() - create_start:.2f}s')
                if sub_sector_gdf is not None and len(sub_sector_gdf) > 0:
                    sub_sector_gdf['parent_id'] = i
                    sector_next_gdf = pd.concat([sector_next_gdf, sub_sector_gdf])
                    print(
                        f'   lvl={sector_lvl}, parent={i}, sub_sectors={len(sub_sector_gdf)}, '
                        f'max_sub_area={sub_sector_gdf["area"].max():.0f}, '
                        f'time={time.perf_counter() - row_start:.2f}s'
                    )
                else:
                    print(f'   lvl={sector_lvl}, parent={i}, no sub sectors, time={time.perf_counter() - row_start:.2f}s')

        sector_next_gdf.set_crs(road_gdf.crs, inplace=True)
        if len(sector_next_gdf) > 0:
            sector_next_gdf['area'] = sector_next_gdf.geometry.area
            sector_next_gdf.reset_index(inplace=True, drop=True)
            sector_next_gdf.to_file(sector_path, driver='GPKG', layer=f'lvl_{sector_lvl}')
            max_area = sector_next_gdf['area'].max()
        else:
            sector_next_gdf['area'] = []
            max_area = 0

        sector_large_gdf = sector_next_gdf[sector_next_gdf['area'] > 10**7]
        print(
            f'   lvl={sector_lvl}, sectors={len(sector_next_gdf)}, '
            f'large={len(sector_large_gdf)}, max_area={max_area:.0f}, '
            f'time={time.perf_counter() - lvl_start:.2f}s'
        )


def city_2_simple_road_timed(city_name: str, simplify_tolerance: float = 50.0):
    save_dir = CITY_DIR / city_name

    _run_stage('buffer_road_by_type', buffer_road_by_type, city_name)
    _run_stage('buffered_road_2_raster', buffered_road_2_raster, city_name)
    _run_stage('rater_skeletonize', rater_skeletonize, city_name)
    _run_stage('remove_close_edge', remove_close_edge, city_name, simplify_tolerance * 1.5)
    _run_stage('match_segment_to_road', match_segment_to_road, city_name)

    simplified_road_gpkg = save_dir / 'city_road_simplified.gpkg'
    _run_stage(
        'dP_simplification',
        road_util.dP_simplification,
        save_dir / 'city_matched.gpkg',
        simplified_road_gpkg,
        simplify_tolerance,
    )
    _run_stage('remove_leaf_node', remove_leaf_node, simplified_road_gpkg, save_dir / 'city_road_no_leaf.gpkg')
    _run_stage('extend_major_road_leaf', extend_major_road_leaf, city_name, 600)
    _run_stage('create_city_sector', create_city_sector_timed, city_name)
    _run_stage('gen_street_block', gen_street_block, city_name)
    _run_stage('merge_small_street_block', merge_small_street_block, city_name)


def city_add_landmark_timed(city_name: str):
    _run_stage('add_landmark_surrounding_road', add_landmark_surrounding_road, city_name)
    _run_stage('remove_degree_2_node', remove_degree_2_node, city_name)


if __name__ == '__main__':
    print(f'Running timed simplify file: {pathlib.Path(__file__).resolve()}')
    parser = argparse.ArgumentParser(description='Timed test entry for simplify road network')
    parser.add_argument('city_name', type=str, help='Name of the city')
    parser.add_argument('-s', '--step', type=int, default=1, help='Simplify step')
    parser.add_argument('-t', '--simplify_tolerance', type=float, default=50.0, help='Simplify tolerance')
    args = parser.parse_args()

    start = time.perf_counter()
    try:
        if args.step == 1:
            city_2_simple_road_timed(args.city_name, args.simplify_tolerance)
            update_or_create_SimplifyTask(
                args.city_name,
                args.simplify_tolerance,
                CITY_DIR / args.city_name / 'city_road_out_block.gpkg',
            )
        else:
            city_add_landmark_timed(args.city_name)
            update_or_create_LandmarkTask(
                args.city_name,
                args.simplify_tolerance,
                CITY_DIR / args.city_name / 'city_road_w_landmark_degree2.gpkg',
            )
    except Exception as e:
        print(e)
        raise
    finally:
        elapsed = time.perf_counter() - start
        print(f'-- total simplify step {args.step}: {elapsed:.2f}s')
