import sys
import pathlib
SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
sys.path.append(str(SCRIPT_DIR.parent))
sys.path.append(str(SCRIPT_DIR.parent.parent))
from constants import *
import osgeo.ogr as ogr
import osgeo.osr as osr
import osgeo.gdal as gdal
import math
import numpy as np
import argparse
CITY_DIR = DATA_DIR / 'city'

DEFAULT_CITY_NAMES = [
    "Baoding",
    "Langfang",
    "Datong",
    "Qinghuangdao",
    "Shijiazhuang",
    "Hengshui",
    "Langfang",
    "Tangshan",
    "Xiong'an New Area",
    "Zhangjiakou",
]


def get_map_tiles(gitff_ds: gdal.Dataset, city_boundaries: tuple[float,float,float,float], tile_size: int = 1024,from_epsg:int = None, from_wkt:str = None):
    """
    Given a GeoTIFF file and a city boundary, this function returns a list of tiles that cover the city.
    """

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
    city_xmin, city_ymin, city_xmax, city_ymax = city_boundaries

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

def create_city_mask(city_name:str):
    save_dir = CITY_DIR / city_name
    street_block_path = save_dir / 'city_block_out_landmark.gpkg'
    city_mask_path = save_dir / 'city.mask'


    base_gitff_path = DATA_DIR /'world_terrain/worldmap_fill.tif'
    gitff_ds: gdal.Dataset = gdal.Open(str(base_gitff_path))
    tile_dict = {}
    if not city_mask_path.exists():
        total_row = int(math.floor(gitff_ds.RasterYSize/1024))
        total_col = int(math.floor(gitff_ds.RasterXSize/1024))
        total_tiles = total_row * total_col
        with city_mask_path.open('wb') as f:
            mask_meta_data = np.array([total_tiles,total_col,total_row],dtype=np.uint32)
            f.write(mask_meta_data.tobytes())
    else:
        with city_mask_path.open('rb') as f:
            mask_meta_data = np.frombuffer(f.read(12),dtype=np.uint32)
            total_tiles,total_col,total_row = mask_meta_data
            while True:
                try:
                    tile_meta_data = np.frombuffer(f.read(20),dtype=np.uint32)
                    tile_index,row,col,height,width = tile_meta_data
                    tile_dict[f'{row}_{col}'] = {'height':height,'width':width, 'offset': f.tell()}
                    read_bytes = height*width
                    tile_data = np.frombuffer(f.read(read_bytes),dtype=np.uint8).reshape(height,width)
                    tile_data_32 = np.array(tile_data,dtype=np.uint32)
                    # f.seek(f.tell()+height*width)
                    pass
                except:
                    break

    # Load the street block layer
    street_block_ds: ogr.DataSource = ogr.Open(str(street_block_path))
    street_block_lyr: ogr.Layer = street_block_ds.GetLayer()

    # Get the boundaries of the city
    city_xmin,city_xmax, city_ymin, city_ymax = street_block_lyr.GetExtent()

    tiles = get_map_tiles(gitff_ds=gitff_ds, city_boundaries=(city_xmin, city_ymin, city_xmax, city_ymax), tile_size=1024, from_epsg=3857)

    for xmin, ymin, xmax, ymax, row, col in tiles:
        # Create a new raster dataset for the tile
        tile_ds: gdal.Dataset = gdal.GetDriverByName('GTiff').Create(save_dir / f'city_mask_{row}_{col}.tif', 1025, 1025, 1, gdal.GDT_Byte)
        tile_ds.SetGeoTransform((xmin,gitff_ds.GetGeoTransform()[1],0,ymax,0,gitff_ds.GetGeoTransform()[5]))
        tile_ds.SetProjection(gitff_ds.GetProjectionRef())

        # Create a new layer for the tile
        tile_lyr = tile_ds.GetRasterBand(1)
        tile_lyr.SetNoDataValue(0)
        gdal.RasterizeLayer(tile_ds, [1], street_block_lyr, burn_values=[22])

        data_bytes = tile_lyr.ReadRaster(0, 0, 1025, 1025,buf_type=gdal.GDT_Byte)
        data_array = np.frombuffer(data_bytes, dtype=np.uint8).reshape(1025, 1025)

        if f'{row}_{col}' in tile_dict and tile_dict[f'{row}_{col}'] is not None:
            if not (tile_dict[f'{row}_{col}']['height'] == data_array.shape[0] and tile_dict[f'{row}_{col}']['width'] == data_array.shape[1]):
                raise ValueError("Tile size does not match")
            with city_mask_path.open('r+b') as f:
                f.seek(tile_dict[f'{row}_{col}']['offset'])
                old_data_array = np.frombuffer(f.read(tile_dict[f'{row}_{col}']['height']*tile_dict[f'{row}_{col}']['width']),dtype=np.uint8).reshape(tile_dict[f'{row}_{col}']['height'],tile_dict[f'{row}_{col}']['width'])
                new_dataarray = np.where(data_array==0,old_data_array,data_array)
                f.seek(tile_dict[f'{row}_{col}']['offset'])
                f.write(new_dataarray.tobytes())
        else:
            tile_meta_data = np.array([row*total_col+col,row,col,data_array.shape[0],data_array.shape[1]],dtype=np.uint32)
            with city_mask_path.open('ab') as f:
                f.write(tile_meta_data.tobytes())
                f.write(data_array.tobytes())
        tile_ds = None
    gitff_ds = None
    street_block_ds = None

def _unique_city_names(city_names: list[str]) -> list[str]:
    return list(dict.fromkeys(city_names))

def main():
    parser = argparse.ArgumentParser(description='Create city mask')
    parser.add_argument('city_name', nargs='*', type=str, help='Name of the city. Multiple cities are allowed.')
    parser.add_argument('--batch-default', action='store_true', help='Run the default city list')

    args = parser.parse_args()

    if args.batch_default:
        city_names = _unique_city_names(DEFAULT_CITY_NAMES)
    elif args.city_name:
        city_names = _unique_city_names(args.city_name)
    else:
        parser.error('city_name is required unless --batch-default is used')

    for city_name in city_names:
        print(f'Create city mask for {city_name}')
        create_city_mask(city_name)

if __name__ == '__main__':
    main()
    
