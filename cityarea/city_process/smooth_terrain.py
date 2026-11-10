import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
import numpy as np
import matplotlib.pyplot as plt
import osgeo.ogr as ogr
import osgeo.osr as osr
import osgeo.gdal as gdal
import math
import numpy as np
from shapely.geometry import Polygon, LineString
import convert_coords.convert_coords as convert_coords
import triangle
from scipy.interpolate import RBFInterpolator
from scipy.ndimage import gaussian_filter
import geopandas as gpd
import pandas as pd
import create_city_mask

world_terrian = DATA_DIR /'world_terrain/worldmap_fill.tif'

def polygon_2_triangle3d(polygon:Polygon, crs_wkt:str = None, rbf_height=True) -> list[Polygon]:
    tri_list = []
    poly_extents = np.array(polygon.exterior.coords)[:-1]
    poly_buffered = np.array(LineString(poly_extents).buffer(500).exterior.coords)
    poly_mean = poly_buffered.mean(axis=0)
    z_values = [convert_coords.get_height_at_coord(coord=coord, from_wkt=crs_wkt) for coord in poly_buffered]
    z_values = np.array(z_values)
    n = len(poly_extents)
    segments = [[i,i+1] for i in range(n-1)]
    segments.append([n-1,0])
    poly_scaled = (poly_buffered - poly_mean)/100.0
    A = {
        'vertices': poly_extents,
        'segments': segments,
    }
    t = triangle.triangulate(A, f'pa{100*100}')
    vb = t['vertices']
    vb_scaled = (vb - poly_mean)/100.0
    ib = t['triangles']
    
    rbf = RBFInterpolator(
        poly_scaled,
        z_values,
        smoothing= 1,
        kernel='thin_plate_spline',
        degree=1,
    )
    z_values_interp = rbf(vb_scaled)
    vb3d = np.column_stack([vb[:,0], vb[:,1], z_values_interp])
    ib = ib[:,::-1]
    for tri in range(len(ib)):
        tri_coords = vb3d[ib[tri]]
        tri_poly = Polygon(tri_coords)
        tri_list.append(tri_poly)
    return tri_list


def create_sector_terrain(sector_path:pathlib.Path):
    sector_gdf = gpd.read_file(sector_path)
    city_bound = sector_gdf.total_bounds
    crs_wkt = sector_gdf.crs.to_wkt()
    convert_coords.load_base_gtiff(world_terrian)
    convert_coords.load_x_y_range_data((city_bound[0],city_bound[2]), (city_bound[1],city_bound[3]),from_wkt=crs_wkt)
    sector_3d_gdf = gpd.GeoDataFrame(columns=sector_gdf.columns)

    for i,row in sector_gdf.iterrows():
        if row['geometry'].geom_type == 'Polygon':
            tri_list = polygon_2_triangle3d(row['geometry'], crs_wkt=crs_wkt)
            temp_gdf = gpd.GeoDataFrame({'geometry':tri_list})
            for col in sector_gdf.columns:
                if col != 'geometry':
                    temp_gdf[col] = row[col]
            sector_3d_gdf = pd.concat([sector_3d_gdf,temp_gdf],ignore_index=True)
    sector_3d_gdf['height'] = sector_3d_gdf.geometry.apply(lambda x: np.array(x.exterior.coords)[0:-1].mean(axis=0)[2])
    sector_3d_gdf.set_crs(crs_wkt, inplace=True)
    sector_3d_gdf.to_file(sector_path.parent / f'{sector_path.stem}_3d.gpkg',layer='sector_3d',driver='GPKG')

