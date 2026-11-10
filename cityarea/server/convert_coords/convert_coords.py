import osgeo.gdal as gdal
import osgeo.ogr as ogr
import osgeo.osr as osr
import shapely.geometry as geom
import math
import numpy as np
import pathlib
import sys
sys.path.append(pathlib.Path(__file__).parent.parent.absolute().as_posix())
from constants import *
from scipy.interpolate import RegularGridInterpolator

BASE_GTIFF_DS = None
BASE_CRS_WKT = None
# BASE_HEIGHT_BAND = None
BASE_RASTER_DATA = None
BASE_RASTER_X_RANGE = (0,0)
BASE_RASTER_Y_RANGE = (0,0)


ORIGIN_POINT = np.array([-2097152.0, -1048576.0])
PIXEL_SCALE = 32

def load_base_gtiff(path:pathlib.Path):
    global BASE_GTIFF_DS
    if not path.exists():
        raise FileNotFoundError(f"File {path} not found")
    BASE_GTIFF_DS = gdal.Open(str(path))
    if not BASE_GTIFF_DS:
        raise ValueError(f"Failed to open {path}")
    global BASE_CRS_WKT
    global BASE_RASTER_DATA
    terrian_crs = osr.SpatialReference()
    terrian_crs.ImportFromWkt(BASE_GTIFF_DS.GetProjectionRef())
    BASE_CRS_WKT = terrian_crs.ExportToWkt()
    pass

def load_x_y_range_data(x_range:tuple, y_range:tuple,from_epsg:int = None, from_wkt:str = None):
    global BASE_RASTER_X_RANGE
    global BASE_RASTER_Y_RANGE
    global BASE_RASTER_DATA
    global BASE_GTIFF_DS
    if not BASE_GTIFF_DS:
        raise ValueError("Base GTIFF not loaded")
    if from_epsg:
        from_crs = osr.SpatialReference()
        from_crs.ImportFromEPSG(from_epsg)
    elif from_wkt:
        from_crs = osr.SpatialReference()
        from_crs.ImportFromWkt(from_wkt)
    else:
        raise ValueError("Either from_epsg or from_wkt should be provided")
    to_crs = osr.SpatialReference()
    to_crs.ImportFromWkt(BASE_CRS_WKT)
    transform = osr.CoordinateTransformation(from_crs, to_crs)
    min_point = ogr.Geometry(ogr.wkbPoint)
    min_point.AddPoint(x_range[0], y_range[0])
    min_point.Transform(transform)
    max_point = ogr.Geometry(ogr.wkbPoint)
    max_point.AddPoint(x_range[1], y_range[1])
    max_point.Transform(transform)

    x_min, x_res, x_skew, y_max, y_skew, y_res = BASE_GTIFF_DS.GetGeoTransform()
    x_min_pixel = (min_point.GetX() - x_min) / x_res - 10
    x_max_pixel = (max_point.GetX() - x_min) / x_res + 10
    y_max_pixel = (min_point.GetY() - y_max) / y_res + 10 
    y_min_pixel = (max_point.GetY() - y_max) / y_res - 10
    x_range_pixel = (int(math.floor(x_min_pixel)), int(math.ceil(x_max_pixel)))
    y_range_pixel = (int(math.floor(y_min_pixel)), int(math.ceil(y_max_pixel)))
    x_range_pixel = (max(0, x_range_pixel[0]), min(x_range_pixel[1], BASE_GTIFF_DS.RasterXSize))
    y_range_pixel = (max(0, y_range_pixel[0]), min(y_range_pixel[1], BASE_GTIFF_DS.RasterYSize))
    x_size_pixel = x_range_pixel[1] - x_range_pixel[0] + 1
    if x_size_pixel <= 0:
        x_size_pixel = BASE_GTIFF_DS.RasterXSize
        x_range_pixel = (0, BASE_GTIFF_DS.RasterXSize)
    y_size_pixel = abs(y_range_pixel[1] - y_range_pixel[0])+1
    
    height_band = BASE_GTIFF_DS.GetRasterBand(1)
    data_bytes = height_band.ReadRaster(x_range_pixel[0], y_range_pixel[0], x_size_pixel, y_size_pixel,
                             buf_type=gdal.GDT_Float32)
    BASE_RASTER_X_RANGE = x_range_pixel
    BASE_RASTER_Y_RANGE = y_range_pixel
    BASE_RASTER_DATA = np.frombuffer(data_bytes, dtype=np.float32).reshape(y_size_pixel, x_size_pixel)
    height_band = None
    pass

