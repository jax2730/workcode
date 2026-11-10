import osgeo.gdal as gdal
import osgeo.ogr as ogr
import osgeo.osr as osr
import sys
import pathlib
import subprocess
import xml.etree.ElementTree as ET
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *

gdal.SetConfigOption('OGR_SKIP', 'NO')
gdal.SetConfigOption('GDAL_DATA', '/usr/share/gdal/')
def convert_to_shapefile(input_file, output_file):
    gdal_options = {
        'format': 'ESRI Shapefile',
        'dstSRS': 'EPSG:4326',
        'layerCreationOptions': [
            'ENCODING=UTF-8'
        ],
    }

    ds = gdal.VectorTranslate(output_file, input_file, **gdal_options)
    ds = None

def compress_dir(input_dir:pathlib.Path, output_file:pathlib.Path):
    file_list = [str(f) for f in input_dir.glob('*.*')]
    cmd = [
        'zip',
        '-j',
        output_file,
        *file_list
    ]

    subprocess.run(cmd)

def convert_and_compress_road_data(country_code:str):
    road_dir = DATA_DIR / 'road'
    save_dir = road_dir / country_code
    road_file = save_dir / 'road_trimmed.gpkg'
    shapefile_dir = save_dir / 'road'
    compress_dst = road_dir / f'{country_code}_road.zip'
    if not shapefile_dir.exists():
        shapefile_dir.mkdir(parents=True)
    # clear the directory
    for f in shapefile_dir.glob('*.*'):
        f.unlink()
    convert_to_shapefile(road_file, shapefile_dir/'road.shp')
    compress_dir(shapefile_dir, compress_dst)

def merge_geopkgs(input_files:str, output_file:str, format='GPKG'):
    if not input_files:
        return
    
    lco_options = []
    if format == 'ESRI Shapefile':
        lco_options.append('ENCODING=UTF-8')

    
    gdal_options =gdal.VectorTranslateOptions(
        format = format,
        dstSRS = 'EPSG:4326',
        layerCreationOptions = lco_options,
    ) 
    ds = gdal.VectorTranslate( destNameOrDestDS=output_file, srcDS=input_files,options = gdal_options)
    ds = None

def build_vrt(input_files:list[str], output_file:str):
    root = ET.Element("OGRVRTDataSource")

    for f in input_files:
        ds = ogr.Open(f)
        src_layer_name = ds.GetLayerByIndex(0).GetName()
        ds = None
        layer = ET.SubElement(root, "OGRVRTLayer", name=f"road")
        src = ET.SubElement(layer, "SrcDataSource")
        src.text = f
        src_layer = ET.SubElement(layer, "SrcLayer")
        src_layer.text = src_layer_name

    tree = ET.ElementTree(root)
    ET.indent(tree, space="  ", level=0)
    tree.write(output_file)

def merge_all_res_to_world():
    road_dir = DATA_DIR / 'road'
    world_dir = road_dir / 'world'
    if world_dir.exists():
        for f in world_dir.rglob('*.*'):
            if f.is_dir():
                continue
            f.unlink()
        for f in world_dir.rglob('*/'):
            f.rmdir()
        world_dir.rmdir()

    country_roads = []
    for f in road_dir.glob('*/'):
        if not f.is_dir():
            f.unlink()
        gpkg_path = f / 'city_matched_smooth.gpkg'
        if gpkg_path.exists():
            ds = ogr.Open(str(gpkg_path.resolve()))
            if ds is None:
                continue
            ds = None
            country_roads.append(str(gpkg_path.resolve()))
    
    world_dir.mkdir(parents=True)
    world_shapefile = world_dir / 'road_trimmed.gpkg'
    vrt_file = world_dir / 'world.vrt'

    build_vrt(country_roads,vrt_file)

    merge_geopkgs(vrt_file,world_shapefile)
    convert_and_compress_road_data('world')


if __name__ == '__main__':
    merge_all_res_to_world()