def create_smoothed_terrain(sector_path:pathlib.Path):
    sector_3d_path = sector_path.parent / f'{sector_path.stem}_3d.gpkg'

    base_gitff_path = DATA_DIR /'world_terrain/worldmap_fill.tif'
    gitff_ds: gdal.Dataset = gdal.Open(str(base_gitff_path))

    sector_ds = ogr.Open(str(sector_3d_path))
    sector_layer = sector_ds.GetLayer()
    city_xmin,city_xmax, city_ymin, city_ymax = sector_layer.GetExtent()

    tiles = create_city_mask.get_map_tiles(gitff_ds, (city_xmin, city_ymin, city_xmax, city_ymax), tile_size=1024, from_epsg=3857)

    for tile in tiles:
        xmin, ymin, xmax, ymax, col, row = tile
        original_ds: gdal.Dataset = gdal.Open(str(DATA_DIR /'world_terrain'/'tiles'/'height'/f'worldmap3_{col+1:03d}_{row+1:03d}.tif'))
        original_band = original_ds.GetRasterBand(1)
        original_bytes = original_band.ReadRaster(0,0,original_ds.RasterXSize, original_ds.RasterYSize, buf_type=gdal.GDT_Float32)
        original_array = np.frombuffer(original_bytes, dtype=np.float32).reshape(original_ds.RasterYSize, original_ds.RasterXSize)

        

        city_gitff_ds: gdal.Dataset = gdal.GetDriverByName('GTiff').Create(sector_path.parent/f'tile_{col+1:03d}_{row+1:03d}.tif', original_ds.RasterXSize, original_ds.RasterYSize, 1, gdal.GDT_Float32)
        city_gitff_ds.SetGeoTransform(original_ds.GetGeoTransform())
        city_gitff_ds.SetProjection(original_ds.GetProjection())

        height_band = city_gitff_ds.GetRasterBand(1)
        height_band.SetNoDataValue(-9999)
        gdal.RasterizeLayer(city_gitff_ds, [1], sector_layer, options=["ATTRIBUTE=height"])
        city_gitff_ds.FlushCache()
        height_bytes = height_band.ReadRaster(0, 0, city_gitff_ds.RasterXSize, city_gitff_ds.RasterYSize, buf_type=gdal.GDT_Float32)
        data_array = np.frombuffer(height_bytes, dtype=np.float32).reshape(city_gitff_ds.RasterYSize, city_gitff_ds.RasterXSize)
        mask = (data_array != -9999)
        # temp_data = np.copy(data_array)
        # temp_data[~mask] = original_array[~mask]
        # weight_data = gaussian_filter(temp_data, sigma=3)
        # result = weight_data
        # weights = gaussian_filter(mask.astype(np.float32), sigma=3)
        # with np.errstate(divide='ignore', invalid='ignore'):
        #     result = weight_data / weights
        result = np.copy(data_array)
        result[~mask] = original_array[~mask]
        # result[~mask] = -9999
        weight_data = gaussian_filter(result, sigma=3)
        weights = gaussian_filter(mask.astype(np.float32), sigma=3)
        reverse_weights = 1 - weights
        with np.errstate(divide='ignore', invalid='ignore'):
            result = weight_data * weights + original_array * reverse_weights
        # result[mask] = smoothed_array[mask]
        height_band.WriteRaster(0, 0, city_gitff_ds.RasterXSize, city_gitff_ds.RasterYSize, result.astype(np.float32).tobytes(), buf_type=gdal.GDT_Float32)
        city_gitff_ds = None
        
        # original_masked_ds = gdal.GetDriverByName('GTiff').Create(sector_path.parent/f'original_masked_tile_{row}_{col}.tif', 1024, 1024, 1, gdal.GDT_Int16)
        # original_masked_ds.SetGeoTransform((xmin, gitff_ds.GetGeoTransform()[1], 0, ymax, 0, gitff_ds.GetGeoTransform()[5]))
        # original_masked_ds.SetProjection(gitff_ds.GetProjection())
        # original_masked_band = original_masked_ds.GetRasterBand(1)
        # original_masked_band.WriteRaster(0, 0, 1024, 1024, (original_array*mask).astype(np.float32).tobytes(), buf_type=gdal.GDT_Int16)

        height_masked_ds = gdal.GetDriverByName('GTiff').Create(sector_path.parent/f'height_masked_tile_{col+1:03d}_{row+1:03d}.tif', original_ds.RasterXSize, original_ds.RasterYSize, 1, gdal.GDT_Float32)
        height_masked_ds.SetGeoTransform((xmin, gitff_ds.GetGeoTransform()[1], 0, ymax, 0, gitff_ds.GetGeoTransform()[5]))
        height_masked_ds.SetProjection(gitff_ds.GetProjection())
        height_masked_band = height_masked_ds.GetRasterBand(1)
        height_masked_band.WriteRaster(0, 0, original_ds.RasterXSize, original_ds.RasterYSize, (result*mask).astype(np.float32).tobytes(), buf_type=gdal.GDT_Float32)
        
        diff_data = result - original_array
        diff_ds = gdal.GetDriverByName('GTiff').Create(sector_path.parent/f'diff_tile_{col+1:03d}_{row+1:03d}.tif', original_ds.RasterXSize, original_ds.RasterYSize, 1, gdal.GDT_Float32)
        diff_ds.SetGeoTransform((xmin, gitff_ds.GetGeoTransform()[1], 0, ymax, 0, gitff_ds.GetGeoTransform()[5]))
        diff_ds.SetProjection(gitff_ds.GetProjection())
        diff_band = diff_ds.GetRasterBand(1)
        diff_band.SetNoDataValue(0)
        diff_band.WriteRaster(0, 0, original_ds.RasterXSize, original_ds.RasterYSize, (diff_data*mask).astype(np.float32).tobytes(), buf_type=gdal.GDT_Float32)


    gitff_ds = None

if __name__ == '__main__':
    sector_path = DATA_DIR/ 'city' / 'Beijing'/ 'city_street_block_merged.gpkg'

    # create_sector_terrain(sector_path)
    create_smoothed_terrain(sector_path)