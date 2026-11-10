import sys
import os
import json
import math
import argparse
import pathlib
import subprocess
import requests

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from constants import *

CITY_DIR = DATA_DIR / 'city'
OSM_DB_DIR = DATA_DIR / 'osm' / 'osm3s_db'


def query_osm_data_local_no_output(query: str, log: bool = False):
    cmd = [
        'osm3s_query',
        f'--db-dir={OSM_DB_DIR}',
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


def query_osm_data_remote(query: str, output_file: pathlib.Path, log: bool = False):
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


def _city_anchor_query(city_name: str, admin_level_pattern: str = '4|5') -> str:
    return f"""
    (
            node["place"~"city|town"]["name:en"="{city_name}"];
            node["place"~"city|town"]["name"="{city_name}"];
            node["place"~"city|town"]["name:zh"="{city_name}"];
            relation["boundary"="administrative"]["admin_level"~"{admin_level_pattern}"]["name:en"="{city_name}"];
            relation["boundary"="administrative"]["admin_level"~"{admin_level_pattern}"]["name"="{city_name}"];
            relation["boundary"="administrative"]["admin_level"~"{admin_level_pattern}"]["name:zh"="{city_name}"];

    )->.center_city;
    """


def _build_center_query(city_name: str, anchor_admin_level_pattern: str = '4|5') -> str:
    return f"""
    [out:json][timeout:120][maxsize:1073741824];
        {_city_anchor_query(city_name, admin_level_pattern=anchor_admin_level_pattern)}
        .center_city out tags center;
    """


def _build_candidates_query(
    center: dict,
    radius_m: int,
    admin_level_pattern: str = '4|5',
) -> str:
    lat = center['lat']
    lon = center['lon']
    return f"""
    [out:json][timeout:120][maxsize:1073741824];
    (
            relation(around:{radius_m},{lat},{lon})["boundary"="administrative"]["admin_level"~"{admin_level_pattern}"];
    );
        out tags center;
    """


def _haversine_m(lon1: float, lat1: float, lon2: float, lat2: float) -> float:
    r = 6371000.0
    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)
    d_phi = math.radians(lat2 - lat1)
    d_lambda = math.radians(lon2 - lon1)
    a = math.sin(d_phi / 2.0) ** 2 + math.cos(phi1) * math.cos(phi2) * math.sin(d_lambda / 2.0) ** 2
    return 2.0 * r * math.atan2(math.sqrt(a), math.sqrt(1.0 - a))


def _query_json_local_or_remote(query: str, log: bool = False) -> dict:
    ret, stdout = query_osm_data_local_no_output(query, log=log)
    if ret == 0 and stdout:
        try:
            return json.loads(stdout)
        except Exception:
            pass

    if log:
        print('⚠️ Local query failed or returned non-json, fallback to remote Overpass...')

    temp_dir = CITY_DIR / '_tmp'
    temp_dir.mkdir(parents=True, exist_ok=True)
    temp_json = temp_dir / 'nearby_city_query.json'
    status = query_osm_data_remote(query, temp_json, log=log)
    if status != 200 or (not temp_json.exists()):
        raise RuntimeError(f'Overpass remote query failed: status={status}')

    with open(temp_json, 'r', encoding='utf-8') as f:
        return json.load(f)


def _extract_center_and_candidates(payload: dict, city_name: str) -> tuple[dict, list[dict]]:
    elements = payload.get('elements', [])
    if len(elements) == 0:
        raise RuntimeError(f'No elements returned for city={city_name}')

    center = None
    candidates = []

    for elem in elements:
        if elem.get('type') not in  ('node','relation'):
            continue

        tags = elem.get('tags', {}) or {}
        name_en = tags.get('name:en')
        name_local = tags.get('name')
        name_zh = tags.get('name:zh')

        lat = elem.get('lat')
        lon = elem.get('lon')
        if lat is None or lon is None:
            center_obj = elem.get('center', {}) or {}
            lat = center_obj.get('lat')
            lon = center_obj.get('lon')

        item = {
            'id': elem.get('id'),
            'lat': lat,
            'lon': lon,
            'place': tags.get('place'),
            'admin_level': tags.get('admin_level'),
            'name': name_local,
            'name_en': name_en,
            'name_zh': name_zh,
        }

        matched_center = False
        city_norm = city_name.strip().lower()
        for name in [name_en, name_local, name_zh]:
            if name and str(name).strip().lower() == city_norm:
                matched_center = True
                break

        if matched_center and center is None:
            center = item
        candidates.append(item)

    if center is None:
        raise RuntimeError(f'Center city node not found in query result: {city_name}')

    return center, candidates


