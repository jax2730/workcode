if __name__ == "__main__" or __package__ is None:
    from osm_util import *
else:
    from .osm_util import *

import sys
import os
import argparse
import json
import subprocess
import pathlib
from collections import Counter
from collections import defaultdict
import xml.etree.ElementTree as ET

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from constants import *


CITY_DIR = DATA_DIR / "city"


def _print_report(report: dict, top_n: int, top_values: int, show_key: str | None = None):
    city_name = report.get("city", "Unknown")
    radius = report.get("radius", "Unknown")
    surface_only = report.get("surface_only", True)
    element_count = report.get("elements_scanned", 0)
    total_key_categories = report.get("total_key_categories", 0)
    focus_present = report.get("focus_keys_present", [])
    focus_missing = report.get("focus_keys_missing", [])
    focus_key_value_stats = report.get("focus_key_value_stats", {})

    print(f"✅ Loaded tag key category report for: {city_name}")
    print(f"   Radius: {radius}")
    print(f"   Surface-only mode: {surface_only}")
    print(f"   Elements scanned: {element_count}")
    print(f"   Total key categories: {total_key_categories}")
    print(f"   Focus keys present ({len(focus_present)}): {', '.join(focus_present) if focus_present else 'None'}")
    if focus_missing:
        print(f"   Focus keys missing ({len(focus_missing)}): {', '.join(focus_missing)}")

    if show_key:
        if show_key not in focus_key_value_stats:
            print(f"\n⚠️  Key '{show_key}' not found in focus_key_value_stats")
            return

        key_stats = focus_key_value_stats[show_key]
        distinct_count = key_stats.get("distinct_value_categories", 0)
        top_list = key_stats.get("top_values", [])
        print(f"\n[{show_key}] distinct value categories: {distinct_count}")
        if len(top_list) == 0:
            print("  (no values)")
            return
        for item in top_list[:top_values]:
            print(f"  {item['value']}: {item['count']}")
        return

    print("\nFocus key -> distinct value categories:")
    for key in focus_present:
        distinct_count = focus_key_value_stats.get(key, {}).get("distinct_value_categories", 0)
        print(f"  {key}: {distinct_count}")

    print(f"\nTop values per focus key (top {top_values}):")
    for key in focus_present:
        print(f"  [{key}]")
        top_list = focus_key_value_stats.get(key, {}).get("top_values", [])
        if len(top_list) == 0:
            print("    (no values)")
            continue
        for item in top_list[:top_values]:
            print(f"    {item['value']}: {item['count']}")

    top_keys = report.get("top_keys", [])
    print(f"\nTop {min(top_n, len(top_keys))} key categories:")
    for item in top_keys[:top_n]:
        print(f"  {item['key']}: {item['count']}")


def show_report_from_json(
    city_name: str,
    report_path: str | None,
    top_n: int,
    top_values: int,
    show_key: str | None = None,
):
    if report_path:
        path = pathlib.Path(report_path)
    else:
        path = CITY_DIR / city_name / "tag_key_categories_report.json"

    if not path.exists():
        print(f"❌ Report file not found: {path}")
        return

    with open(path, "r", encoding="utf-8") as f:
        report = json.load(f)

    _print_report(report, top_n=top_n, top_values=top_values, show_key=show_key)


def _build_city_query(city_name: str, radius: int) -> str:
    return f"""
    [out:xml][timeout:1800][maxsize:2147483648];
    (
      node[place~"city|town"]["name:en"="{city_name}"];
      node[place~"city|town"]["name"="{city_name}"];
      node[place~"city|town"]["name:zh"="{city_name}"];
    )->.city_center;
    nwr(around.city_center:{radius});
    out body qt;
    """


def _element_tags(element: ET.Element) -> dict[str, str]:
    tags = {}
    for tag in element.findall("tag"):
        k = tag.get("k")
        v = tag.get("v")
        if k and v is not None:
            tags[k] = v
    return tags


