import sys
import pathlib
import importlib.util
import argparse

sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
import geopandas as gpd
import pandas as pd
import osmium
import shapely.wkb as wkblib
import shapely.geometry as geom
import importlib

try:
    import pyogrio
except Exception:
    pyogrio = None

CITY_DIR = DATA_DIR / 'city'


class OSMPolygonHandler(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.polygons = []
        self.wkbfab = osmium.geom.WKBFactory()

    def way(self, w):
        if not w.is_closed():
            return
        try:
            if hasattr(self.wkbfab, 'create_polygon'):
                wkb = self.wkbfab.create_polygon(w)
                polygon = wkblib.loads(wkb, hex=True)
            else:
                wkb = self.wkbfab.create_linestring(w)
                line = wkblib.loads(wkb, hex=True)
                coords = list(line.coords)
                if len(coords) < 4:
                    return
                if coords[0] != coords[-1]:
                    coords.append(coords[0])
                polygon = geom.Polygon(coords)
                if not polygon.is_valid:
                    polygon = polygon.buffer(0)
                if polygon.is_empty:
                    return
            self.polygons.append(polygon)
        except Exception:
            return

    def area(self, a):
        try:
            wkb = self.wkbfab.create_multipolygon(a)
            polygon = wkblib.loads(wkb, hex=True)
            self.polygons.append(polygon)
        except Exception:
            return


def load_category_mapping() -> dict:
    candidates = [
        WORKING_DIR / 'type_clasiffy.py',
        WORKING_DIR.parent / 'type_clasiffy.py',
    ]
    for type_file in candidates:
        if type_file.exists():
            spec = importlib.util.spec_from_file_location('type_clasiffy', str(type_file))
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            if hasattr(module, 'category_mapping'):
                return module.category_mapping
    raise FileNotFoundError('type_clasiffy.py with category_mapping not found')


def build_value_to_category(category_mapping: dict) -> dict:
    value_to_category = {}
    for category, values in category_mapping.items():
        for value in values:
            value_to_category[value] = category
            value_to_category[str(value).lower()] = category
    return value_to_category


def map_to_category(value, value_to_category: dict):
    if value is None or pd.isna(value):
        return None
    text = str(value)
    return value_to_category.get(text) or value_to_category.get(text.lower())


def load_priority_blocks_evidence(
    city_dir: pathlib.Path,
    value_to_category: dict,
    evidence_filename: str | None = None,
    evidence_layer: str | None = None,
) -> gpd.GeoDataFrame:
    candidate_names = [
        evidence_filename,
        'priority_blocks_incremental_man.gpkg',
        'priority_blocks_incremental.gpkg',
        'priority_blocks.gpkg',
        'priority_block.gpkg',
    ]
    candidate_names = [name for name in candidate_names if name]

    priority_path = None
    for name in candidate_names:
        path = city_dir / str(name)
        if path.exists():
            priority_path = path
            break

    if priority_path is None:
        raise FileNotFoundError(
            f'Priority evidence gpkg not found under {city_dir}. Tried: {candidate_names}'
        )

    layer_to_use = evidence_layer
    if layer_to_use is None:
        try:
            layers = list_gpkg_layers(priority_path)
        except Exception:
            layers = []

        if 'priority_blocks' in layers:
            layer_to_use = 'priority_blocks'
        elif len(layers) > 0:
            layer_to_use = str(layers[0])
        else:
            layer_to_use = None

    if layer_to_use is None:
        gdf = gpd.read_file(priority_path)
    else:
        gdf = gpd.read_file(priority_path, layer=layer_to_use)

    print(f'Using evidence file: {priority_path.name}, layer: {layer_to_use if layer_to_use else "DEFAULT"}')
    if len(gdf) == 0:
        return gpd.GeoDataFrame(columns=['macro_type', 'source_category', 'value', 'geometry'])

    gdf = gdf[gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
    if len(gdf) == 0:
        return gpd.GeoDataFrame(columns=['macro_type', 'source_category', 'value', 'geometry'])

    if 'category' not in gdf.columns:
        gdf['category'] = 'unknown'
    if 'value' not in gdf.columns:
        gdf['value'] = gdf['category'].astype(str)

    gdf['source_category'] = gdf['category'].astype(str)
    gdf['macro_type'] = gdf['value'].apply(lambda x: map_to_category(x, value_to_category))
    gdf = gdf.to_crs(epsg=3857)

    return gdf[['macro_type', 'source_category', 'value', 'geometry']]


def build_scored_sector_layer(
    sector_gdf: gpd.GeoDataFrame,
    evidence_gdf: gpd.GeoDataFrame,
    categories: list[str],
) -> gpd.GeoDataFrame:
    if len(sector_gdf) == 0:
        return sector_gdf

    sector = sector_gdf.to_crs(epsg=3857).copy()
    sector = sector.reset_index(drop=False).rename(columns={'index': '_sector_index'})

    for category in categories:
        sector[f'score_{category}'] = 0.0

    if len(evidence_gdf) == 0:
        sector['type'] = 'Others'
        sector['type_score'] = 0.0
        return sector

    evidence_gdf = gpd.GeoDataFrame(evidence_gdf.copy(), geometry='geometry', crs=sector.crs)
    evidence_gdf = evidence_gdf[evidence_gdf.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()

    if len(evidence_gdf) == 0:
        sector['type'] = 'Others'
        sector['type_score'] = 0.0
        return sector

    overlay = gpd.overlay(
        sector[['_sector_index', 'geometry']],
        evidence_gdf[['macro_type', 'source_category', 'geometry']],
        how='intersection',
        keep_geom_type=False,
    )
    overlay = overlay[overlay.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()

    if len(overlay) == 0:
        sector['type'] = 'Others'
        sector['type_score'] = 0.0
        return sector

    overlay['score_area'] = overlay.geometry.area

    source_area_df = (
        overlay.groupby(['_sector_index', 'source_category'])['score_area']
        .sum()
        .reset_index()
    )

    total_source_area = (
        source_area_df.groupby('_sector_index')['score_area']
        .sum()
        .reset_index()
        .rename(columns={'score_area': '_source_total_area'})
    )
    source_area_df = source_area_df.merge(total_source_area, on='_sector_index', how='left')
    source_area_df['source_pct'] = (
        source_area_df['score_area'] / source_area_df['_source_total_area'] * 100.0
    ).fillna(0.0)

    source_categories = sorted(source_area_df['source_category'].astype(str).unique().tolist())
    for source_category in source_categories:
        safe_name = source_category.replace(':', '_').replace('-', '_').replace(' ', '_')
        part = source_area_df[source_area_df['source_category'] == source_category][['_sector_index', 'source_pct']]
        if len(part) == 0:
            continue
        pct_col = f'cat_pct_{safe_name}'
        sector = sector.merge(
            part.rename(columns={'source_pct': pct_col}),
            on='_sector_index',
            how='left',
            suffixes=('', '_new')
        )
        if f'{pct_col}_new' in sector.columns:
            sector[pct_col] = sector[f'{pct_col}_new'].fillna(sector.get(pct_col, 0.0))
            sector.drop(columns=[f'{pct_col}_new'], inplace=True)

    pct_cols = [c for c in sector.columns if c.startswith('cat_pct_')]
    if len(pct_cols) > 0:
        sector[pct_cols] = sector[pct_cols].fillna(0.0)

    score_overlay = overlay[overlay['macro_type'].notna()].copy()
    if len(score_overlay) == 0:
        sector['type'] = 'Others'
        sector['type_score'] = 0.0
        sector.drop(columns=['_sector_index'], inplace=True)
        return sector

    score_df = (
        score_overlay.groupby(['_sector_index', 'macro_type'])['score_area']
        .sum()
        .reset_index()
    )

    for category in categories:
        category_df = score_df[score_df['macro_type'] == category][['_sector_index', 'score_area']]
        if len(category_df) == 0:
            continue
        sector = sector.merge(
            category_df.rename(columns={'score_area': f'score_{category}'}),
            on='_sector_index',
            how='left',
            suffixes=('', '_new')
        )
        if f'score_{category}_new' in sector.columns:
            sector[f'score_{category}'] = sector[f'score_{category}_new'].fillna(sector[f'score_{category}'])
            sector.drop(columns=[f'score_{category}_new'], inplace=True)

    score_cols = [f'score_{category}' for category in categories]
    sector[score_cols] = sector[score_cols].fillna(0.0)
    sector['type'] = sector[score_cols].idxmax(axis=1).str.replace('score_', '', regex=False)
    sector['type_score'] = sector[score_cols].max(axis=1)
    sector.loc[sector['type_score'] <= 0, 'type'] = 'Others'

    sector.drop(columns=['_sector_index'], inplace=True)
    return sector


def list_gpkg_layers(gpkg_path: pathlib.Path) -> list[str]:
    if pyogrio is not None:
        try:
            layer_info = pyogrio.list_layers(str(gpkg_path))
            if hasattr(layer_info, 'columns'):
                if 'name' in layer_info.columns:
                    return layer_info['name'].astype(str).tolist()
                return layer_info.iloc[:, 0].astype(str).tolist()

            rows = list(layer_info)
            if len(rows) == 0:
                return []

            first = rows[0]
            if isinstance(first, (tuple, list)):
                return [str(row[0]) for row in rows]

            if hasattr(first, '__len__') and not isinstance(first, (str, bytes)):
                try:
                    return [str(row[0]) for row in rows]
                except Exception:
                    pass

            return [str(row) for row in rows]
        except Exception:
            pass

    fiona_spec = importlib.util.find_spec('fiona')
    if fiona_spec is None:
        raise ImportError('Neither pyogrio nor fiona is available to read GPKG layers')
    fiona_module = importlib.import_module('fiona')
    return fiona_module.listlayers(gpkg_path)


def classify_city_sector(
    city_name: str,
    sector_filename: str = 'city_sector.gpkg',
    output_filename: str = 'classify_city_sector.gpkg',
    evidence_filename: str | None = None,
    evidence_layer: str | None = None,
):
    city_dir = CITY_DIR / city_name
    sector_path = city_dir / sector_filename
    output_path = city_dir / output_filename

    if not sector_path.exists():
        raise FileNotFoundError(f'city_sector.gpkg not found: {sector_path}')

    category_mapping = load_category_mapping()
    categories = list(category_mapping.keys())
    value_to_category = build_value_to_category(category_mapping)

    evidence_gdf = load_priority_blocks_evidence(
        city_dir,
        value_to_category,
        evidence_filename=evidence_filename,
        evidence_layer=evidence_layer,
    )

    if output_path.exists():
        output_path.unlink()

    layers = list_gpkg_layers(sector_path)
    for layer in layers:
        sector_gdf = gpd.read_file(sector_path, layer=layer)
        classified = build_scored_sector_layer(sector_gdf, evidence_gdf, categories)
        classified.to_file(output_path, layer=layer, driver='GPKG')

    print(f'Saved classified sectors: {output_path}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Classify sector/block polygons by area overlap with priority blocks'
    )
    parser.add_argument('city', nargs='?', default=DEFAULT_CITY, help=f'City name (default: {DEFAULT_CITY})')  # type: ignore
    parser.add_argument('--sector-file', type=str, default='city_sector.gpkg', help='Input sector/block gpkg filename under city dir')
    parser.add_argument('--output-file', type=str, default='classify_city_sector.gpkg', help='Output classified gpkg filename under city dir')
    parser.add_argument('--evidence-file', type=str, default=None, help='Priority evidence gpkg filename under city dir; auto-fallback if omitted')
    parser.add_argument('--evidence-layer', type=str, default=None, help='Layer name in evidence gpkg; defaults to priority_blocks or first layer')
    args = parser.parse_args()

    classify_city_sector(
        city_name=args.city,
        sector_filename=args.sector_file,
        output_filename=args.output_file,
        evidence_filename=args.evidence_file,
        evidence_layer=args.evidence_layer,
    )
