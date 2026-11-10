if __name__ == "__main__" or __package__ is None:
    from osm_util import *
else:
    from .osm_util import *
import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from constants import *
import geopandas as gpd

CITY_DIR = DATA_DIR / "city"


def resolve_city_dir(city_name: str) -> pathlib.Path:
    primary = CITY_DIR / city_name
    legacy = WORKING_DIR / 'citydata' / city_name
    if primary.exists():
        return primary
    if legacy.exists():
        return legacy
    primary.mkdir(parents=True, exist_ok=True)
    return primary


def get_city_clip_mask(city_name: str):
    city_dir = resolve_city_dir(city_name)
    block_path = city_dir / 'city_street_block_filled.gpkg'
    if block_path.exists():
        block_gdf = gpd.read_file(block_path).to_crs(epsg=3857)
        if len(block_gdf) > 0:
            return block_gdf.geometry.union_all()
    city_path = city_dir / 'city.gpkg'
    if city_path.exists():
        road_gdf = gpd.read_file(city_path, layer='city_road').to_crs(epsg=3857)
        if len(road_gdf) > 0:
            return road_gdf.geometry.union_all().convex_hull
    return None


def build_landuse_blocks(city_name: str, gpkg_path: pathlib.Path, layer_name: str = 'landuse_raw'):
    landuse_gdf = gpd.read_file(gpkg_path, layer=layer_name)
    if len(landuse_gdf) == 0:
        return

    landuse_gdf = landuse_gdf.to_crs(epsg=3857)
    landuse_gdf = landuse_gdf[landuse_gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()

    city_mask = get_city_clip_mask(city_name)
    if city_mask is not None:
        landuse_gdf.geometry = landuse_gdf.geometry.intersection(city_mask)
        landuse_gdf = landuse_gdf[~landuse_gdf.geometry.is_empty].copy()

    landuse_block = landuse_gdf.explode(index_parts=False).reset_index(drop=True)
    landuse_block = landuse_block[landuse_block.geometry.geom_type == 'Polygon'].copy()
    landuse_block['area_m2'] = landuse_block.geometry.area
    landuse_block = landuse_block[landuse_block['area_m2'] > 100].copy()
    landuse_block['block_id'] = range(len(landuse_block))

    landuse_by_type = landuse_block.dissolve(by='landuse', as_index=False)
    landuse_by_type['area_m2'] = landuse_by_type.geometry.area

    landuse_block.to_file(gpkg_path, layer='landuse_block', driver='GPKG')
    landuse_by_type.to_file(gpkg_path, layer='landuse_by_type', driver='GPKG')


def build_landuse_from_local_osm(city_name: str, gpkg_path: pathlib.Path, log: bool = False) -> bool:
    city_dir = resolve_city_dir(city_name)
    osm_dir = city_dir / 'osm'
    if not osm_dir.exists():
        return False

    osm_files = sorted(osm_dir.glob('*.osm'))
    if len(osm_files) == 0:
        return False

    rows = []
    for osm_file in osm_files:
        handler = LanduseHandler()
        try:
            handler.apply_file(str(osm_file), locations=True)
        except Exception:
            continue
        for item in handler.features:
            row = item.copy()
            row['source_file'] = osm_file.name
            rows.append(row)

    if len(rows) == 0:
        return False

    gdf = gpd.GeoDataFrame(rows, crs='EPSG:4326')
    gdf = gdf[gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
    if len(gdf) == 0:
        return False

    gdf = gdf.drop_duplicates(subset=['id', 'landuse', 'source_file']).reset_index(drop=True)
    gdf['landuse_area_m2'] = gdf.to_crs(epsg=3857).geometry.area

    if gpkg_path.exists():
        gpkg_path.unlink()
    gdf.to_file(gpkg_path, layer='landuse_raw', driver='GPKG')
    if log:
        print(f"✅  Build landuse from local osm/: {len(gdf)} features")
    return True


def query_city_landuse(city_name: str, radius: int = 25000, log: bool = False):
    city_dir = resolve_city_dir(city_name)

    osm_path = city_dir / 'landuse.osm'
    gpkg_path = city_dir / 'city_landuse.gpkg'
    allow_remote = os.environ.get('LANDUSE_ALLOW_REMOTE', '0') == '1'
    prefer_local_osm = os.environ.get('LANDUSE_PREFER_LOCAL_OSM', '1') != '0'

    around_query = f"""
    [out:xml][timeout:1200][maxsize:2147483648];
    node[place="city"]["name:en"="{city_name}"];
    nwr(around:{radius})[landuse];
    out body;
    >;
    out skel qt;
    """

    area_query = f"""
    [out:xml][timeout:1200][maxsize:2147483648];
    (
      area["boundary"="administrative"]["name:en"="{city_name}"];
      area["boundary"="administrative"]["name"="{city_name}"];
    )->.searchArea;
    nwr(area.searchArea)[landuse];
    out body;
    >;
    out skel qt;
    """

    city_mask = get_city_clip_mask(city_name)

    if prefer_local_osm:
        built = build_landuse_from_local_osm(city_name, gpkg_path, log=log)
        if built:
            build_landuse_blocks(city_name, gpkg_path, layer_name='landuse_raw')
            if log:
                print(f"✅  Use local osm cache only: {city_dir / 'osm'}")
            return

    def run_query_with_fallback(query_str: str):
        try:
            local_ret = query_osm_data_local(query_str, osm_path, log)
        except FileNotFoundError:
            local_ret = 1
        if (local_ret != 0 or osm_result_empty(osm_path)) and allow_remote:
            try:
                query_osm_data_remote(query_str, osm_path, log)
            except Exception:
                return False
        return not osm_result_empty(osm_path)

    found = run_query_with_fallback(around_query)

    if not found:
        found = run_query_with_fallback(area_query)

    if (not found) and (city_mask is not None):
        city_mask_4326 = gpd.GeoSeries([city_mask], crs='EPSG:3857').to_crs(epsg=4326)
        minx, miny, maxx, maxy = city_mask_4326.total_bounds
        bbox_query = f"""
        [out:xml][timeout:1200][maxsize:2147483648];
        nwr({miny},{minx},{maxy},{maxx})[landuse];
        out body;
        >;
        out skel qt;
        """
        found = run_query_with_fallback(bbox_query)

    if not found:
        found = build_landuse_from_local_osm(city_name, gpkg_path, log=log)
        if not found:
            if log and not allow_remote:
                print("ℹ️  Remote fallback disabled. Set LANDUSE_ALLOW_REMOTE=1 to enable remote query.")
            print(f"No landuse data found for {city_name}")
            return

    if found and osm_path.exists() and (not gpkg_path.exists()):
        convert_landuse_osm_2_gpkg(osm_path, gpkg_path, layer_name='landuse_raw')
    if gpkg_path.exists():
        build_landuse_blocks(city_name, gpkg_path, layer_name='landuse_raw')


def query_cities_landuse(city_names: list[str], radius: int = 25000, log: bool = False):
    for city_name in city_names:
        query_city_landuse(city_name, radius=radius, log=log)
        print(f"Finish querying landuse in {city_name}")


if __name__ == "__main__":
    city_name = "Beijing"
    query_city_landuse(city_name, radius=50000, log=True)