def set_origin_point(x:float, y:float):
    global ORIGIN_POINT
    ORIGIN_POINT = np.array([x, y])

def set_pixel_scale(scale:float):
    global PIXEL_SCALE
    PIXEL_SCALE = scale

def convert_coords(coord:np.ndarray=None,x:float=None, y:float=None,\
                    from_epsg:int = None, from_wkt:str = None) -> np.ndarray:
    global BASE_GTIFF_DS
    global BASE_CRS_WKT
    global ORIGIN_POINT

    if not BASE_GTIFF_DS:
        raise ValueError("Base GTIFF not loaded")
    if not BASE_CRS_WKT:
        raise ValueError("Base CRS not loaded")
    from_crs = osr.SpatialReference()
    if from_epsg:
        from_crs.ImportFromEPSG(from_epsg)
    elif from_wkt:
        from_crs.ImportFromWkt(from_wkt)
    else:
        raise ValueError("Either from_epsg or from_wkt should be provided")
    to_crs = osr.SpatialReference()
    to_crs.ImportFromWkt(BASE_CRS_WKT)
    transform = osr.CoordinateTransformation(from_crs, to_crs)
    point = ogr.Geometry(ogr.wkbPoint)
    if coord is not None:
        pass
    elif x is not None and y is not None:
        coord = np.array([x, y])
    else:
        raise ValueError("Either coord or x and y should be provided")
    point.AddPoint(coord[0], coord[1])
    point.Transform(transform)
    
    x_min, x_res, x_skew, y_max, y_skew, y_res = BASE_GTIFF_DS.GetGeoTransform()
    x_pixel = (point.GetX() - x_min) / x_res
    y_pixel = (y_max - point.GetY()) / y_res
    x_coord = (x_pixel - 1)*PIXEL_SCALE + ORIGIN_POINT[0]
    y_coord = (y_pixel - 1)*PIXEL_SCALE + ORIGIN_POINT[1]
    return np.array([x_coord, y_coord])

