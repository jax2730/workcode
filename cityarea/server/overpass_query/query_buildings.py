"""
Query and process all building data from OSM for a city.

Usage:
    python query_buildings.py <city_name> [radius] [--log]

Examples:
    python query_buildings.py Beijing 50000 --log
    python query_buildings.py Shanghai
"""

if __name__ == "__main__" or __package__ is None:
    from osm_util import *
else:
    from .osm_util import *

import sys
import os
import argparse
import pathlib

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from constants import *


CITY_DIR = DATA_DIR / "city"


def query_city_buildings(city_name: str, radius: int = 50000, log: bool = False):
    """Query all buildings in a city from OSM.

    Args:
        city_name: Name of the city (e.g., 'Beijing')
        radius: Search radius in meters (default: 50000)
        log: Whether to log execution details
    """
    if log:
        print(f"Querying buildings for {city_name} with radius {radius}m")

    # OSL Overpass query for all buildings
    # This query gets:
    # 1. City center point by name
    # 2. All building ways and areas within the radius
    query = f"""
    [out:xml][timeout:1800][maxsize:2147483648];
    (
      node[place~"city|town"]["name:en"="{city_name}"];
      node[place~"city|town"]["name"="{city_name}"];
      node[place~"city|town"]["name:zh"="{city_name}"];
    )->.city_center;
    (
        way(around.city_center:{radius})[building];
        relation(around.city_center:{radius})[building];
    );
    out body;
    >;
    out skel qt;
    """

    # Prepare output directory
    save_dir = CITY_DIR / city_name
    if not save_dir.exists():
        save_dir.mkdir(parents=True)
        if log:
            print(f"Created directory: {save_dir}")

    save_file = save_dir / "buildings.osm"

    # Query OSM data (local first, fallback to remote)
    if log:
        print(f"Querying OSM data...")
    local_code = query_osm_data_local(query, save_file, log)
    if local_code != 0 or not save_file.exists() or osm_result_empty(save_file):
        if log:
            print("⚠️  Local query failed or returned empty result, trying remote Overpass...")
        remote_code = query_osm_data_remote(query, save_file, log)
        if remote_code != 200 or not save_file.exists() or osm_result_empty(save_file):
            if log:
                print("⚠️  Full query (way+relation) still empty, fallback to way-only query...")
            way_only_query = f"""
            [out:xml][timeout:1800][maxsize:2147483648];
            (
              node[place~"city|town"]["name:en"="{city_name}"];
              node[place~"city|town"]["name"="{city_name}"];
              node[place~"city|town"]["name:zh"="{city_name}"];
            )->.city_center;
            way(around.city_center:{radius})[building];
            out body;
            >;
            out skel qt;
            """

            local_way_code = query_osm_data_local(way_only_query, save_file, log)
            if local_way_code != 0 or not save_file.exists() or osm_result_empty(save_file):
                remote_way_code = query_osm_data_remote(way_only_query, save_file, log)
                if remote_way_code != 200 or not save_file.exists() or osm_result_empty(save_file):
                    print("❌ No building OSM data returned (full query and way-only fallback both failed/empty).")
                    print(f"   OSM file: {save_file}")
                    if save_file.exists():
                        try:
                            preview = save_file.read_text(encoding='utf-8', errors='ignore')[:1200]
                            if preview.strip():
                                print("--- OSM response preview (first 1200 chars) ---")
                                print(preview)
                                print("--- end preview ---")
                        except Exception as read_error:
                            print(f"   Failed to read response preview: {read_error}")
                    return

    # Convert OSM to GPKG with raw OSM building types
    output_file = save_dir / "All_Building.gpkg"
    if log:
        print(f"Converting OSM to GPKG: {output_file}")

    success = convert_building_osm_2_gpkg(save_file, output_file, log=log)

    if not success or not output_file.exists():
        print("❌ Building conversion finished without output file.")
        print(f"   Expected: {output_file}")
        return

    if log:
        print(f"Done! Building data saved to: {output_file}")
        print("Use QGIS to open this GPKG and select sublayers by building type.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Query buildings from OSM and save as All_Building.gpkg'
    )
    parser.add_argument('city', nargs='?', default=DEFAULT_CITY, help=f'City name (default: {DEFAULT_CITY})')  # type: ignore
    parser.add_argument('--radius', type=int, default=50000,
                        help='Search radius in meters (default: 50000)')
    parser.add_argument('--log', action='store_true', help='Enable logging')

    args = parser.parse_args()

    query_city_buildings(args.city, args.radius, args.log)
