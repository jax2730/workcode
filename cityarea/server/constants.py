import pathlib
import os

# Define directories
WORKING_DIR = pathlib.Path(__file__).parent
PIXEL_SHARED_DATA_DIR = pathlib.Path('/home/pixel/geo_tools/data')


def _resolve_data_dir() -> pathlib.Path:
    env_data_dir = os.environ.get('CITYAREA_DATA_DIR')
    if env_data_dir:
        return pathlib.Path(env_data_dir)

    return PIXEL_SHARED_DATA_DIR


DATA_DIR = _resolve_data_dir()

# Define default city for queries
DEFAULT_CITY = "Beijing"

# Define HTTP headers
HTTP_HEADERS = {
    "User-Agent": "geo_tools/0.1",
    "Content-Type": "application/x-www-form-urlencoded; charset=UTF-8"
}

# Define API URLs
OVERPASS_URL = "http://overpass-api.de/api/interpreter"
OVERPASS_URL = "https://overpass.private.coffee/api/interpreter"

# Define API WKT formats
PROJECTED_WKT_STR = """PROJCRS["WGS_1984_Mercator",BASEGEOGCRS["WGS 84",DATUM["World Geodetic System 1984",ELLIPSOID["WGS 84",6378137,298.257223563,LENGTHUNIT["metre",1]],ID["EPSG",6326]],PRIMEM["Greenwich",0,ANGLEUNIT["Degree",0.0174532925199433]]],CONVERSION["unnamed",METHOD["Mercator (variant B)",ID["EPSG",9805]],PARAMETER["Latitude of 1st standard parallel",60,ANGLEUNIT["Degree",0.0174532925199433],ID["EPSG",8823]],PARAMETER["Longitude of natural origin",150,ANGLEUNIT["Degree",0.0174532925199433],ID["EPSG",8802]],PARAMETER["False easting",0,LENGTHUNIT["metre",1],ID["EPSG",8806]],PARAMETER["False northing",0,LENGTHUNIT["metre",1],ID["EPSG",8807]]],CS[Cartesian,2],AXIS["(E)",east,ORDER[1],LENGTHUNIT["metre",1,ID["EPSG",9001]]],AXIS["(N)",north,ORDER[2],LENGTHUNIT["metre",1,ID["EPSG",9001]]]]"""

# Explicitly export public symbols for star imports
__all__ = [
    "WORKING_DIR",
    "PIXEL_SHARED_DATA_DIR",
    "DATA_DIR",
    "DEFAULT_CITY",
    "HTTP_HEADERS",
    "OVERPASS_URL",
    "PROJECTED_WKT_STR",
]