def _is_surface_feature(element: ET.Element, tags: dict[str, str]) -> bool:
    """Return True for area-like features: closed ways and multipolygon-type relations."""
    if element.tag == "node":
        return False

    if element.tag == "way":
        # Explicit area tag
        if tags.get("area") == "yes":
            return True

        # Closed way check: first nd ref == last nd ref
        nd_refs = [nd.get("ref") for nd in element.findall("nd") if nd.get("ref")]
        return len(nd_refs) >= 4 and nd_refs[0] == nd_refs[-1]

    if element.tag == "relation":
        relation_type = tags.get("type", "")
        return relation_type in ("multipolygon", "boundary")

    return False


def _query_osm_data_local_raw(query: str, output_file: pathlib.Path, log: bool = False) -> int:
    """Run local overpass query and write raw XML directly to file (no in-memory sort)."""
    if OSM3S_QUERY_EXE is None:
        raise FileNotFoundError('osm3s_query is not found. Set OSM3S_QUERY_EXE or add it to PATH.')
    if not OSM_DB_DIR.exists():
        raise FileNotFoundError(f'OSM_DB_DIR does not exist: {OSM_DB_DIR}')

    cmd = [OSM3S_QUERY_EXE, f'--db-dir={OSM_DB_DIR}']
    if log:
        print(f"ℹ️  Local OSM query exe: {OSM3S_QUERY_EXE}")
        print(f"ℹ️  Local OSM db: {OSM_DB_DIR}")

    with open(output_file, 'wb') as fout:
        process = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE,
            stdout=fout,
            stderr=subprocess.PIPE,
        )
        _, stderr_data = process.communicate(query.encode('utf-8'))

    if process.returncode != 0 and log:
        print("❌  Local OSM raw query failed")
        print(stderr_data.decode('utf-8', errors='ignore'))

    return process.returncode


def _query_city_osm(city_name: str, radius: int, save_file: pathlib.Path, log: bool) -> bool:
    query = _build_city_query(city_name, radius)

    # Use raw local query to avoid heavy XML sort and huge memory usage.
    local_code = _query_osm_data_local_raw(query, save_file, log)
    if local_code == 0 and save_file.exists() and (not osm_result_empty(save_file)):
        return True

    if log:
        print("⚠️  Local query failed/empty, fallback to remote Overpass...")

    remote_code = query_osm_data_remote(query, save_file, log)
    if remote_code == 200 and save_file.exists() and (not osm_result_empty(save_file)):
        return True

    return False


def _collect_tag_keys(
    osm_file: pathlib.Path,
    focus_keys: list[str],
    surface_only: bool,
    log: bool = False,
) -> tuple[Counter, dict[str, Counter], int]:
    key_counter = Counter()
    key_value_counter: dict[str, Counter] = defaultdict(Counter)
    focus_key_set = set(focus_keys)
    element_count = 0

    # Streaming parse to keep memory stable on large OSM files.
    for event, element in ET.iterparse(str(osm_file), events=("end",)):
        if element.tag in ("node", "way", "relation"):
            element_count += 1
            tags = _element_tags(element)

            if surface_only and not _is_surface_feature(element, tags):
                element.clear()
                continue

            for k, v in tags.items():
                key_counter[k] += 1
                if k in focus_key_set and v:
                    key_value_counter[k][v] += 1

            if log and element_count % 500000 == 0:
                print(f"  Parsed elements: {element_count}")

            element.clear()

    return key_counter, key_value_counter, element_count


