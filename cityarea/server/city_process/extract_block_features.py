import sys
import argparse
import pathlib
import re
import importlib.util
import importlib

import geopandas as gpd
import pandas as pd

sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *

CITY_DIR = DATA_DIR / 'city'


def _sanitize_value_name(value: str) -> str:
    text = str(value).strip().lower()
    text = re.sub(r'[^0-9a-zA-Z_]+', '_', text)
    text = re.sub(r'_+', '_', text).strip('_')
    return text if text else 'unknown'


def _write_optional_excel(df: pd.DataFrame, output_xlsx: pathlib.Path) -> bool:
    try:
        df.to_excel(output_xlsx, index=False)
        return True
    except Exception as exc:
        print(f"⚠️ Skip xlsx export (openpyxl likely missing): {exc}")
        return False


def _list_gpkg_layers(gpkg_path: pathlib.Path) -> list[str]:
    pyogrio_spec = importlib.util.find_spec('pyogrio')
    if pyogrio_spec is not None:
        try:
            pyogrio = importlib.import_module('pyogrio')
            info = pyogrio.list_layers(str(gpkg_path))
            if hasattr(info, 'columns'):
                if 'name' in info.columns:
                    return info['name'].astype(str).tolist()
                return info.iloc[:, 0].astype(str).tolist()
            rows = list(info)
            if len(rows) == 0:
                return []
            first = rows[0]
            if isinstance(first, (tuple, list)):
                return [str(r[0]) for r in rows]
            return [str(r) for r in rows]
        except Exception:
            pass

    fiona_spec = importlib.util.find_spec('fiona')
    if fiona_spec is not None:
        fiona_module = importlib.import_module('fiona')
        return list(fiona_module.listlayers(gpkg_path))

    return []


def _choose_sector_layer(sector_path: pathlib.Path, requested_layer: str | None) -> tuple[gpd.GeoDataFrame, str]:
    if requested_layer:
        return gpd.read_file(sector_path, layer=requested_layer), requested_layer

    layers = _list_gpkg_layers(sector_path)
    if len(layers) == 0:
        return gpd.read_file(sector_path), 'DEFAULT'

    best_layer = None
    best_count = -1
    best_gdf = None

    for layer in layers:
        try:
            gdf = gpd.read_file(sector_path, layer=layer, rows=1)
            count = len([c for c in gdf.columns if str(c).startswith('cat_pct_')])
            if count > best_count:
                best_count = count
                best_layer = layer
        except Exception:
            continue

    if best_layer is None:
        return gpd.read_file(sector_path), 'DEFAULT'

    return gpd.read_file(sector_path, layer=best_layer), str(best_layer)


