import importlib.util
import pathlib
import sys
import unittest
from unittest import mock


SERVER_ROOT = pathlib.Path(__file__).resolve().parents[2]
MODULE_PATH = SERVER_ROOT / "server_query_ori" / "city_process" / "expand_nearby_cities_pipeline.py"

sys.path.insert(0, str(SERVER_ROOT))
sys.path.insert(0, str(SERVER_ROOT / "server_query"))


def load_pipeline_module():
    spec = importlib.util.spec_from_file_location("expand_nearby_cities_pipeline", MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class ExpandNearbyCitiesPipelineTests(unittest.TestCase):
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

    def test_missing_outputs_detects_street_block_and_poi_pos(self):
        import tempfile

        pipeline = load_pipeline_module()

        with tempfile.TemporaryDirectory() as temp_dir:
            pipeline.CITY_DIR = pathlib.Path(temp_dir)
            missing = pipeline.missing_city_outputs("Tianjin")

        self.assertEqual(missing, {"street_block", "poi_pos"})

    def test_resume_missing_skips_complete_city(self):
        import tempfile

        pipeline = load_pipeline_module()

        with tempfile.TemporaryDirectory() as temp_dir:
            pipeline.CITY_DIR = pathlib.Path(temp_dir)
            city_dir = pathlib.Path(temp_dir) / "Tianjin"
            city_dir.mkdir(parents=True)
            (city_dir / "city_street_block.json").write_text("[]", encoding="utf-8")
            (city_dir / "poi_pos.json").write_text("{}", encoding="utf-8")
            commands = pipeline.build_city_commands(
                city_name="Tianjin",
                py="python",
                radius=25000,
                simplify_tolerance=50.0,
                skip_landmark=False,
                resume_missing=True,
            )

        self.assertEqual(commands, [])

    def test_resume_missing_only_runs_poi_when_poi_pos_missing(self):
        import tempfile

        pipeline = load_pipeline_module()

        with tempfile.TemporaryDirectory() as temp_dir:
            pipeline.CITY_DIR = pathlib.Path(temp_dir)
            city_dir = pathlib.Path(temp_dir) / "Tianjin"
            city_dir.mkdir(parents=True)
            (city_dir / "city_street_block.json").write_text("[]", encoding="utf-8")
            commands = pipeline.build_city_commands(
                city_name="Tianjin",
                py="python",
                radius=25000,
                simplify_tolerance=50.0,
                skip_landmark=False,
                resume_missing=True,
            )

        self.assertEqual(commands, [["python", "city_process/query_city.py", "-c", "Tianjin", "-r", "25000", "--poi"]])

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

    def test_query_landmark_create_landmark_poly_writes_original_path_for_empty_landmarks(self):
        import tempfile
        import types

        query_landmark_path = SERVER_ROOT / "server_query_ori" / "city_process" / "query_landmark.py"
        fake_point_gdf = mock.Mock()
        fake_point_gdf.columns = ["geometry"]
        fake_point_gdf.crs = "EPSG:3857"
        fake_point_gdf.__len__ = mock.Mock(return_value=0)
        fake_point_gdf.iterrows.return_value = iter([])
        fake_poly_gdf = mock.Mock()
        fake_poly_gdf.__len__ = mock.Mock(return_value=0)
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


if __name__ == "__main__":
    unittest.main()
