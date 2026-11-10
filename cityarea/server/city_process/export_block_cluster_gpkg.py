import sys
import argparse
import pathlib
import importlib.util
import importlib

import geopandas as gpd
import pandas as pd

sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *


CITY_DIR = DATA_DIR / 'city'


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
            if hasattr(first, '__len__') and not isinstance(first, (str, bytes)):
                try:
                    return [str(r[0]) for r in rows]
                except Exception:
                    pass
            return [str(r) for r in rows]
        except Exception:
            pass

    fiona_spec = importlib.util.find_spec('fiona')
    if fiona_spec is not None:
        fiona_module = importlib.import_module('fiona')
        return list(fiona_module.listlayers(gpkg_path))

    return []


def _choose_block_layer(block_path: pathlib.Path, requested_layer: str | None) -> tuple[gpd.GeoDataFrame, str]:
    if requested_layer:
        return gpd.read_file(block_path, layer=requested_layer), requested_layer

    layers = _list_gpkg_layers(block_path)
    if len(layers) == 0:
        return gpd.read_file(block_path), 'DEFAULT'
    if len(layers) == 1:
        layer = layers[0]
        return gpd.read_file(block_path, layer=layer), layer

    best_layer = None
    best_count = -1
    for layer in layers:
        try:
            gdf = gpd.read_file(block_path, layer=layer, rows=1)
            count = len([c for c in gdf.columns if str(c).startswith('cat_pct_')])
            if count > best_count:
                best_count = count
                best_layer = layer
        except Exception:
            continue

    if best_layer is None:
        return gpd.read_file(block_path), 'DEFAULT'

    return gpd.read_file(block_path, layer=best_layer), str(best_layer)


def _resolve_csv(city_dir: pathlib.Path, explicit_file: str | None, candidates: list[str], label: str) -> pathlib.Path:
    if explicit_file is not None and str(explicit_file).strip() != '':
        path = city_dir / explicit_file
        if not path.exists():
            raise FileNotFoundError(f'{label} file not found: {path}')
        return path

    for name in candidates:
        path = city_dir / name
        if path.exists():
            return path

    raise FileNotFoundError(f'{label} file not found, tried: {candidates}')


def _load_assignment(assign_path: pathlib.Path) -> pd.DataFrame:
    df = pd.read_csv(assign_path)
    required = {'_block_id', 'cluster_id'}
    missing = required - set(df.columns)
    if missing:
        raise ValueError(f'Assignment file missing columns: {sorted(missing)}')

    keep = ['_block_id', 'cluster_id']
    for col in ['cluster_signature', 'construction_role', 'construction_pct']:
        if col in df.columns:
            keep.append(col)

    out = df[keep].copy()
    out['_block_id'] = out['_block_id'].astype(str)
    out = out.drop_duplicates(subset=['_block_id'], keep='last')
    return out


def _load_reconstruction(recon_path: pathlib.Path) -> pd.DataFrame:
    df = pd.read_csv(recon_path)
    if '_block_id' not in df.columns:
        raise ValueError('Reconstruction file missing _block_id column')

    value_cols = [c for c in df.columns if str(c).startswith('val_pct_')]
    keep = ['_block_id']
    for col in ['cluster_id', 'construction_role', 'construction_pct']:
        if col in df.columns:
            keep.append(col)
    keep.extend(value_cols)

    out = df[keep].copy()
    out['_block_id'] = out['_block_id'].astype(str)
    out = out.drop_duplicates(subset=['_block_id'], keep='last')
    return out