def _extract_candidates(payload: dict) -> list[dict]:
    candidates = []
    for elem in payload.get('elements', []):
        if elem.get('type') != 'relation':
            continue

        tags = elem.get('tags', {}) or {}
        lat = elem.get('lat')
        lon = elem.get('lon')
        if lat is None or lon is None:
            center_obj = elem.get('center', {}) or {}
            lat = center_obj.get('lat')
            lon = center_obj.get('lon')

        candidates.append({
            'id': elem.get('id'),
            'lat': lat,
            'lon': lon,
            'place': tags.get('place'),
            'admin_level': tags.get('admin_level'),
            'name': tags.get('name'),
            'name_en': tags.get('name:en'),
            'name_zh': tags.get('name:zh'),
        })
    return candidates


def _name_key(item: dict) -> str:
    return str(item.get('name_en') or item.get('name') or item.get('name_zh') or f"id_{item.get('id')}").strip().lower()


def _dedupe_and_rank(
    center: dict,
    candidates: list[dict],
    target_count: int,
    max_distance_m: int | None = None,
) -> list[dict]:
    center_lat = float(center['lat'])
    center_lon = float(center['lon'])
    center_names = {
        str(name).strip().lower()
        for name in [center.get('name_en'), center.get('name'), center.get('name_zh')]
        if name
    }

    dedup = {}
    for item in candidates:
        name_key = _name_key(item)
        if name_key == '':
            continue
        if name_key in center_names:
            continue

        lat = item.get('lat')
        lon = item.get('lon')
        if lat is None or lon is None:
            continue

        dist = _haversine_m(center_lon, center_lat, float(lon), float(lat))
        if max_distance_m is not None and dist > max_distance_m:
            continue

        item = item.copy()
        item['distance_m'] = dist

        old = dedup.get(name_key)
        if old is None or dist < old['distance_m']:
            dedup[name_key] = item

    result = [v for v in dedup.values() if v['distance_m'] > 10.0]
    result.sort(key=lambda x: x['distance_m'])
    return result[:target_count]


def discover_nearby_cities(
    center_city: str,
    target_count: int = 8,
    max_radius_m: int = 600000,
    radius_step_m: int = 50000,
    admin_level_pattern: str = '5',
    anchor_admin_level_pattern: str = '4|5',
    debug_names: list[str] | None = None,
    log: bool = False,
) -> dict:
    if target_count <= 0:
        raise ValueError('target_count must be > 0')

    center = None
    all_candidates = []
    center_payload = _query_json_local_or_remote(
        _build_center_query(center_city, anchor_admin_level_pattern=anchor_admin_level_pattern),
        log=log,
    )
    center, _ = _extract_center_and_candidates(center_payload, center_city)

    radius = radius_step_m
    while radius <= max_radius_m:
        query = _build_candidates_query(
            center,
            radius,
            admin_level_pattern=admin_level_pattern,
        )
        payload = _query_json_local_or_remote(query, log=log)
        elements = payload.get('elements', [])
        if len(elements) == 0:
            if log:
                print(f'Radius={radius}m -> no elements returned, skipped')
            radius += radius_step_m
            continue

        all_candidates.extend(_extract_candidates(payload))

        if log:
            found_so_far = _dedupe_and_rank(center, all_candidates, target_count, max_distance_m=max_radius_m)
            print(f'Radius={radius}m -> accumulated={len(found_so_far)} (target={target_count})')
            if debug_names:
                _print_debug_name_hits(center, all_candidates, debug_names)

        radius += radius_step_m

    found = _dedupe_and_rank(center, all_candidates, target_count, max_distance_m=max_radius_m)

    if log:
        print(f'Radius={max_radius_m}m -> found={len(found)} (target={target_count})')

    return {
        'center_city': center_city,
        'admin_level_pattern': admin_level_pattern,
        'center': center,
        'target_count': target_count,
        'search_radius_used_m': max_radius_m,
        'max_radius_m': max_radius_m,
        'radius_step_m': radius_step_m,
        'nearby_cities': found,
    }