def query_tag_categories(
    city_name: str,
    radius: int = 50000,
    top_n: int = 80,
    top_values: int = 50,
    focus_keys: list[str] | None = None,
    surface_only: bool = True,
    log: bool = False,
):
    save_dir = CITY_DIR / city_name
    save_dir.mkdir(parents=True, exist_ok=True)

    osm_file = save_dir / "city_all_tags.osm"
    ok = _query_city_osm(city_name, radius, osm_file, log)
    if not ok:
        print("❌ Failed to query city OSM data for tag-category statistics.")
        print(f"   OSM file: {osm_file}")
        return

    if not focus_keys:
        focus_keys = [
            "amenity", "leisure", "shop", "tourism", "office", "building",
            "landuse", "industrial", "man_made", "natural", "highway", "railway"
        ]

    key_counter, key_value_counter, element_count = _collect_tag_keys(
        osm_file,
        focus_keys=focus_keys,
        surface_only=surface_only,
        log=log,
    )
    total_key_categories = len(key_counter)

    focus_present = [k for k in focus_keys if k in key_counter]
    focus_missing = [k for k in focus_keys if k not in key_counter]

    top_keys = key_counter.most_common(top_n)

    focus_key_value_stats = {}
    for key in focus_present:
        value_counter = key_value_counter.get(key, Counter())
        focus_key_value_stats[key] = {
            "distinct_value_categories": len(value_counter),
            "top_values": [
                {"value": value, "count": count}
                for value, count in value_counter.most_common(top_values)
            ],
        }

    report = {
        "city": city_name,
        "radius": radius,
        "surface_only": surface_only,
        "elements_scanned": element_count,
        "total_key_categories": total_key_categories,
        "focus_keys_present": focus_present,
        "focus_keys_missing": focus_missing,
        "focus_key_occurrences": {k: key_counter.get(k, 0) for k in focus_keys},
        "focus_key_value_stats": focus_key_value_stats,
        "top_keys": [{"key": k, "count": c} for k, c in top_keys],
        "all_keys_sorted": sorted(key_counter.keys()),
    }

    report_path = save_dir / "tag_key_categories_report.json"
    with open(report_path, "w", encoding="utf-8") as f:
        json.dump(report, f, ensure_ascii=False, indent=2)

    print(f"✅ Tag key category report saved: {report_path}")
    _print_report(report, top_n=top_n, top_values=top_values)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Count OSM tag key categories (amenity/leisure/building...) in city data"
    )
    parser.add_argument("city", nargs="?", default=DEFAULT_CITY, help=f"City name (default: {DEFAULT_CITY})")  # type: ignore
    parser.add_argument("--radius", type=int, default=50000, help="Search radius in meters (default: 50000)")
    parser.add_argument("--top", type=int, default=80, help="Top N key categories to print (default: 80)")
    parser.add_argument("--top-values", type=int, default=50, help="Top N values per focus key (default: 50)")
    parser.add_argument(
        "--focus-keys",
        type=str,
        default="amenity,leisure,shop,tourism,office,building,landuse,industrial,man_made,natural,highway,railway",
        help="Comma-separated key list for value-category statistics",
    )
    parser.add_argument(
        "--include-points",
        action="store_true",
        help="Include point features (nodes). Default is surface-only (exclude points).",
    )
    parser.add_argument("--log", action="store_true", help="Enable query log")
    parser.add_argument(
        "--from-report",
        type=str,
        default=None,
        help="Read existing JSON report instead of querying OSM; pass report path or leave empty to use city default path",
    )
    parser.add_argument(
        "--show-key",
        type=str,
        default=None,
        help="When reading report, only print one key's value-category stats (e.g. amenity)",
    )

    args = parser.parse_args()

    if args.from_report is not None:
        from_report_path = args.from_report.strip() if args.from_report else None
        show_report_from_json(
            city_name=args.city,
            report_path=from_report_path,
            top_n=args.top,
            top_values=args.top_values,
            show_key=args.show_key,
        )
        sys.exit(0)

    focus_keys = [k.strip() for k in args.focus_keys.split(",") if k.strip()]
    query_tag_categories(
        args.city,
        radius=args.radius,
        top_n=args.top,
        top_values=args.top_values,
        focus_keys=focus_keys,
        surface_only=not args.include_points,
        log=args.log,
    )