def convert_coords_with_height(coord:np.ndarray=None,x:float=None, z:float=None,\
                                from_epsg:int = None, from_wkt:str = None) -> np.ndarray:
    global BASE_GTIFF_DS
    global BASE_CRS_WKT
    global BASE_RASTER_X_RANGE
    global BASE_RASTER_Y_RANGE
    global BASE_RASTER_DATA

    if not BASE_GTIFF_DS:
        raise ValueError("Base GTIFF not loaded")
    if not BASE_CRS_WKT:
        raise ValueError("Base CRS not loaded")
    if BASE_RASTER_DATA is None:
        raise ValueError("Base raster data not loaded")
    from_crs = osr.SpatialReference()
    if from_epsg:
        from_crs.ImportFromEPSG(from_epsg)
    elif from_wkt:
        from_crs.ImportFromWkt(from_wkt)
    else:
        raise ValueError("Either from_epsg or from_wkt should be provided")
    to_crs = osr.SpatialReference()
    to_crs.ImportFromWkt(BASE_CRS_WKT)
    transform = osr.CoordinateTransformation(from_crs, to_crs)
    point = ogr.Geometry(ogr.wkbPoint)
    if coord is not None:
        pass
    elif x is not None and z is not None:
        coord = np.array([x, z],dtype=np.float32)
    else:
        raise ValueError("Either coord or x and y should be provided")
    point.AddPoint(np.float64(coord[0]), np.float64(coord[1]))
    point.Transform(transform)
    
    x_min, x_res, x_skew, z_max, z_skew, z_res = BASE_GTIFF_DS.GetGeoTransform()
    x_pixel = (point.GetX() - x_min) / x_res
    z_pixel = (point.GetY() - z_max) / z_res

    if x_pixel < BASE_RASTER_X_RANGE[0] or x_pixel > BASE_RASTER_X_RANGE[1] or\
        z_pixel < BASE_RASTER_Y_RANGE[0] or z_pixel > BASE_RASTER_Y_RANGE[1]:
        raise ValueError("Coordinate out of range")
    else:
        x_pixel = x_pixel - BASE_RASTER_X_RANGE[0]
        z_pixel = z_pixel - BASE_RASTER_Y_RANGE[0]

    x_floor = math.floor(x_pixel)
    y_floor = math.floor(z_pixel)
    x_ceil = x_floor + 1
    y_ceil = y_floor + 1

    x_frac = x_pixel - x_floor
    z_frac = z_pixel - y_floor

    y_00 = BASE_RASTER_DATA[y_floor, x_floor]
    y_01 = BASE_RASTER_DATA[y_floor, x_ceil]
    y_10 = BASE_RASTER_DATA[y_ceil, x_floor]
    y_11 = BASE_RASTER_DATA[y_ceil, x_ceil]

    x_coord = (x_pixel + BASE_RASTER_X_RANGE[0] - 1)*PIXEL_SCALE + ORIGIN_POINT[0]
    z_coord = (z_pixel + BASE_RASTER_Y_RANGE[0] - 1)*PIXEL_SCALE + ORIGIN_POINT[1]

    y = (1 - x_frac)*(1 - z_frac)*y_00 + (1 - x_frac)*z_frac*y_01 +\
        x_frac*(1 - z_frac)*y_10 + x_frac*z_frac*y_11
    # interp_func = RegularGridInterpolator((np.arange(BASE_GTIFF_DS.RasterYSize),np.arange(BASE_GTIFF_DS.RasterXSize)), BASE_RASTER_DATA)
    # y = interp_func([z_pixel,x_pixel])[0]
    y /= 8
    return np.array([x_coord, y, z_coord],dtype=np.float32)

def convert_2_gis_coords(coord:np.ndarray=None,x:float=None, y:float=None,\
                         to_epsg:int = None, to_wkt:str = None) -> np.ndarray:
    global BASE_GTIFF_DS
    global BASE_CRS_WKT
    global BASE_RASTER_X_RANGE
    global BASE_RASTER_Y_RANGE

    if not BASE_GTIFF_DS:
        raise ValueError("Base GTIFF not loaded")
    if not BASE_CRS_WKT:
        raise ValueError("Base CRS not loaded")
    to_crs = osr.SpatialReference()
    if to_epsg:
        to_crs.ImportFromEPSG(to_epsg)
    elif to_wkt:
        to_crs.ImportFromWkt(to_wkt)
    else:
        raise ValueError("Either from_epsg or from_wkt should be provided")
    from_crs = osr.SpatialReference()
    from_crs.ImportFromWkt(BASE_CRS_WKT)
    transform = osr.CoordinateTransformation(from_crs, to_crs)
    point = ogr.Geometry(ogr.wkbPoint)
    if coord is not None:
        pass
    elif x is not None and z is not None:
        coord = np.array([x, z],dtype=np.float32)
    else:
        raise ValueError("Either coord or x and y should be provided")
    x_min, x_res, x_skew, z_max, z_skew, z_res = BASE_GTIFF_DS.GetGeoTransform()

    x_pixel = (coord[0] - ORIGIN_POINT[0]) / PIXEL_SCALE + 1
    y_pixel = (coord[1] - ORIGIN_POINT[1]) / PIXEL_SCALE + 1
    x_coord = x_pixel * x_res + x_min
    y_coord = z_max + y_pixel * z_res
    point.AddPoint(np.float64(x_coord), np.float64(y_coord))
    point.Transform(transform)
    return np.array([point.GetX(), point.GetY()])

