import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from constants import *
import geopandas as gpd
import numpy as np
import shapely.geometry as geom
import convert_coords.convert_coords as convert_coords
import triangle
import open3d as o3d
from scipy.interpolate import RBFInterpolator
import xatlas
import subprocess
CITY_DIR = DATA_DIR / 'city'
MAP_OFFSET = [-2097152.0, -1048576.0]; 

def polygon_2_triangles(polygon:geom.Polygon) -> np.ndarray:
    A = {
        'vertices': np.array(polygon.exterior.coords),
        'segments': [[i,i+1] for i in range(len(polygon.exterior.coords)-1)]
    }

    t = triangle.triangulate(A, 'p')
    vb = t['vertices']
    ib = t['triangles']

    return vb,ib

def polygon3d_2_triangles(polygon:geom.Polygon, crs_wkt:str = None, rbf_height=True) -> np.ndarray:
    poly_extents = np.array(polygon.exterior.coords)[:-1]
    if rbf_height:
        poly3d = convert_coords.convert_coords_list(poly_extents, with_height=True, from_wkt=crs_wkt)
        poly2d = poly3d[:,[0,2]]
        n = len(poly2d) 
        segments = [[i, i+1] for i in range(n-1)] 
        segments.append([n-1, 0])
        A = {
            'vertices': poly2d,
            'segments': segments
        }
        t = triangle.triangulate(A, f'pa{50*50}')
        vb = t['vertices']
        ib = t['triangles']
        polyh = poly3d[:,1]
        rbf = RBFInterpolator(
            poly2d,
            polyh,
            kernel='thin_plate_spline',
        )
        vb_h = rbf(vb)
        vb3d = np.column_stack([vb[:,0],vb_h,vb[:,1]])
        ib = ib[:,::-1]
    else:
        n = len(poly_extents) 
        segments = [[i, i+1] for i in range(n-1)] 
        segments.append([n-1, 0])
        A = {
            'vertices': poly_extents,
            'segments': segments
        }
        t = triangle.triangulate(A, f'pa{50*50*8*8}')
        vb = t['vertices']
        ib = t['triangles']
        vb3d = convert_coords.convert_coords_list(vb, with_height=True, from_wkt=crs_wkt)
    return vb3d,ib

def save_mesh(vb:np.ndarray, ib:np.ndarray, file_path:pathlib.Path):
    # center = np.mean(vb,axis=0)
    # vb -= center
    mesh = o3d.geometry.TriangleMesh()
    mesh.vertices = o3d.utility.Vector3dVector(vb)
    mesh.triangles = o3d.utility.Vector3iVector(ib)
    mesh.compute_triangle_normals()
    mesh.compute_vertex_normals()
    atlas = xatlas.Atlas() 
    atlas.add_mesh(vb, ib) 
    atlas.generate()
    mesh.paint_uniform_color([0.5, 0.5, 0.5])
    vmapping, indices, uvs = atlas.get_mesh(0)
    mesh.triangle_uvs = o3d.utility.Vector2dVector(uvs[indices].reshape((-1, 2)))
    # mesh.compute_vertex_normals()
    o3d.io.write_triangle_mesh(file_path, mesh, write_triangle_uvs=True)

def obj2fbx(file_path:pathlib.Path):
    output_fbx = file_path.with_suffix('.fbx')
    blender_script = DATA_DIR.parent / 'blender_script/obj2fbx.py'
    subprocess.run([
        'blender',
        '-b',
        '-P', str(blender_script),
        '--',
        str(file_path),
        str(output_fbx),
    ])

def obj2fbx_batch(file_dir:pathlib.Path):
    blender_script = DATA_DIR.parent / 'blender_script/obj2fbx.py'
    process = subprocess.Popen(
        [
        "blender",
        "-b",
        "-P", str(blender_script),
        ],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        text=True,
        bufsize=1,
    )
    for file_path in file_dir.glob('*.obj'):
        output_fbx = file_path.with_suffix('.fbx')
        process.stdin.write(f'{str(file_path)}  {str(output_fbx)}\n')
        process.stdin.flush()
        output = process.stdout
        while True:
            output_line = output.readline().decode().strip()
            if 'FINISH' in output_line or not output_line:
                break
    process.stdin.write("END\n")
    process.stdin.flush()
        

def show_mesh(vb:np.ndarray, ib:np.ndarray):
    mesh = o3d.geometry.TriangleMesh()
    mesh.vertices = o3d.utility.Vector3dVector(vb)
    mesh.triangles = o3d.utility.Vector3iVector(ib)
    mesh.compute_vertex_normals()
    mesh.paint_uniform_color([0.5, 0.5, 0.5])
    o3d.visualization.draw_geometries([mesh])

# def save_mesh2obj(vb:np.ndarray, ib:np.ndarray, file_path:pathlib.Path):
#     with open(file_path, 'w') as f:
#         for v in vb:
#             f.write(f'v {v[0]} {v[1]} {v[2]}\n')
#         for i in ib:
#             f.write(f'f {i[0]+1}//{i[0]+1} {i[1]+1}//{i[1]+1} {i[2]+1}//{i[2]+1}\n')

def appen_vbib(vb:np.ndarray, ib:np.ndarray, vb_new:np.ndarray, ib_new:np.ndarray):
    ib = np.vstack([ib, ib_new+len(vb)])
    vb = np.vstack([vb, vb_new])
    return vb,ib

def create_sectors_mesh(city_name:str):
    save_dir = CITY_DIR / city_name
    sector_gdf = gpd.read_file(save_dir / 'city_sector.gpkg',layer='lvl_0')
    city_center = np.array(sector_gdf.union_all().centroid.coords.xy).reshape(2)
    city_bound = sector_gdf.total_bounds
    crs_wkt = sector_gdf.crs.to_wkt()
    world_terrian = DATA_DIR /'world_terrain/worldmap_fill.tif'
    convert_coords.load_base_gtiff(world_terrian)
    convert_coords.load_x_y_range_data((city_bound[0],city_bound[2]), (city_bound[1],city_bound[3]),from_wkt=crs_wkt)
    convert_coords.set_origin_point(MAP_OFFSET[0],MAP_OFFSET[1])
    convert_coords.set_pixel_scale(32)
    city_center = convert_coords.convert_coords_with_height(city_center, from_wkt=crs_wkt)
    vb = np.empty((0,3))
    ib = np.empty((0,3))
    for i, row in sector_gdf.iterrows():
        sector_poly = row['geometry']
        vb_new,ib_new = polygon3d_2_triangles(sector_poly,crs_wkt,rbf_height=True)
        print(f'Sector {i} has {len(ib)} triangles')
        vb,ib = appen_vbib(vb,ib,vb_new,ib_new)
        # save_mesh2obj(vb,ib,CITY_DIR/'Test' / f'sector_{i}.obj')
        # faces = np.hstack([np.full((ib.shape[0],1),3), ib])
        # mesh = pv.PolyData(vb, faces)
        # mesh.plot(show_edges=True, line_width=1, color='w')
    vb -= city_center
    return vb,ib


if __name__ == '__main__':
    beijing_dir = CITY_DIR / 'Beijing'
    # sector_gdf = gpd.read_file(beijing_dir / 'city_sector.gpkg',layer='lvl_0')
    # vb,ib = create_sectors_mesh('Beijing')
    # show_mesh(vb,ib)
    # save_mesh(vb,ib,beijing_dir/'city_mesh_real_height.obj')
    # obj2fbx(beijing_dir/'city_mesh_real_height.obj')
    obj2fbx_batch(beijing_dir/'sector_mesh')