import sys
import os
import json
import argparse
import pathlib
import subprocess
import importlib.util
import types
from concurrent.futures import ThreadPoolExecutor, as_completed

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
SERVER_QUERY_DIR = SCRIPT_DIR.parent
SERVER_DIR = SERVER_QUERY_DIR.parent
sys.path.append(str(SERVER_QUERY_DIR))
sys.path.append(str(SERVER_DIR))
from constants import *

CITY_DIR = DATA_DIR / 'city'


def _python_executable(repo_root: pathlib.Path) -> str:
    candidates = [
        repo_root / '.venv' / 'bin' / 'python',
        repo_root / '.venv' / 'Scripts' / 'python.exe',
    ]
    for candidate in candidates:
        if candidate.exists():
            return str(candidate)
    return sys.executable


def _run_cmd(cmd: list[str], cwd: pathlib.Path, dry_run: bool = False):
    print('>>', ' '.join(cmd))
    if dry_run:
        return 0
    result = subprocess.run(cmd, cwd=str(cwd))
    return result.returncode


def _query_osm_data_local_no_output(query: str, log: bool = False):
    cmd = [
        'osm3s_query',
        f"--db-dir={DATA_DIR / 'osm' / 'osm3s_db'}",
    ]
    process = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout_data, stderr_data = process.communicate(query.encode('utf-8'))
    if process.returncode != 0:
        if log:
            print(f"Failed to query OSM data: {query}")
            print(f"    stdout: {stdout_data.decode('utf-8')}")
            print(f"    stderr: {stderr_data.decode('utf-8')}")
    else:
        if log:
            print(f"Query OSM data successful: {query}")
    return process.returncode, stdout_data.decode('utf-8')


def _query_osm_data_remote(query: str, output_file: pathlib.Path, log: bool = False):
    import requests

    response = requests.get(OVERPASS_URL, params={'data': query}, headers=HTTP_HEADERS)
    if response.status_code != 200:
        if log:
            print(f"Failed to query OSM data: {query}")
            print(f"    status code: {response.status_code}")
            print(f"    response: {response.text}")
    else:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(response.text)
        if log:
            print(f"Query OSM data successful: {query}")
    return response.status_code


def _load_query_nearby_cities_module():
    module_path = SERVER_QUERY_DIR / 'overpass_query' / 'query_nearby_cities.py'
    spec = importlib.util.spec_from_file_location('query_nearby_cities_standalone', module_path)
    module = importlib.util.module_from_spec(spec)

    osm_util_stub = types.ModuleType('osm_util')
    osm_util_stub.query_osm_data_local_no_output = _query_osm_data_local_no_output
    osm_util_stub.query_osm_data_remote = _query_osm_data_remote

    previous_osm_util = sys.modules.get('osm_util')
    sys.modules['osm_util'] = osm_util_stub
    try:
        spec.loader.exec_module(module)
    finally:
        if previous_osm_util is None:
            sys.modules.pop('osm_util', None)
        else:
            sys.modules['osm_util'] = previous_osm_util
    return module


def _nonempty_file(path: pathlib.Path) -> bool:
    return path.exists() and path.stat().st_size > 0


def missing_city_outputs(city_name: str) -> set[str]:
    city_dir = CITY_DIR / city_name
    missing = set()
    if not _nonempty_file(city_dir / 'city_street_block.json'):
        missing.add('street_block')
    if not _nonempty_file(city_dir / 'poi_pos.json'):
        missing.add('poi_pos')
    return missing


def build_city_commands(
    city_name: str,
    py: str,
    radius: int,
    simplify_tolerance: float,
    skip_landmark: bool,
    resume_missing: bool,
) -> list[list[str]]:
    missing = missing_city_outputs(city_name) if resume_missing else {'street_block', 'poi_pos'}
    commands = []

    if 'street_block' in missing:
        commands.append([py, 'city_process/query_city_fast.py', '-c', city_name, '-r', str(radius)])
        commands.append([py, 'city_process/test_simplify_road_timed.py', city_name, '-s', '1', '-t', str(simplify_tolerance)])

    if 'poi_pos' in missing:
        commands.append([py, 'city_process/query_city.py', '-c', city_name, '-r', str(radius), '--poi'])

    if 'street_block' in missing:
        if not skip_landmark:
            commands.append([py, 'city_process/query_landmark.py', '-c', city_name, '-f', 'landmarks.json'])
            commands.append([py, 'city_process/test_simplify_road_timed.py', city_name, '-s', '2', '-t', str(simplify_tolerance)])
        commands.append([py, 'city_process/street_block_process.py', city_name])

    return commands