def obb_2_poly(center:np.ndarray, extends:np.ndarray, direction:np.ndarray):
    normal = np.array([-direction[1],direction[0]])
    points = np.array([
        center - normal*extends[2] - direction*extends[0],
        center - normal*extends[2] + direction*extends[1],
        center + normal*extends[3] + direction*extends[1],
        center + normal*extends[3] - direction*extends[0],
        center - normal*extends[2] - direction*extends[0]
    ])
    for i in range(5):
        points[i] = convert_2_gis_coords(coord=points[i], to_epsg=3857)
    return geom.Polygon(points)

def get_height_at_coord(coord:np.ndarray=None,x:float=None, y:float=None,\
                        from_epsg:int = None, from_wkt:str = None) -> float:
    global BASE_GTIFF_DS
    global BASE_CRS_WKT
    global BASE_RASTER_X_RANGE
    global BASE_RASTER_Y_RANGE
    global BASE_RASTER_DATA

    if not BASE_GTIFF_DS:
        raise ValueError("Base GTIFF not loaded")
    if not BASE_CRS_WKT:
        raise ValueError("Base CRS not loaded")
    if BASE_RASTER_DATA is None:
        raise ValueError("Base raster data not loaded")
    from_crs = osr.SpatialReference()
    if from_epsg:
        from_crs.ImportFromEPSG(from_epsg)
    elif from_wkt:
        from_crs.ImportFromWkt(from_wkt)
    else:
        raise ValueError("Either from_epsg or from_wkt should be provided")
    to_crs = osr.SpatialReference()
    to_crs.ImportFromWkt(BASE_CRS_WKT)
    transform = osr.CoordinateTransformation(from_crs, to_crs)
    point = ogr.Geometry(ogr.wkbPoint)
    if coord is not None:
        pass
    elif x is not None and y is not None:
        coord = np.array([x, y],dtype=np.float32)
    else:
        raise ValueError("Either coord or x and y should be provided")
    point.AddPoint(np.float64(coord[0]), np.float64(coord[1]))
    point.Transform(transform)
    
    x_min, x_res, x_skew, y_max, y_skew, y_res = BASE_GTIFF_DS.GetGeoTransform()
    x_pixel = (point.GetX() - x_min) / x_res
    y_pixel = (point.GetY() - y_max) / y_res

    if x_pixel < BASE_RASTER_X_RANGE[0] or x_pixel > BASE_RASTER_X_RANGE[1] or\
        y_pixel < BASE_RASTER_Y_RANGE[0] or y_pixel > BASE_RASTER_Y_RANGE[1]:
        raise ValueError("Coordinate out of range")
    else:
        x_pixel = x_pixel - BASE_RASTER_X_RANGE[0]
        y_pixel = y_pixel - BASE_RASTER_Y_RANGE[0]

    x_floor = math.floor(x_pixel)
    y_floor = math.floor(y_pixel)
    x_ceil = x_floor + 1
    y_ceil = y_floor + 1

    x_frac = x_pixel - x_floor
    y_frac = y_pixel - y_floor

    y_00 = BASE_RASTER_DATA[y_floor, x_floor]
    y_01 = BASE_RASTER_DATA[y_floor, x_ceil]
    y_10 = BASE_RASTER_DATA[y_ceil, x_floor]
    y_11 = BASE_RASTER_DATA[y_ceil, x_ceil]

    y = (1 - x_frac)*(1 - y_frac)*y_00 + (1 - x_frac)*y_frac*y_01 +\
        x_frac*(1 - y_frac)*y_10 + x_frac*y_frac*y_11
    
    return y

