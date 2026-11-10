import importlib.util
import pathlib
import sys
import unittest
from unittest import mock


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
SERVER_ROOT = REPO_ROOT / "server"
MODULE_PATH = SERVER_ROOT / "server_query" / "city_process" / "expand_nearby_cities_pipeline.py"

sys.path.insert(0, str(SERVER_ROOT))
sys.path.insert(0, str(SERVER_ROOT / "server_query"))


def load_pipeline_module():
    spec = importlib.util.spec_from_file_location("expand_nearby_cities_pipeline", MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_query_nearby_cities_module():
    import types

    query_nearby_path = SERVER_ROOT / "server_query" / "overpass_query" / "query_nearby_cities.py"
    fake_osm_util = types.ModuleType("osm_util")
    fake_osm_util.query_osm_data_local_no_output = mock.Mock(return_value=(0, "{}"))
    fake_osm_util.query_osm_data_remote = mock.Mock(return_value=200)
    fake_constants = types.ModuleType("constants")
    fake_constants.DATA_DIR = pathlib.Path(".")
    fake_server_query = types.ModuleType("server_query")
    fake_server_query.__path__ = [str(SERVER_ROOT / "server_query")]
    fake_overpass_query = types.ModuleType("server_query.overpass_query")
    fake_overpass_query.__path__ = [str(SERVER_ROOT / "server_query" / "overpass_query")]

    spec = importlib.util.spec_from_file_location("server_query.overpass_query.query_nearby_cities", query_nearby_path)
    module = importlib.util.module_from_spec(spec)
    with mock.patch.dict(
        sys.modules,
        {
            "server_query": fake_server_query,
            "server_query.overpass_query": fake_overpass_query,
            "server_query.overpass_query.osm_util": fake_osm_util,
            "osm_util": fake_osm_util,
            "constants": fake_constants,
        },
    ):
        spec.loader.exec_module(module)
    return module


def load_query_city_module():
    import types

    query_city_path = SERVER_ROOT / "server_query" / "overpass_query" / "query_city.py"
    fake_osm_util = types.ModuleType("osm_util")
    fake_osm_util.query_osm_data_local = mock.Mock()
    fake_osm_util.convert_city_osm_2_gpkg = mock.Mock()
    fake_osm_util.convert_poi_osm_2_gpkg = mock.Mock()
    fake_constants = types.ModuleType("constants")
    fake_constants.DATA_DIR = pathlib.Path(".")
    fake_constants.WORKING_DIR = pathlib.Path(".")
    fake_server_query = types.ModuleType("server_query")
    fake_server_query.__path__ = [str(SERVER_ROOT / "server_query")]
    fake_overpass_query = types.ModuleType("server_query.overpass_query")
    fake_overpass_query.__path__ = [str(SERVER_ROOT / "server_query" / "overpass_query")]

    spec = importlib.util.spec_from_file_location("server_query.overpass_query.query_city", query_city_path)
    module = importlib.util.module_from_spec(spec)
    with mock.patch.dict(
        sys.modules,
        {
            "server_query": fake_server_query,
            "server_query.overpass_query": fake_overpass_query,
            "server_query.overpass_query.osm_util": fake_osm_util,
            "osm_util": fake_osm_util,
            "constants": fake_constants,
        },
    ):
        spec.loader.exec_module(module)
    return module


def load_simplify_road_module():
    import types
    import networkx as nx

    simplify_road_path = SERVER_ROOT / "server_query" / "city_process" / "simplify_road.py"

    fake_constants = types.ModuleType("constants")
    fake_constants.DATA_DIR = pathlib.Path(".")

    fake_road_process = types.ModuleType("road_process")
    fake_road_process.road_util = mock.Mock()
    fake_road_process.simplify_road = mock.Mock()

    fake_gen_voronoi = types.ModuleType("gen_voronoi")
    fake_gen_voronoi.gen_voroni = mock.Mock()

    fake_osgeo = types.ModuleType("osgeo")
    fake_osgeo_ogr = types.ModuleType("osgeo.ogr")
    fake_osgeo_osr = types.ModuleType("osgeo.osr")
    fake_osgeo_gdal = types.ModuleType("osgeo.gdal")
    fake_osgeo.ogr = fake_osgeo_ogr
    fake_osgeo.osr = fake_osgeo_osr
    fake_osgeo.gdal = fake_osgeo_gdal

    fake_geopandas = types.ModuleType("geopandas")
    fake_geopandas.GeoDataFrame = mock.Mock()
    fake_pandas = types.ModuleType("pandas")
    fake_pandas.DataFrame = mock.Mock()
    fake_skimage = types.ModuleType("skimage")
    fake_skimage_morphology = types.ModuleType("skimage.morphology")
    fake_skimage_morphology.skeletonize = mock.Mock()
    fake_skimage.morphology = fake_skimage_morphology
    fake_sknw = types.ModuleType("sknw")
    fake_numpy = types.ModuleType("numpy")
    fake_shapely = types.ModuleType("shapely")
    fake_shapely_geometry = types.ModuleType("shapely.geometry")
    fake_shapely_geometry.Polygon = object
    fake_shapely_geometry.LineString = object
    fake_shapely_geometry.MultiPolygon = object
    fake_shapely_ops = types.ModuleType("shapely.ops")
    fake_shapely_ops.linemerge = mock.Mock()
    fake_shapely.geometry = fake_shapely_geometry
    fake_shapely.ops = fake_shapely_ops
    fake_matplotlib = types.ModuleType("matplotlib")
    fake_matplotlib_pyplot = types.ModuleType("matplotlib.pyplot")
    fake_matplotlib.pyplot = fake_matplotlib_pyplot
    fake_fiona = types.ModuleType("fiona")
    fake_momepy = types.ModuleType("momepy")
    fake_common_db = types.ModuleType("common.db")
    fake_common_db.SessionLocal = mock.Mock()
    fake_common_model = types.ModuleType("common.model")
    fake_common_model.SimplyfyTask = object
    fake_common_model.LandmarkTask = object

    spec = importlib.util.spec_from_file_location("server_query_simplify_road", simplify_road_path)
    module = importlib.util.module_from_spec(spec)
    with mock.patch.dict(
        sys.modules,
        {
            "constants": fake_constants,
            "road_process": fake_road_process,
            "gen_voronoi": fake_gen_voronoi,
            "osgeo": fake_osgeo,
            "osgeo.ogr": fake_osgeo_ogr,
            "osgeo.osr": fake_osgeo_osr,
            "osgeo.gdal": fake_osgeo_gdal,
            "geopandas": fake_geopandas,
            "pandas": fake_pandas,
            "skimage": fake_skimage,
            "skimage.morphology": fake_skimage_morphology,
            "sknw": fake_sknw,
            "numpy": fake_numpy,
            "shapely": fake_shapely,
            "shapely.geometry": fake_shapely_geometry,
            "shapely.ops": fake_shapely_ops,
            "networkx": nx,
            "matplotlib": fake_matplotlib,
            "matplotlib.pyplot": fake_matplotlib_pyplot,
            "fiona": fake_fiona,
            "momepy": fake_momepy,
            "common.db": fake_common_db,
            "common.model": fake_common_model,
        },
    ):
        spec.loader.exec_module(module)
    return module


class ExpandNearbyCitiesPipelineTests(unittest.TestCase):
    def test_query_city_uses_anchor_set_for_around_query(self):
        query_city = load_query_city_module()
        query = query_city._city_anchor_query("Yuncheng")

        self.assertIn('node[place~"city|town"]["name:en"="Yuncheng"];', query)

    def test_nearby_query_keeps_center_anchor_even_when_candidate_levels_are_narrow(self):
        nearby = load_query_nearby_cities_module()

        query = nearby._build_candidates_query("Beijing", 50000, admin_level_pattern="5")

        self.assertIn('->.center_city;', query)
        self.assertIn('.center_city;', query)
        self.assertIn('node["place"~"city|town"]["name:en"="Beijing"];', query)
        self.assertIn('relation["boundary"="administrative"]["admin_level"~"5"](around.center_city:50000);', query)
        self.assertIn('relation["boundary"="administrative"]["admin_level"~"4|5"]["name:en"="Beijing"];', query)

    def test_single_city_mode_runs_only_requested_city(self):
        pipeline = load_pipeline_module()

        run_list = pipeline.build_run_list(
            center_city="Beijing",
            discovered_city_items=[{"name_en": "Langfang"}, {"name_en": "Tianjin"}],
            include_center=True,
            city_name="Tianjin",
        )

        self.assertEqual(run_list, [{"name_en": "Tianjin"}])

    def test_discovery_mode_preserves_include_center_order(self):
        pipeline = load_pipeline_module()

        run_list = pipeline.build_run_list(
            center_city="Beijing",
            discovered_city_items=[{"name_en": "Langfang"}, {"name_en": "Tianjin"}],
            include_center=True,
            city_name=None,
        )

        self.assertEqual(run_list, [{"name_en": "Beijing"}, {"name_en": "Langfang"}, {"name_en": "Tianjin"}])

    def test_empty_landmark_assets_include_future_editable_lists(self):
        import tempfile

        pipeline = load_pipeline_module()

        with tempfile.TemporaryDirectory() as temp_dir:
            pipeline.CITY_DIR = pathlib.Path(temp_dir)
            pipeline._ensure_landmark_assets("Tianjin")

            city_dir = pathlib.Path(temp_dir) / "Tianjin"
            self.assertEqual((city_dir / "landmarks.json").read_text(encoding="utf-8"), "{}")
            self.assertEqual((city_dir / "landmark_list.json").read_text(encoding="utf-8"), "[]")
            self.assertEqual((city_dir / "landmark_info.json").read_text(encoding="utf-8"), "[]")

    def test_empty_landmark_assets_create_expected_gpkg_paths_when_geopandas_available(self):
        import tempfile

        pipeline = load_pipeline_module()
        fake_gdf = mock.Mock()
        fake_geopandas = mock.Mock()
        fake_geopandas.GeoDataFrame.return_value = fake_gdf

        with tempfile.TemporaryDirectory() as temp_dir:
            pipeline.CITY_DIR = pathlib.Path(temp_dir)
            with mock.patch.dict(sys.modules, {"geopandas": fake_geopandas}):
                pipeline._ensure_landmark_assets("Tianjin")

            city_dir = pathlib.Path(temp_dir) / "Tianjin"
            saved_paths = [call.args[0] for call in fake_gdf.to_file.call_args_list]
            self.assertIn(city_dir / "landmarks" / "landmarks.gpkg", saved_paths)
            self.assertIn(city_dir / "landmark_polygons.gpkg", saved_paths)

    def test_query_landmark_create_landmark_poly_writes_old_and_new_paths_for_empty_landmarks(self):
        import tempfile
        import types

        query_landmark_path = SERVER_ROOT / "server_query" / "city_process" / "query_landmark.py"
        fake_point_gdf = mock.Mock()
        fake_point_gdf.columns = ["geometry"]
        fake_point_gdf.crs = "EPSG:3857"
        fake_point_gdf.__len__ = mock.Mock(return_value=0)
        fake_poly_gdf = mock.Mock()
        fake_geopandas = mock.Mock()
        fake_geopandas.read_file.return_value = fake_point_gdf
        fake_geopandas.GeoDataFrame.return_value = fake_poly_gdf
        fake_convert_coords_pkg = types.ModuleType("convert_coords")
        fake_convert_coords_mod = types.ModuleType("convert_coords.convert_coords")
        fake_convert_coords_mod.load_base_gtiff = mock.Mock()
        fake_convert_coords_mod.set_origin_point = mock.Mock()
        fake_convert_coords_mod.set_pixel_scale = mock.Mock()
        fake_convert_coords_pkg.convert_coords = fake_convert_coords_mod
        fake_shapely_pkg = types.ModuleType("shapely")
        fake_shapely_geometry_mod = types.ModuleType("shapely.geometry")
        fake_shapely_pkg.geometry = fake_shapely_geometry_mod
        fake_overpass_pkg = types.ModuleType("overpass_query")
        fake_overpass_pkg.query_landmark = mock.Mock()
        fake_constants = types.ModuleType("constants")
        fake_constants.DATA_DIR = pathlib.Path(".")

        spec = importlib.util.spec_from_file_location("query_landmark_module", query_landmark_path)
        module = importlib.util.module_from_spec(spec)
        with mock.patch.dict(
            sys.modules,
            {
                "geopandas": fake_geopandas,
                "convert_coords": fake_convert_coords_pkg,
                "convert_coords.convert_coords": fake_convert_coords_mod,
                "shapely": fake_shapely_pkg,
                "shapely.geometry": fake_shapely_geometry_mod,
                "overpass_query": fake_overpass_pkg,
                "constants": fake_constants,
            },
        ):
            spec.loader.exec_module(module)

        with tempfile.TemporaryDirectory() as temp_dir:
            landmark_point_gpkg = pathlib.Path(temp_dir) / "Tianjin" / "landmarks" / "landmarks.gpkg"
            landmark_point_gpkg.parent.mkdir(parents=True, exist_ok=True)
            landmark_file = pathlib.Path(temp_dir) / "Tianjin" / "landmarks.json"
            landmark_file.parent.mkdir(parents=True, exist_ok=True)
            landmark_file.write_text("{}", encoding="utf-8")
            module.create_landmark_poly(landmark_point_gpkg, landmark_file)

            saved_paths = [call.args[0] for call in fake_poly_gdf.to_file.call_args_list]
            self.assertIn(landmark_point_gpkg.parent / "landmarks_poly.gpkg", saved_paths)
            self.assertIn(landmark_point_gpkg.parent.parent / "landmark_polygons.gpkg", saved_paths)

    def test_city_handler_keeps_residential_roads_in_city_road(self):
        import types

        osm_util_path = SERVER_ROOT / "server_query" / "overpass_query" / "osm_util.py"

        fake_osmium = types.ModuleType("osmium")
        fake_osmium.SimpleHandler = type("SimpleHandler", (), {"__init__": lambda self: None})
        fake_osmium_geom = types.ModuleType("osmium.geom")
        fake_osmium_geom.WKBFactory = lambda: mock.Mock(create_linestring=mock.Mock(return_value="fake_wkb"))
        fake_osmium.geom = fake_osmium_geom

        fake_osmium_osm = types.ModuleType("osmium.osm")
        fake_osmium_types = types.ModuleType("osmium.osm.types")
        fake_osmium_types.Node = object
        fake_osmium_types.Way = object
        fake_osmium_types.Relation = object
        fake_osmium_types.Area = object
        fake_osmium_osm.types = fake_osmium_types

        fake_geopandas = types.ModuleType("geopandas")
        fake_geopandas.GeoDataFrame = mock.Mock()

        fake_pandas = types.ModuleType("pandas")
        fake_pandas.DataFrame = mock.Mock()

        fake_shapely_pkg = types.ModuleType("shapely")
        fake_shapely_wkb = types.ModuleType("shapely.wkb")
        fake_shapely_wkb.loads = mock.Mock(return_value="fake_geometry")
        fake_shapely_wkt = types.ModuleType("shapely.wkt")
        fake_shapely_ops = types.ModuleType("shapely.ops")
        fake_shapely_pkg.wkb = fake_shapely_wkb
        fake_shapely_pkg.wkt = fake_shapely_wkt
        fake_shapely_pkg.ops = fake_shapely_ops

        fake_requests = types.ModuleType("requests")
        fake_lxml = types.ModuleType("lxml")
        fake_lxml_etree = types.ModuleType("lxml.etree")
        fake_lxml.etree = fake_lxml_etree

        fake_constants = types.ModuleType("constants")
        fake_constants.DATA_DIR = pathlib.Path(".")
        fake_constants.WORKING_DIR = pathlib.Path(".")
        fake_constants.OVERPASS_URL = ""
        fake_constants.HTTP_HEADERS = {}

        spec = importlib.util.spec_from_file_location("server_query_osm_util", osm_util_path)
        module = importlib.util.module_from_spec(spec)
        with mock.patch.dict(
            sys.modules,
            {
                "osmium": fake_osmium,
                "osmium.geom": fake_osmium_geom,
                "osmium.osm": fake_osmium_osm,
                "osmium.osm.types": fake_osmium_types,
                "geopandas": fake_geopandas,
                "pandas": fake_pandas,
                "shapely": fake_shapely_pkg,
                "shapely.wkb": fake_shapely_wkb,
                "shapely.wkt": fake_shapely_wkt,
                "shapely.ops": fake_shapely_ops,
                "requests": fake_requests,
                "lxml": fake_lxml,
                "lxml.etree": fake_lxml_etree,
                "constants": fake_constants,
            },
        ):
            spec.loader.exec_module(module)

        handler = module.CityHandler()
        residential_way = types.SimpleNamespace(
            id=1449442795,
            tags={"highway": "residential", "name": "柴庄子路"},
        )

        handler.way(residential_way)

        self.assertEqual(len(handler.ways), 1)
        self.assertEqual(handler.ways[0]["highway"], "residential")

    def test_prune_short_leaf_spurs_removes_short_residential_leaf_only(self):
        import json
        import types
        import networkx as nx

        simplify_road = load_simplify_road_module()
        road_network = nx.Graph()

        road_network.add_edge(
            "a",
            "b",
            geometry=types.SimpleNamespace(length=120),
            highway=json.dumps(["tertiary"]),
        )
        road_network.add_edge(
            "b",
            "c",
            geometry=types.SimpleNamespace(length=120),
            highway=json.dumps(["tertiary"]),
        )
        road_network.add_edge(
            "b",
            "spur",
            geometry=types.SimpleNamespace(length=20),
            highway=json.dumps(["residential"]),
        )

        simplify_road._prune_short_leaf_spurs(road_network, max_length=40)

        self.assertFalse(road_network.has_edge("b", "spur"))
        self.assertTrue(road_network.has_edge("a", "b"))
        self.assertTrue(road_network.has_edge("b", "c"))

    def test_prune_short_leaf_spurs_supports_multigraph_edges(self):
        import json
        import types
        import networkx as nx

        simplify_road = load_simplify_road_module()
        road_network = nx.MultiGraph()

        road_network.add_edge(
            "a",
            "b",
            geometry=types.SimpleNamespace(length=120),
            highway=json.dumps(["tertiary"]),
        )
        road_network.add_edge(
            "b",
            "c",
            geometry=types.SimpleNamespace(length=120),
            highway=json.dumps(["tertiary"]),
        )
        road_network.add_edge(
            "b",
            "spur",
            geometry=types.SimpleNamespace(length=20),
            highway=json.dumps(["residential"]),
        )

        simplify_road._prune_short_leaf_spurs(road_network, max_length=40)

        self.assertFalse(road_network.has_edge("b", "spur"))
        self.assertTrue(road_network.has_edge("a", "b"))
        self.assertTrue(road_network.has_edge("b", "c"))

    def test_prune_short_leaf_spurs_default_removes_medium_residential_spur(self):
        import json
        import types
        import networkx as nx

        simplify_road = load_simplify_road_module()
        road_network = nx.Graph()

        road_network.add_edge(
            "a",
            "b",
            geometry=types.SimpleNamespace(length=240),
            highway=json.dumps(["tertiary"]),
        )
        road_network.add_edge(
            "b",
            "c",
            geometry=types.SimpleNamespace(length=240),
            highway=json.dumps(["tertiary"]),
        )
        road_network.add_edge(
            "c",
            "a",
            geometry=types.SimpleNamespace(length=240),
            highway=json.dumps(["tertiary"]),
        )
        road_network.add_edge(
            "b",
            "spur",
            geometry=types.SimpleNamespace(length=150),
            highway=json.dumps(["residential"]),
        )

        simplify_road._prune_short_leaf_spurs(road_network)

        self.assertFalse(road_network.has_edge("b", "spur"))
        self.assertTrue(road_network.has_edge("a", "b"))
        self.assertTrue(road_network.has_edge("b", "c"))
        self.assertTrue(road_network.has_edge("c", "a"))

    def test_prune_short_leaf_spurs_default_removes_long_terminal_leaf(self):
        import json
        import types
        import networkx as nx

        simplify_road = load_simplify_road_module()
        road_network = nx.Graph()

        road_network.add_edge(
            "a",
            "b",
            geometry=types.SimpleNamespace(length=240),
            highway=json.dumps(["tertiary"]),
        )
        road_network.add_edge(
            "b",
            "terminal_leaf",
            geometry=types.SimpleNamespace(length=500),
            highway=json.dumps(["primary"]),
        )

        simplify_road._prune_short_leaf_spurs(road_network)

        self.assertFalse(road_network.has_edge("b", "terminal_leaf"))

    def test_prune_short_leaf_spurs_can_keep_protected_highways(self):
        import json
        import types
        import networkx as nx

        simplify_road = load_simplify_road_module()
        road_network = nx.Graph()

        road_network.add_edge(
            "a",
            "b",
            geometry=types.SimpleNamespace(length=240),
            highway=json.dumps(["tertiary"]),
        )
        road_network.add_edge(
            "b",
            "primary_leaf",
            geometry=types.SimpleNamespace(length=150),
            highway=json.dumps(["primary"]),
        )
        road_network.add_edge(
            "b",
            "c",
            geometry=types.SimpleNamespace(length=240),
            highway=json.dumps(["tertiary"]),
        )
        road_network.add_edge(
            "c",
            "a",
            geometry=types.SimpleNamespace(length=240),
            highway=json.dumps(["tertiary"]),
        )

        simplify_road._prune_short_leaf_spurs(road_network, protected_highways={"primary"})

        self.assertTrue(road_network.has_edge("b", "primary_leaf"))

    def test_merge_degree_2_edge_data_rejects_multilinestring_merge(self):
        import types

        simplify_road = load_simplify_road_module()
        geom0 = types.SimpleNamespace(length=10)
        geom1 = types.SimpleNamespace(length=20)
        merged = types.SimpleNamespace(geom_type="MultiLineString", length=30)

        with mock.patch.object(simplify_road.ops, "linemerge", return_value=merged):
            result = simplify_road._merge_degree_2_edge_data(
                {"geometry": geom0, "highway": "residential"},
                {"geometry": geom1, "highway": "residential"},
            )

        self.assertIsNone(result)

    def test_merge_degree_2_edge_data_accepts_linestring_merge(self):
        import types

        simplify_road = load_simplify_road_module()
        geom0 = types.SimpleNamespace(length=10)
        geom1 = types.SimpleNamespace(length=20)
        merged = types.SimpleNamespace(geom_type="LineString", length=30)

        with mock.patch.object(simplify_road.ops, "linemerge", return_value=merged):
            result = simplify_road._merge_degree_2_edge_data(
                {"geometry": geom0, "highway": "secondary"},
                {"geometry": geom1, "highway": "tertiary"},
            )

        self.assertIsNotNone(result)
        self.assertIs(result["geometry"], merged)
        self.assertEqual(result["mm_len"], 30)
        self.assertEqual(result["highway"], "tertiary")


if __name__ == "__main__":
    unittest.main()