def _run_city_item(item: dict, args, repo_root: pathlib.Path) -> bool:
    city_name = _city_name_for_pipeline(item)
    return run_city_to_street_block(
        city_name=city_name,
        radius=args.query_radius,
        simplify_tolerance=args.simplify_tolerance,
        repo_root=repo_root,
        skip_landmark=args.skip_landmark,
        resume_missing=args.resume_missing,
        dry_run=args.dry_run,
    )


def _city_name_for_pipeline(city_item: dict) -> str:
    name = city_item.get('name_en') or city_item.get('name') or city_item.get('name_zh')
    if not name:
        raise ValueError(f'Invalid city item without name: {city_item}')
    return str(name)


def _write_json_if_missing(path: pathlib.Path, data):
    if not path.exists():
        with open(path, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)


def _write_empty_landmark_gpkg_if_missing(path: pathlib.Path):
    if path.exists():
        return

    try:
        import geopandas as gpd
    except ImportError:
        print(f'Warning: geopandas is not installed; skip creating empty landmark gpkg: {path}')
        return

    path.parent.mkdir(parents=True, exist_ok=True)
    gdf = gpd.GeoDataFrame(
        columns=['name:local', 'name:en', 'name:zh', 'geometry'],
        geometry='geometry',
        crs='EPSG:3857',
    )
    gdf.to_file(path, layer='landmarks', driver='GPKG')


def _ensure_landmark_assets(city_name: str):
    city_dir = CITY_DIR / city_name
    city_dir.mkdir(parents=True, exist_ok=True)
    _write_json_if_missing(city_dir / 'landmarks.json', {})
    _write_json_if_missing(city_dir / 'landmark_list.json', [])
    _write_json_if_missing(city_dir / 'landmark_info.json', [])
    _write_empty_landmark_gpkg_if_missing(city_dir / 'landmarks' / 'landmarks.gpkg')
    _write_empty_landmark_gpkg_if_missing(city_dir / 'landmark_polygons.gpkg')


def build_run_list(
    center_city: str,
    discovered_city_items: list[dict],
    include_center: bool,
    city_name: str | None,
) -> list[dict]:
    if city_name:
        return [{'name_en': city_name}]

    run_list = []
    if include_center:
        run_list.append({'name_en': center_city})
    run_list.extend(discovered_city_items)
    return run_list


def discover_and_save_nearby_cities(args) -> list[dict]:
    query_nearby_cities = _load_query_nearby_cities_module()

    result = query_nearby_cities.discover_nearby_cities(
        center_city=args.city,
        target_count=args.count,
        max_radius_m=args.max_radius,
        radius_step_m=args.radius_step,
        admin_level_pattern=args.admin_levels,
        anchor_admin_level_pattern=args.anchor_admin_levels,
        log=args.log,
    )

    out_json = CITY_DIR / args.city / f'nearby_cities_top{args.count}.json'
    query_nearby_cities.save_discovery_result(result, out_json)
    print(f'Nearby city list saved: {out_json}')
    return result['nearby_cities']


