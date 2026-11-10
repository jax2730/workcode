import argparse
import logging
import pathlib
import sys
import time
import xml.etree.ElementTree as ET

sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
from overpass_query.osm_util import convert_city_osm_2_gpkg, query_osm_data_local, query_osm_data_local_str

from common.db import SessionLocal
from common.model import QueryTask


def _city_name_variants(city_name: str) -> list[str]:
    variants = [city_name]
    if city_name.endswith(" City"):
        variants.append(city_name[:-5])
    else:
        variants.append(f"{city_name} City")
    return list(dict.fromkeys(variants))


def _name_match_clauses(element_type: str, extra_filter: str, city_name: str) -> str:
    clauses = []
    for name in _city_name_variants(city_name):
        clauses.append(f'{element_type}{extra_filter}["name:en"="{name}"];')
        clauses.append(f'{element_type}{extra_filter}["name"="{name}"];')
        clauses.append(f'{element_type}{extra_filter}["name:zh"="{name}"];')
    return "\n        ".join(clauses)


def _city_anchor_query(city_name: str) -> str:
    return f"""
    (
        {_name_match_clauses('node', '["place"~"city|town"]', city_name)}
        {_name_match_clauses('relation', '["boundary"="administrative"]["admin_level"~"4|5"]', city_name)}
    )->.city;
    """


def _city_relation_set_query(city_name: str) -> str:
    return f"""
    (
        {_name_match_clauses('relation', '["boundary"="administrative"]["admin_level"~"4|5"]', city_name)}
    )->.city_rel;
    """


def _relation_member_node_query(city_name: str, role: str) -> str:
    return f"""
    [out:xml][timeout:120][maxsize:1073741824];
    {_city_relation_set_query(city_name)}
    node(r.city_rel:"{role}");
    out body;
    """


def _relation_center_query(city_name: str) -> str:
    return f"""
    [out:xml][timeout:120][maxsize:1073741824];
    {_city_relation_set_query(city_name)}
    .city_rel;
    out body center;
    """


def _city_boundary_relation_query(city_name: str) -> str:
    return f"""
    {_city_relation_set_query(city_name)}
    .city_rel map_to_area -> .city_area;
    """


def _get_element_coord(element: ET.Element):
    lat = element.get("lat")
    lon = element.get("lon")
    if lat is not None and lon is not None:
        return float(lat), float(lon)

    center = element.find("center")
    if center is not None:
        lat = center.get("lat")
        lon = center.get("lon")
        if lat is not None and lon is not None:
            return float(lat), float(lon)

    return None


def _element_score(element: ET.Element, city_name: str) -> int:
    tags = {tag.get("k"): tag.get("v") for tag in element.findall("tag")}
    score = 0

    if tags.get("name:en") == city_name:
        score += 100
    if tags.get("name") == city_name or tags.get("name:zh") == city_name:
        score += 100

    if element.tag == "node" and tags.get("place") == "city":
        score += 40
    elif element.tag == "node" and tags.get("place") == "town":
        score += 30

    if element.tag == "relation" and tags.get("admin_level") == "5":
        score += 25
    elif element.tag == "relation" and tags.get("admin_level") == "4":
        score += 10

    return score


def _best_node_coord_from_xml(center_xml: str, city_name: str):
    root = ET.fromstring(center_xml)
    candidates = []
    for element in root:
        if element.tag != "node":
            continue
        coord = _get_element_coord(element)
        if coord is None:
            continue
        candidates.append((_element_score(element, city_name), coord, element))

    if not candidates:
        return None

    candidates.sort(key=lambda item: item[0], reverse=True)
    return candidates[0][1]


def _best_relation_center_from_xml(center_xml: str, city_name: str):
    root = ET.fromstring(center_xml)
    candidates = []
    for element in root:
        if element.tag != "relation":
            continue
        coord = _get_element_coord(element)
        if coord is None:
            continue
        candidates.append((_element_score(element, city_name), coord, element))

    if not candidates:
        return None

    candidates.sort(key=lambda item: item[0], reverse=True)
    return candidates[0][1]