def _print_debug_name_hits(center: dict, candidates: list[dict], debug_names: list[str]):
    center_lat = float(center['lat'])
    center_lon = float(center['lon'])
    normalized_debug_names = [name.strip().lower() for name in debug_names if name.strip()]
    if not normalized_debug_names:
        return

    for item in candidates:
        names = [item.get('name_en'), item.get('name'), item.get('name_zh')]
        names = [str(name) for name in names if name]
        lower_names = [name.lower() for name in names]
        if not any(any(debug_name in name for name in lower_names) for debug_name in normalized_debug_names):
            continue

        lat = item.get('lat')
        lon = item.get('lon')
        dist = None
        if lat is not None and lon is not None:
            dist = _haversine_m(center_lon, center_lat, float(lon), float(lat))
        print(
            'Debug candidate:',
            {
                'name': item.get('name'),
                'name_en': item.get('name_en'),
                'name_zh': item.get('name_zh'),
                'admin_level': item.get('admin_level'),
                'distance_m': None if dist is None else round(dist),
            },
        )


def save_discovery_result(result: dict, output_json: pathlib.Path):
    output_json.parent.mkdir(parents=True, exist_ok=True)
    with open(output_json, 'w', encoding='utf-8') as f:
        json.dump(result, f, ensure_ascii=False, indent=2)


def print_discovery_result(result: dict):
    print(f"Center city: {result['center_city']}")
    print(f"Search radius used: {result['search_radius_used_m']} m")
    print(f"Nearby city count: {len(result['nearby_cities'])}")
    for i, city in enumerate(result['nearby_cities'], start=1):
        name = city.get('name_en') or city.get('name') or city.get('name_zh') or str(city.get('id'))
        print(f"{i:02d}. {name} ({city.get('distance_m', 0.0):.0f} m)")


def build_overpass_turbo_query(
    city_name: str,
    max_radius_m: int = 200000,
    admin_level_pattern: str = '5',
    anchor_admin_level_pattern: str = '4|5',
) -> str:
    return f"""
[out:json][timeout:120];
(
    relation["boundary"="administrative"]["admin_level"~"{anchor_admin_level_pattern}"]["name:en"="{city_name}"];
    relation["boundary"="administrative"]["admin_level"~"{anchor_admin_level_pattern}"]["name"="{city_name}"];
    relation["boundary"="administrative"]["admin_level"~"{anchor_admin_level_pattern}"]["name:zh"="{city_name}"];
)->.center_city;
(
    relation(around:{max_radius_m},{{center_lat}},{{center_lon}})["boundary"="administrative"]["admin_level"~"{admin_level_pattern}"];
);
out center;
""".strip()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Discover nearby cities within a fixed radius from the center city.')
    parser.add_argument('city', nargs='?', default='Beijing', help=f'Center city name (default: )')  # type: ignore
    parser.add_argument('--count', type=int, default=8, help='Target nearby city count (default: 8)')
    parser.add_argument('--max-radius', type=int, default=600000, help='Fixed search radius in meters (default: 600000)')
    parser.add_argument('--radius-step', type=int, default=50000, help='Kept for old command compatibility; no longer expands search radius')
    parser.add_argument('--admin-levels', type=str, default='5', help='Administrative levels regex for nearby candidate relations (default: 5)')
    parser.add_argument('--anchor-admin-levels', type=str, default='4|5', help='Administrative levels regex used only to locate center city anchor (default: 4|5)')
    parser.add_argument('--debug-names', type=str, default='', help='Comma-separated city names to print when seen in raw candidates')
    parser.add_argument('--output', type=str, default=None, help='Output json path; default: <city>/nearby_cities_topN.json')
    parser.add_argument('--print-query', action='store_true', help='Print a directly runnable Overpass Turbo query and exit')
    parser.add_argument('--log', action='store_true', help='Enable verbose logs')
    args = parser.parse_args()

    if args.print_query:
        print(
            build_overpass_turbo_query(
                args.city,
                max_radius_m=args.max_radius,
                admin_level_pattern=args.admin_levels,
                anchor_admin_level_pattern=args.anchor_admin_levels,
            )
        )
        raise SystemExit(0)

    result = discover_nearby_cities(
        center_city=args.city,
        target_count=args.count,
        max_radius_m=args.max_radius,
        radius_step_m=args.radius_step,
        admin_level_pattern=args.admin_levels,
        anchor_admin_level_pattern=args.anchor_admin_levels,
        debug_names=[x.strip() for x in args.debug_names.split(',') if x.strip()],
        log=args.log,
    )

    default_output = CITY_DIR / args.city / f'nearby_cities_top{args.count}.json'
    output_path = pathlib.Path(args.output) if args.output else default_output

    save_discovery_result(result, output_path)
    print_discovery_result(result)
    print(f'Saved discovery result: {output_path}')
