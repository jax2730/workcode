import osgeo.gdal as gdal
import rasterio
import numpy as np
import math

import pathlib
import sys
sys.path.append(pathlib.Path(__file__).parent.parent.absolute().as_posix())
from constants import *

BASE_RESOLUTION = 1024
def get_raster_area_by_bound(raster_data:rasterio.DatasetReader, bounds:tuple[float, float, float, float], resolution:int = 1024, base_resolution:int = BASE_RESOLUTION):
    up_scale = resolution / base_resolution
    x_min_orgin, y_min_origin, x_max_origin, y_max_origin = bounds
    raster_x_min, raster_y_min, raster_x_max, raster_y_max = raster_data.bounds
    x_min,y_min = raster_data.index(x_min_orgin, y_min_origin)
    x_max,y_max = raster_data.index(x_max_origin, y_max_origin)
    if x_min > x_max:
        x_min, x_max = x_max, x_min
    if y_min > y_max:
        y_min, y_max = y_max, y_min
    row_min = int(math.floor( x_min/ base_resolution) * base_resolution)
    col_min = int(math.floor( y_min/ base_resolution) * base_resolution)
    row_max = int(math.ceil( x_max/ base_resolution) * base_resolution)
    col_max = int(math.ceil(y_max / base_resolution) * base_resolution)
    

    window = rasterio.windows.Window(col_min, row_min, col_max - col_min, row_max - row_min)
    trans = raster_data.window_transform(window)
    return window, trans

def rasterize_district(district_path, raster_path, xmin, ymin, xmax, ymax, driver_name='GTiff', width = 1024, height = 1024, default_value=0):
    
    
    district_ds = gdal.OpenEx(district_path, gdal.OF_VECTOR)
    #district_ds.SetProjection('PROJCRS["WGS_1984_Mercator",BASEGEOGCRS["WGS 84",DATUM["World Geodetic System 1984",ELLIPSOID["WGS 84",6378137,298.257223563,LENGTHUNIT["metre",1]]],PRIMEM["Greenwich",0,ANGLEUNIT["degree",0.0174532925199433]],ID["EPSG",4326]],CONVERSION["Mercator (variant B)",METHOD["Mercator (variant B)",ID["EPSG",9805]],PARAMETER["Latitude of 1st standard parallel",60,ANGLEUNIT["degree",0.0174532925199433],ID["EPSG",8823]],PARAMETER["Longitude of natural origin",150,ANGLEUNIT["degree",0.0174532925199433],ID["EPSG",8802]],PARAMETER["False easting",0,LENGTHUNIT["metre",1],ID["EPSG",8806]],PARAMETER["False northing",0,LENGTHUNIT["metre",1],ID["EPSG",8807]]],CS[Cartesian,2],AXIS["easting",east,ORDER[1],LENGTHUNIT["metre",1,ID["EPSG",9001]]],AXIS["northing",north,ORDER[2],LENGTHUNIT["metre",1,ID["EPSG",9001]]]]')
    district_layer = district_ds.GetLayer()
    district_projection = district_ds.GetProjection()
    x_res = (xmax - xmin) / width
    y_res = (ymax - ymin) / height
    driver = gdal.GetDriverByName(driver_name)
    out_ds = driver.Create(raster_path, width, height, 1, gdal.GDT_UInt32)
    out_ds.SetGeoTransform((xmin, x_res, 0, ymax, 0, -y_res))
    #proj = district_ds.GetSpatialRef().ExportToWkt()
    out_ds.SetProjection('PROJCRS["WGS_1984_Mercator",BASEGEOGCRS["WGS 84",DATUM["World Geodetic System 1984",ELLIPSOID["WGS 84",6378137,298.257223563,LENGTHUNIT["metre",1]]],PRIMEM["Greenwich",0,ANGLEUNIT["degree",0.0174532925199433]],ID["EPSG",4326]],CONVERSION["Mercator (variant B)",METHOD["Mercator (variant B)",ID["EPSG",9805]],PARAMETER["Latitude of 1st standard parallel",60,ANGLEUNIT["degree",0.0174532925199433],ID["EPSG",8823]],PARAMETER["Longitude of natural origin",150,ANGLEUNIT["degree",0.0174532925199433],ID["EPSG",8802]],PARAMETER["False easting",0,LENGTHUNIT["metre",1],ID["EPSG",8806]],PARAMETER["False northing",0,LENGTHUNIT["metre",1],ID["EPSG",8807]]],CS[Cartesian,2],AXIS["easting",east,ORDER[1],LENGTHUNIT["metre",1,ID["EPSG",9001]]],AXIS["northing",north,ORDER[2],LENGTHUNIT["metre",1,ID["EPSG",9001]]]]')
    out_band = out_ds.GetRasterBand(1)
    out_band.SetNoDataValue(default_value)
    out_band.Fill(default_value)
    # rasterize district layer
    gdal.RasterizeLayer(out_ds, [1], district_layer, options=["ATTRIBUTE=adcode"])

    # close datasets
    out_ds = None
    district_ds = None
    return raster_path