def _query_city_admin_centre_info(city_name: str, log: bool = False):
    for role, source in (("label", "label"), ("admin_centre", "admin_centre")):
        center_xml = query_osm_data_local_str(_relation_member_node_query(city_name, role), log)
        if not center_xml:
            continue
        coord = _best_node_coord_from_xml(center_xml, city_name)
        if coord is not None:
            return {"lat": coord[0], "lon": coord[1], "source": source}

    center_xml = query_osm_data_local_str(_relation_center_query(city_name), log)
    if center_xml:
        coord = _best_relation_center_from_xml(center_xml, city_name)
        if coord is not None:
            return {"lat": coord[0], "lon": coord[1], "source": "relation_center"}

    raise RuntimeError(f"Fast query found city but no usable label, admin centre or relation center: {city_name}")


def _query_city_admin_centre(city_name: str, log: bool = False):
    center = _query_city_admin_centre_info(city_name, log)
    return center["lat"], center["lon"]


def _road_query_by_city_area_and_center(city_name: str, lat: float, lon: float, radius: int) -> str:
    return f"""
    [out:xml][timeout:1000][maxsize:2147483648];
    {_city_boundary_relation_query(city_name)}
    (
        nwr(area.city_area)(around:{radius},{lat},{lon})["highway"~"^(motorway|trunk|primary|secondary|tertiary|road|unclassified|track|residential|path)$"];
        node(area.city_area)(around:{radius},{lat},{lon})["place"];
        node(area.city_area)(around:{radius},{lat},{lon})["amenity"];
    );
    out body;
    >;
    out skel qt;
    """


def _write_query_center_layer(gpkg_file: pathlib.Path, city_name: str, center: dict, radius: int):
    import geopandas as gpd
    from shapely.geometry import Point

    center_gdf = gpd.GeoDataFrame(
        [{
            "city": city_name,
            "source": center["source"],
            "radius": radius,
            "lat": center["lat"],
            "lon": center["lon"],
        }],
        geometry=[Point(center["lon"], center["lat"])],
        crs="EPSG:4326",
    )
    center_gdf.to_file(str(gpkg_file), layer="query_center", driver="GPKG")


def update_or_create_task(city, radius, result_path: pathlib.Path):
    session = SessionLocal()

    task = session.query(QueryTask).filter(
        QueryTask.city == city,
        QueryTask.radius == radius
    ).first()

    if task is None:
        task = QueryTask(
            city=city,
            radius=radius,
            status="finished",
            result_path=result_path.as_posix()
        )
        session.add(task)
    else:
        task.status = "finished"
        task.result_path = result_path.as_posix()

    session.commit()
    session.close()


def query_city_fast(city_name: str, radius: int = 25000, log: bool = False):
    save_dir = DATA_DIR / "city" / city_name
    save_dir.mkdir(parents=True, exist_ok=True)

    save_file = save_dir / "city.osm"
    gpkg_file = save_dir / "city.gpkg"
    if save_file.exists():
        save_file.unlink()

    center = _query_city_admin_centre_info(city_name, log)
    result = query_osm_data_local(
        _road_query_by_city_area_and_center(city_name, center["lat"], center["lon"], radius),
        save_file,
        log,
    )
    if result != 0 or not save_file.exists() or save_file.stat().st_size == 0:
        raise RuntimeError(f"Fast query failed, city.osm not generated: {save_file}")

    convert_city_osm_2_gpkg(save_file, gpkg_file)
    _write_query_center_layer(gpkg_file, city_name, center, radius)
    update_or_create_task(city_name, radius, gpkg_file)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Query filtered city road data from local Overpass')
    parser.add_argument('-c', '--city_name', type=str, required=True, help='City name')
    parser.add_argument('-r', '--radius', type=int, default=25000, help='Radius of the query in meters')
    parser.add_argument('--log', action='store_true', default=False, help='Log the output')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)
    start_time = time.time()
    query_city_fast(args.city_name, args.radius, args.log)
    print(f"Fast querying city {args.city_name} took {time.time() - start_time:.2f} seconds")