def run_city_to_street_block(
    city_name: str,
    radius: int,
    simplify_tolerance: float,
    repo_root: pathlib.Path,
    skip_landmark: bool,
    resume_missing: bool,
    dry_run: bool,
) -> bool:
    py = _python_executable(repo_root)

    commands = build_city_commands(
        city_name=city_name,
        py=py,
        radius=radius,
        simplify_tolerance=simplify_tolerance,
        skip_landmark=skip_landmark,
        resume_missing=resume_missing,
    )
    if resume_missing and not commands:
        print(f'Skip city={city_name}: city_street_block.json and poi_pos.json already exist')
        return True

    if any('query_landmark.py' in part for cmd in commands for part in cmd):
        _ensure_landmark_assets(city_name)

    for cmd in commands:
        code = _run_cmd(cmd, cwd=repo_root, dry_run=dry_run)
        if code != 0:
            print(f'Failed for city={city_name}, cmd={cmd}')
            return False

    print(f'Finished city_street_block.json for city={city_name}')
    return True


def main():
    default_city = globals().get('DEFAULT_CITY', 'Beijing')
    parser = argparse.ArgumentParser(description='Expand from center city to nearby cities and run full pipeline to city_street_block.json')
    parser.add_argument('city', nargs='?', default=default_city, help=f'Center city (default: {default_city})')
    parser.add_argument('--count', type=int, default=8, help='Nearby city count target (default: 8)')
    parser.add_argument('--city-name', type=str, default=None, help='Run one explicit city and skip nearby discovery')
    parser.add_argument('--max-radius', type=int, default=600000, help='Max radius used by discovery in meters (default: 600000)')
    parser.add_argument('--radius-step', type=int, default=50000, help='Discovery radius step in meters (default: 50000)')
    parser.add_argument('--admin-levels', type=str, default='5', help='Administrative levels regex for nearby candidate relations (default: 5)')
    parser.add_argument('--anchor-admin-levels', type=str, default='4|5', help='Administrative levels regex used only to locate center city anchor (default: 4|5)')
    parser.add_argument('--query-radius', type=int, default=25000, help='Per-city query radius for query_city.py (default: 25000)')
    parser.add_argument('--simplify-tolerance', type=float, default=50.0, help='Road simplify tolerance (default: 50.0)')
    parser.add_argument('--include-center', action='store_true', help='Also run the center city in pipeline')
    parser.add_argument('--skip-landmark', action='store_true', help='Skip landmark query + simplify step2')
    parser.add_argument('--workers', type=int, default=1, help='Number of cities to process in parallel (default: 1)')
    parser.add_argument('--resume-missing', action='store_true', help='Skip complete cities and run only missing city_street_block.json / poi_pos.json outputs')
    parser.add_argument('--dry-run', action='store_true', help='Print commands only, do not execute')
    parser.add_argument('--log', action='store_true', help='Enable discovery logs')
    args = parser.parse_args()

    city_items = [] if args.city_name else discover_and_save_nearby_cities(args)
    run_list = build_run_list(
        center_city=args.city,
        discovered_city_items=city_items,
        include_center=args.include_center,
        city_name=args.city_name,
    )

    repo_root = pathlib.Path(__file__).resolve().parents[1]

    succeeded_cities = []
    failed_cities = []
    workers = max(1, args.workers)
    if workers == 1:
        for item in run_list:
            city_name = _city_name_for_pipeline(item)
            try:
                ok = _run_city_item(item, args, repo_root)
            except Exception as e:
                print(f'Failed for city={city_name}, error={e}')
                ok = False
            if ok:
                succeeded_cities.append(city_name)
            else:
                failed_cities.append(city_name)
    else:
        print(f'Processing {len(run_list)} cities with workers={workers}')
        with ThreadPoolExecutor(max_workers=workers) as executor:
            future_to_city = {
                executor.submit(_run_city_item, item, args, repo_root): _city_name_for_pipeline(item)
                for item in run_list
            }
            for future in as_completed(future_to_city):
                city_name = future_to_city[future]
                try:
                    ok = future.result()
                except Exception as e:
                    print(f'Failed for city={city_name}, error={e}')
                    ok = False
                if ok:
                    succeeded_cities.append(city_name)
                else:
                    failed_cities.append(city_name)

    print(f'Completed: {len(succeeded_cities)}/{len(run_list)} cities')
    if succeeded_cities:
        print(f"Succeeded cities: {', '.join(succeeded_cities)}")
    if failed_cities:
        print(f"Failed cities: {', '.join(failed_cities)}")


if __name__ == '__main__':
    main()