def convert_coords_list(coords_list:np.ndarray, with_height:bool = False, from_epsg:int = None, from_wkt:str = None) -> np.ndarray:
    old_len = len(coords_list)
    mask = np.concatenate(([True],np.any(coords_list[1:]!=coords_list[:-1],axis=1)))
    coords_list = coords_list[mask]
    new_len = len(coords_list)
    if new_len != old_len:
        pass
    arr_shape = coords_list.shape
    if len(arr_shape) != 2 or arr_shape[1] != 2:
        raise ValueError("coords_list should be a 2D array with 2 columns")
    result = np.ndarray(shape=(arr_shape[0], 3 if with_height else 2), dtype=np.float32)
    for i in range(arr_shape[0]):
        if with_height:
            result[i] = convert_coords_with_height(coord=coords_list[i], from_epsg=from_epsg, from_wkt=from_wkt)
        else:
            result[i] = convert_coords(coord=coords_list[i], from_epsg=from_epsg, from_wkt=from_wkt)
    return result
            

def convert_linestring(linestring:geom.LineString, with_height:bool = False, from_epsg:int = None, from_wkt:str = None) -> np.ndarray:
    coords_list = np.array(linestring.coords,dtype=np.float32)
    return convert_coords_list(coords_list, with_height=with_height, from_epsg=from_epsg, from_wkt=from_wkt)


def convert_gpkg_to_bin(gpkg_path: pathlib.Path, bin_path: pathlib.Path):
    if bin_path.exists():
        bin_path.unlink()
    
    gpkg_ds = ogr.Open(str(gpkg_path),0)
    if not gpkg_ds:
        raise ValueError(f"Failed to open {gpkg_path}")
    layer = gpkg_ds.GetLayer()
    city_xmin,city_xmax, city_ymin, city_ymax = layer.GetExtent()
    load_x_y_range_data((city_xmin,city_xmax),(city_ymin,city_ymax),from_wkt=layer.GetSpatialRef().ExportToWkt())

    gpkg_crs = layer.GetSpatialRef()

    projected_crs = osr.SpatialReference()
    projected_crs.ImportFromEPSG(3857)
    transform = osr.CoordinateTransformation(gpkg_crs, projected_crs)
    # p_layer = layer.Transfrom(transform)
    crs_wkb = projected_crs.ExportToWkt()
    feature_count = layer.GetFeatureCount()


    with bin_path.open('wb') as bin_file:
        bin_file.write(np.array([1,feature_count],dtype=np.uint32).tobytes())
        for i,feature in enumerate(layer):
            ogr_geom = feature.GetGeometryRef()
            if ogr_geom is None:
                continue
            if ogr_geom.GetGeometryType() == ogr.wkbLineString:
                ogr_geom.Transform(transform)
                coords_list = np.array(ogr_geom.GetPoints())

                bin_coords_list = convert_coords_list(coords_list, with_height=True, from_wkt=crs_wkb)
                bin_file.write(np.array([bin_coords_list.shape[0]],dtype=np.uint32).tobytes())
                bin_file.write(bin_coords_list.tobytes())
            elif ogr_geom.GetGeometryType() == ogr.wkbMultiLineString:
                ogr_geom.Transform(transform)
                for j in range(ogr_geom.GetGeometryCount()):
                    ogr_sub_geom = ogr_geom.GetGeometryRef(j)
                    if ogr_sub_geom.GetGeometryType() == ogr.wkbLineString:
                        coords_list = np.array(ogr_sub_geom.GetPoints())
                        bin_coords_list = convert_coords_list(coords_list, with_height=True, from_wkt=crs_wkb)
                        bin_file.write(np.array([bin_coords_list.shape[0]],dtype=np.uint32).tobytes())
                        bin_file.write(bin_coords_list.tobytes())
    layer = None
    gpkg_ds = None