def rasterize_district_wRef(district_path, raster_path, ref_raster, driver_name='GTiff', default_value=0):
    district_ds = gdal.OpenEx(district_path, gdal.OF_VECTOR)
    district_layer = district_ds.GetLayer()

    ref_ds = gdal.OpenEx(ref_raster, gdal.OF_RASTER)
    ref_band = ref_ds.GetRasterBand(1)

    driver = gdal.GetDriverByName(driver_name)
    out_ds = driver.Create(raster_path, ref_ds.RasterXSize, ref_ds.RasterYSize, 1, gdal.GDT_UInt32)
    out_ds.SetGeoTransform(ref_ds.GetGeoTransform())
    out_ds.SetProjection(ref_ds.GetProjection())
    out_band = out_ds.GetRasterBand(1)
    out_band.SetNoDataValue(default_value)
    out_band.Fill(default_value)
    # rasterize district layer
    gdal.RasterizeLayer(out_ds, [1], district_layer, options=["ATTRIBUTE=adcode"])

    # close datasets
    out_ds = None
    district_ds = None
    ref_ds = None
    return raster_path

if __name__ == "__main__":
    
    data_proj = WORKING_DIR.joinpath(r'./data/district/world/world_admin_town_proj.gpkg')
    out_rester = WORKING_DIR.joinpath(r'./data/district/world/rasterize_district.tif')
    ref_raster_path = WORKING_DIR.joinpath(r'./data/world_terrain/worldmap_fill.tif')

    # shp_data = gpd.read_file(data_proj)

    # xmin = shp_data.total_bounds[0]
    # ymin = shp_data.total_bounds[1]
    # xmax = shp_data.total_bounds[2]
    # ymax = shp_data.total_bounds[3]
    # with rasterio.Env():
    #     with rasterio.open(ref_raster_path, 'r') as raster_data:
    #         window, trans = get_raster_area_by_bound(raster_data, (xmin, ymin, xmax, ymax), 1024, 1024)
    #         p0 = rasterio.transform.xy(trans, 0, 0, offset='ul')
    #         p1 = rasterio.transform.xy(trans, 0, window.width, offset='ul')
    #         p2 = rasterio.transform.xy(trans, window.height, window.width, offset='ul')
    #         p3 = rasterio.transform.xy(trans, window.height, 0, offset='ul')
    #         xmin = min(p0[0], p1[0], p2[0], p3[0])
    #         xmax = max(p0[0], p1[0], p2[0], p3[0])
    #         ymin = min(p0[1], p1[1], p2[1], p3[1])
    #         ymax = max(p0[1], p1[1], p2[1], p3[1])
    #         width = window.width
    #         height = window.height
    #         rasterize_district(data_proj, out_rester, xmin, ymin, xmax, ymax, driver_name='GTiff', width=width, height=height, default_value=0xffffffff)

    rasterize_district_wRef(data_proj, out_rester, ref_raster_path, driver_name='GTiff', default_value=0)