def extract_features(city_name: str, sector_filename: str, evidence_filename: str, sector_layer: str | None = None):
    city_dir = CITY_DIR / city_name
    sector_path = city_dir / sector_filename
    evidence_path = city_dir / evidence_filename

    if not sector_path.exists():
        print(f"❌ Error: Sector file not found: {sector_path}")
        return
    if not evidence_path.exists():
        print(f"❌ Error: Evidence priority blocks file not found: {evidence_path}")
        return

    print(f"📦 Loading sectors from {sector_filename}...")
    sectors, used_layer = _choose_sector_layer(sector_path, sector_layer)
    print(f"📚 Using sector layer: {used_layer}")
    sectors = sectors.to_crs(epsg=3857)

    if 'sector_id' not in sectors.columns:
        sectors['sector_id'] = ['block_' + str(i).zfill(6) for i in range(len(sectors))]
    else:
        sectors['sector_id'] = sectors['sector_id'].astype(str)

    sectors = sectors.reset_index(drop=True)
    sectors['_block_id'] = sectors.index.astype(str)
    sectors['sector_area'] = sectors.geometry.area

    cat_cols = [c for c in sectors.columns if str(c).startswith('cat_pct_')]
    has_category_constraints = len(cat_cols) > 0
    if has_category_constraints:
        print(f"📌 Using category constraints from sector file: {len(cat_cols)} columns")
    else:
        raise ValueError(
            f"No cat_pct_* columns found in selected sector layer '{used_layer}'. "
            f"Please pass --sector-layer explicitly to the classified layer."
        )

    print(f"📦 Loading evidence from {evidence_filename} (layer='priority_blocks')...")
    evidence = gpd.read_file(evidence_path, layer='priority_blocks')
    evidence = evidence.to_crs(epsg=3857)

    if 'category' not in evidence.columns:
        evidence['category'] = 'unknown'
    evidence['category'] = evidence['category'].fillna('unknown').astype(str)

    if 'value' not in evidence.columns:
        evidence['value'] = evidence['category']
    evidence['value'] = evidence['value'].fillna('unknown').astype(str)

    sec_sub = sectors[['_block_id', 'sector_id', 'sector_area', 'geometry']].copy()
    evi_sub = evidence[['category', 'value', 'geometry']].copy()

    sec_sub = sec_sub[sec_sub.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
    evi_sub = evi_sub[evi_sub.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()

    print("✂️ Calculating intersections...")
    overlay = gpd.overlay(sec_sub, evi_sub, how='intersection', keep_geom_type=False)
    overlay = overlay[overlay.geometry.geom_type.isin(['Polygon', 'MultiPolygon'])].copy()
    overlay['intersect_area'] = overlay.geometry.area
    overlay = overlay[overlay['intersect_area'] > 0].copy()

    output_csv = city_dir / 'block_value_features.csv'
    output_tsv = city_dir / 'block_value_features.tsv'
    output_xlsx = city_dir / 'block_value_features.xlsx'
    output_long = city_dir / 'block_value_features_long.csv'

    if len(overlay) == 0:
        print("⚠️ No intersections found. Writing empty outputs.")
        empty = sec_sub[['_block_id', 'sector_id', 'sector_area']].drop_duplicates(subset=['_block_id']).copy()
        empty['signature'] = 'Empty'
        empty.to_csv(output_csv, index=False, encoding='utf-8-sig')
        empty.to_csv(output_tsv, index=False, sep='\t', encoding='utf-8-sig')
        excel_saved = _write_optional_excel(empty, output_xlsx)

        pd.DataFrame(
            columns=[
                '_block_id', 'sector_id', 'category', 'value',
                'intersect_area_m2', 'sector_area_m2', 'ratio_in_category', 'cat_pct', 'val_pct'
            ]
        ).to_csv(output_long, index=False, encoding='utf-8-sig')

        print(f"✅ Data saved to: {output_csv}")
        print(f"✅ Data saved to: {output_tsv}")
        if excel_saved:
            print(f"✅ Data saved to: {output_xlsx}")
        print(f"✅ Data saved to: {output_long}")
        return

    print("📊 Aggregating value composition...")
    grouped = overlay.groupby(['_block_id', 'sector_id', 'category', 'value'], as_index=False)['intersect_area'].sum()

    area_df = sec_sub[['_block_id', 'sector_area']].drop_duplicates(subset=['_block_id']).copy()
    grouped = grouped.merge(area_df, on='_block_id', how='left', validate='many_to_one')

    grouped['ratio_in_category'] = 0.0
    grouped['cat_pct'] = 0.0

    if has_category_constraints:
        cat_pct_df = sectors[['_block_id'] + cat_cols].copy()
        cat_pct_long = cat_pct_df.melt(
            id_vars=['_block_id'],
            value_vars=cat_cols,
            var_name='cat_col',
            value_name='cat_pct'
        )
        cat_pct_long['category'] = cat_pct_long['cat_col'].str.replace('cat_pct_', '', regex=False)
        cat_pct_long['cat_pct'] = pd.to_numeric(cat_pct_long['cat_pct'], errors='coerce').fillna(0.0)
        cat_pct_long = cat_pct_long[['_block_id', 'category', 'cat_pct']]

        cat_area = grouped.groupby(['_block_id', 'category'], as_index=False)['intersect_area'].sum()
        cat_area = cat_area.rename(columns={'intersect_area': 'cat_area'})

        grouped = grouped.merge(cat_area, on=['_block_id', 'category'], how='left', validate='many_to_one')
        grouped['ratio_in_category'] = (grouped['intersect_area'] / grouped['cat_area']).replace([float('inf'), -float('inf')], 0.0).fillna(0.0)

        grouped = grouped.merge(cat_pct_long, on=['_block_id', 'category'], how='left', suffixes=('', '_target'))
        grouped['cat_pct'] = grouped['cat_pct_target'].fillna(grouped['cat_pct']) if 'cat_pct_target' in grouped.columns else grouped['cat_pct']
        if 'cat_pct_target' in grouped.columns:
            grouped.drop(columns=['cat_pct_target'], inplace=True)

        grouped['cat_pct'] = grouped['cat_pct'].fillna(0.0)
        grouped['val_pct'] = (grouped['ratio_in_category'] * grouped['cat_pct']).fillna(0.0)
    else:
        grouped['val_pct'] = ((grouped['intersect_area'] / grouped['sector_area']) * 100.0).fillna(0.0)

    print("🏷️ Building signatures and wide feature matrix...")

    def make_signature(df: pd.DataFrame) -> str:
        part = df[df['val_pct'] >= 0.5].sort_values('val_pct', ascending=False).head(25)
        if len(part) == 0:
            return 'Empty/Minor'
        return ' | '.join([f"{row['value']}:{row['val_pct']:.2f}%" for _, row in part.iterrows()])

    signatures = grouped.groupby(['_block_id', 'sector_id']).apply(make_signature, include_groups=False).reset_index(name='signature')

    grouped['value_col'] = grouped['value'].apply(_sanitize_value_name)
    pivot_df = grouped.pivot_table(
        index=['_block_id', 'sector_id'],
        columns='value_col',
        values='val_pct',
        aggfunc='sum',
        fill_value=0.0,
    )
    pivot_df.columns = [f'val_pct_{str(c)}' for c in pivot_df.columns]
    pivot_df = pivot_df.reset_index()

    final_df = sec_sub[['_block_id', 'sector_id', 'sector_area']].drop_duplicates(subset=['_block_id']).copy()
    final_df = final_df.merge(signatures, on=['_block_id', 'sector_id'], how='left')
    final_df = final_df.merge(pivot_df, on=['_block_id', 'sector_id'], how='left')

    final_df['signature'] = final_df['signature'].fillna('Empty')
    num_cols = final_df.select_dtypes(include=['number']).columns.tolist()
    if len(num_cols) > 0:
        final_df[num_cols] = final_df[num_cols].fillna(0.0)

    final_df['sector_area'] = final_df['sector_area'].round(3)

    long_df = grouped[['_block_id', 'sector_id', 'category', 'value', 'intersect_area', 'sector_area', 'ratio_in_category', 'cat_pct', 'val_pct']].copy()
    long_df = long_df.rename(columns={'intersect_area': 'intersect_area_m2', 'sector_area': 'sector_area_m2'})
    long_df['intersect_area_m2'] = long_df['intersect_area_m2'].round(3)
    long_df['sector_area_m2'] = long_df['sector_area_m2'].round(3)
    long_df['ratio_in_category'] = long_df['ratio_in_category'].round(8)
    long_df['cat_pct'] = long_df['cat_pct'].round(8)
    long_df['val_pct'] = long_df['val_pct'].round(8)

    coverage_check = long_df.groupby('_block_id', as_index=False)['val_pct'].sum().rename(columns={'val_pct': 'val_pct_sum'})
    over100 = int((coverage_check['val_pct_sum'] > 100.5).sum())

    final_df.to_csv(output_csv, index=False, encoding='utf-8-sig')
    final_df.to_csv(output_tsv, index=False, sep='\t', encoding='utf-8-sig')
    excel_saved = _write_optional_excel(final_df, output_xlsx)
    long_df.to_csv(output_long, index=False, encoding='utf-8-sig')

    print(f"\n✅ Blocks: {len(final_df)}")
    print(f"✅ Value feature dimensions: {len(pivot_df.columns) - 2}")
    print(f"✅ Data saved to: {output_csv}")
    print(f"✅ Data saved to: {output_tsv}")
    if excel_saved:
        print(f"✅ Data saved to: {output_xlsx}")
    print(f"✅ Data saved to: {output_long}")
    print(f"ℹ️ Blocks with val_pct sum > 100.5%: {over100}")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Extract block value features; when cat_pct_* exists in sector file, constrain value pct to those category percentages.'
    )
    parser.add_argument('city', nargs='?', default=DEFAULT_CITY, help=f'City name (default: {DEFAULT_CITY})')  # type: ignore
    parser.add_argument(
        '--sector-file',
        type=str,
        default='classify_city_sector.gpkg',
        help='Classified block file (recommended, contains cat_pct_*), e.g. classify_street_block.gpkg'
    )
    parser.add_argument(
        '--sector-layer',
        type=str,
        default=None,
        help='Layer name in sector gpkg; if omitted, auto-select layer with most cat_pct_* columns'
    )
    parser.add_argument(
        '--evidence-file',
        type=str,
        default='priority_blocks_incremental_man.gpkg',
        help='Priority blocks file containing category/value polygons'
    )
    args = parser.parse_args()

    extract_features(
        city_name=args.city,
        sector_filename=args.sector_file,
        evidence_filename=args.evidence_file,
        sector_layer=args.sector_layer,
    )