def get_map_tiles(boundaries: tuple[float,float,float,float], tile_size: int = 1024, gitff_ds: gdal.Dataset = BASE_GTIFF_DS,from_epsg:int = None, from_wkt:str = None):
    """
    Given a GeoTIFF file and a boundary, this function returns a list of tiles that cover the city.
    """

    if gitff_ds is None:
        gitff_ds = BASE_GTIFF_DS
    # Get the projection of the GeoTIFF file
    srs = osr.SpatialReference()
    srs.ImportFromWkt(gitff_ds.GetProjectionRef())

    if not gitff_ds:
        raise ValueError("Base GTIFF not loaded")
    if from_epsg:
        from_crs = osr.SpatialReference()
        from_crs.ImportFromEPSG(from_epsg)
    elif from_wkt:
        from_crs = osr.SpatialReference()
        from_crs.ImportFromWkt(from_wkt)
    else:
        raise ValueError("Either from_epsg or from_wkt should be provided")
    
    transform = osr.CoordinateTransformation(from_crs,srs )

    # Get the boundaries of the city
    city_xmin, city_ymin, city_xmax, city_ymax = boundaries

    city_minP = ogr.CreateGeometryFromWkt(f"POINT({city_xmin} {city_ymin})")
    city_maxP = ogr.CreateGeometryFromWkt(f"POINT({city_xmax} {city_ymax})")

    city_minP.Transform(transform)
    city_maxP.Transform(transform)

    #get min row and col
    min_col = (city_minP.GetX() - gitff_ds.GetGeoTransform()[0]) / gitff_ds.GetGeoTransform()[1]
    min_row = (city_maxP.GetY() - gitff_ds.GetGeoTransform()[3]) / gitff_ds.GetGeoTransform()[5]

    #get max row and col
    max_col = (city_maxP.GetX() - gitff_ds.GetGeoTransform()[0]) / gitff_ds.GetGeoTransform()[1]
    max_row = (city_minP.GetY() - gitff_ds.GetGeoTransform()[3]) / gitff_ds.GetGeoTransform()[5]

    min_tile_col = int(math.floor(min_col / tile_size))
    min_tile_row = int(math.floor(min_row / tile_size))
    max_tile_col = int(math.ceil(max_col / tile_size))
    max_tile_row = int(math.ceil(max_row / tile_size))

    # Create a list of tiles
    tiles = []
    for row in range(min_tile_row, max_tile_row):
        for col in range(min_tile_col, max_tile_col):
            xmin = gitff_ds.GetGeoTransform()[0] + col * tile_size * gitff_ds.GetGeoTransform()[1]
            xmax = gitff_ds.GetGeoTransform()[0] + (col+1) * tile_size  * gitff_ds.GetGeoTransform()[1] + 1
            ymin = gitff_ds.GetGeoTransform()[3] + (row + 1) * tile_size  * gitff_ds.GetGeoTransform()[5] + 1 
            ymax = gitff_ds.GetGeoTransform()[3] + row * (tile_size) * gitff_ds.GetGeoTransform()[5]
            tiles.append((xmin, ymin, xmax, ymax, row, col))

    return tiles

if __name__ == '__main__':
    world_terrian = DATA_DIR /'world_terrain/worldmap_fill.tif'
    MAP_OFFSET = [-2097152.0, -1048576.0]; 
    load_base_gtiff(world_terrian)
    set_origin_point(MAP_OFFSET[0],MAP_OFFSET[1])
    set_pixel_scale(32)
    
    import geopandas as gpd

    # road_gpkg = DATA_DIR/'road'/'world'/'road_filtered.gpkg',
    # road_gdf = gpd.read_file(road_gpkg,driver='GPKG')
    # road_gdf.to_crs(epsg=3857, inplace=True)
    # road_gdf.to_file(road_gpkg, driver='GPKG')

    # # load_x_y_range_data((railway_bound[0],railway_bound[2]),(railway_bound[1],railway_bound[3]),from_wkt=railway_gdf.crs.to_wkt())

    # convert_gpkg_to_bin(DATA_DIR/'railway'/'railway.gpkg',DATA_DIR/'railway'/'railway.bin')

    # # get_height_at_coord(x=-1879623.9,y=2416851.7,from_epsg=3857)

    line_gdf = gpd.read_file(DATA_DIR/'road'/'world'/'road_filtered.gpkg')
    line_gdf.to_crs(epsg=3857, inplace=True)
    line_gdf.to_file(DATA_DIR/'road'/'world'/'road_filtered.gpkg', driver='GPKG')
    convert_gpkg_to_bin(DATA_DIR/'road'/'world'/'road_filtered.gpkg',DATA_DIR/'road'/'world'/'world_road.bin')