def export_block_cluster_gpkg(
    city_name: str,
    block_file: str = 'classify_street_block.gpkg',
    block_layer: str | None = None,
    assignment_file: str | None = None,
    reconstruction_file: str | None = None,
    output_file: str = 'classify_streetId_block.gpkg',
    output_layer: str = 'classify_streetId_block',
    include_reconstruction: bool = False,
):
    city_dir = CITY_DIR / city_name
    block_path = city_dir / block_file
    out_path = city_dir / output_file

    if not block_path.exists():
        raise FileNotFoundError(f'Block gpkg not found: {block_path}')

    assignment_path = _resolve_csv(
        city_dir,
        assignment_file,
        candidates=['block_cluster_assignments_bestauto.csv', 'block_cluster_assignments.csv'],
        label='Assignment',
    )

    reconstruction_path = None
    if include_reconstruction:
        reconstruction_path = _resolve_csv(
            city_dir,
            reconstruction_file,
            candidates=['block_reconstruction_from_class_bestauto.csv', 'block_reconstruction_from_class.csv'],
            label='Reconstruction',
        )

    blocks, used_layer = _choose_block_layer(block_path, block_layer)
    blocks = blocks.reset_index(drop=True).copy()

    if 'sector_id' not in blocks.columns:
        blocks['sector_id'] = ['block_' + str(i).zfill(6) for i in range(len(blocks))]
    else:
        blocks['sector_id'] = blocks['sector_id'].astype(str)

    # 这里有个非常关键的契约：
    # _block_id 必须用和特征提取阶段完全一致的方式重建：
    #   reset_index(drop=True) -> index.astype(str)
    # 如果图层不同、行顺序不同、或 index 重建方式不同，
    # 后面的 assignment 就会和几何错位。
    blocks['_block_id'] = blocks.index.astype(str)

    assignment_df = _load_assignment(assignment_path)
    # 这里做的是纯回写：
    # - 按 _block_id 把 cluster 标签挂回 geometry
    # - 不涉及新的聚类计算
    # - 只是为 GIS 可视化和空间联查准备结果
    merged = blocks.merge(assignment_df, on='_block_id', how='left')

    if reconstruction_path is not None:
        reconstruction_df = _load_reconstruction(reconstruction_path)
        # reconstruction 文件里携带的是“该 block 所属 cluster 的 centroid
        # 组成向量”，合并后就能在地图上直接查看 archetype 的组成特征。
        recon_cols = [c for c in reconstruction_df.columns if c not in {'cluster_id', 'construction_role', 'construction_pct'}]
        reconstruction_df = reconstruction_df[recon_cols]
        merged = merged.merge(reconstruction_df, on='_block_id', how='left')

    if out_path.exists():
        out_path.unlink()

    merged.to_file(out_path, layer=output_layer, driver='GPKG')

    matched = int(merged['cluster_id'].notna().sum()) if 'cluster_id' in merged.columns else 0
    print(f'Block layer used: {used_layer}')
    print(f'Assignment source: {assignment_path.name}')
    if reconstruction_path is not None:
        print(f'Reconstruction source: {reconstruction_path.name}')
    print(f'Output gpkg: {out_path}')
    print(f'Total blocks: {len(merged)}, matched cluster_id: {matched}, unmatched: {len(merged) - matched}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Reverse-map cluster results to block geometries using the exact _block_id contract from extract_block_features.'
    )
    parser.add_argument('city', nargs='?', default=DEFAULT_CITY, help=f'City name (default: {DEFAULT_CITY})')  # type: ignore
    parser.add_argument('--block-file', type=str, default='classify_street_block.gpkg', help='Block geometry gpkg used by feature extraction')
    parser.add_argument('--block-layer', type=str, default=None, help='Layer in block gpkg; default auto-pick')
    parser.add_argument('--assignment-file', type=str, default=None, help='Cluster assignment csv under city dir; default auto-pick bestauto then standard')
    parser.add_argument('--reconstruction-file', type=str, default=None, help='Reconstruction csv under city dir; default auto-pick bestauto then standard')
    parser.add_argument('--output-file', type=str, default='classify_streetId_block.gpkg', help='Output gpkg filename under city dir')
    parser.add_argument('--output-layer', type=str, default='classify_streetId_block', help='Output layer name in gpkg')
    parser.add_argument('--include-reconstruction', action='store_true', help='Also merge val_pct_* reconstruction columns')
    args = parser.parse_args()

    export_block_cluster_gpkg(
        city_name=args.city,
        block_file=args.block_file,
        block_layer=args.block_layer,
        assignment_file=args.assignment_file,
        reconstruction_file=args.reconstruction_file,
        output_file=args.output_file,
        output_layer=args.output_layer,
        include_reconstruction=args.include_reconstruction,
    )
