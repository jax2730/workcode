import importlib.util
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

import pandas as pd


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULE_PATH = REPO_ROOT / "server" / "city_process" / "batch_extract_city_labels.py"


def load_module():
    spec = importlib.util.spec_from_file_location("batch_extract_city_labels", MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class BatchExtractCityLabelsTests(unittest.TestCase):
    def test_find_project_root_supports_flat_server_layout_from_screenshot(self):
        module = load_module()

        with tempfile.TemporaryDirectory() as temp_dir:
            root = pathlib.Path(temp_dir)
            (root / "city_process").mkdir()
            (root / "overpass_query").mkdir()
            (root / "overpass_query" / "query_priority_blocks.py").write_text("", encoding="utf-8")
            (root / "overpass_query" / "osm_util.py").write_text("", encoding="utf-8")
            (root / "constants.py").write_text("DATA_DIR = 'x'\n", encoding="utf-8")

            found = module.find_project_root(root / "city_process")

        self.assertEqual(found, root.resolve())

    def test_find_project_root_uses_env_path_when_layout_differs(self):
        module = load_module()

        with tempfile.TemporaryDirectory() as temp_dir:
            root = pathlib.Path(temp_dir)
            (root / "overpass_query").mkdir()
            (root / "overpass_query" / "query_priority_blocks.py").write_text("", encoding="utf-8")
            (root / "overpass_query" / "osm_util.py").write_text("", encoding="utf-8")
            (root / "constants.py").write_text("DATA_DIR = 'x'\n", encoding="utf-8")

            with mock.patch.dict("os.environ", {"CITYAREA_PROJECT_ROOT": str(root)}):
                found = module.find_project_root()

        self.assertEqual(found, root.resolve())

    def test_priority_builder_loader_uses_files_under_discovered_flat_root(self):
        module = load_module()

        with tempfile.TemporaryDirectory() as temp_dir:
            root = pathlib.Path(temp_dir)
            overpass_dir = root / "overpass_query"
            overpass_dir.mkdir(parents=True)
            constants_path = root / "constants.py"
            constants_path.write_text("import pathlib\nDATA_DIR = pathlib.Path('/tmp/cityarea-data')\n", encoding="utf-8")
            (overpass_dir / "osm_util.py").write_text("LOCAL_MARKER = 'project-osm-util'\n", encoding="utf-8")
            (overpass_dir / "query_priority_blocks.py").write_text(
                "if __name__ == '__main__' or __package__ is None:\n"
                "    from osm_util import *\n"
                "else:\n"
                "    raise RuntimeError('must not use package relative import in this test')\n"
                "def query_priority_blocks(**kwargs):\n"
                "    return LOCAL_MARKER\n",
                encoding="utf-8",
            )

            with mock.patch.object(module, "OVERPASS_DIR", overpass_dir), mock.patch.object(module, "CONSTANTS_PATH", constants_path):
                loaded = module._load_query_priority_blocks_module()

        self.assertEqual(loaded.query_priority_blocks(), "project-osm-util")

    def test_extract_value_rows_keeps_only_priority_surface_value_labels(self):
        module = load_module()
        source = pd.DataFrame(
            {
                "category": ["landuse", "building", "highway", "amenity", "man_made"],
                "value": ["residential", "apartments", "primary", "", None],
                "area_m2": [10.0, 20.0, 30.0, 40.0, 50.0],
            }
        )

        rows = module.extract_value_rows("Beijing", source)

        self.assertEqual(rows["value"].tolist(), ["residential", "apartments"])
        self.assertEqual(rows["source_category"].tolist(), ["landuse", "building"])

    def test_summarize_value_rows_counts_city_category_value(self):
        module = load_module()
        rows = pd.DataFrame(
            {
                "city": ["Beijing", "Beijing", "Shanghai"],
                "source_category": ["building", "building", "landuse"],
                "value": ["apartments", "apartments", "residential"],
                "area_m2": [5.0, 7.0, 11.0],
            }
        )

        summary = module.summarize_value_rows(rows)

        self.assertEqual(summary.loc[0, "city"], "Beijing")
        self.assertEqual(summary.loc[0, "source_category"], "building")
        self.assertEqual(summary.loc[0, "value"], "apartments")
        self.assertEqual(summary.loc[0, "feature_count"], 2)
        self.assertEqual(summary.loc[0, "area_m2_sum"], 12.0)

    def test_build_unique_value_catalog_merges_categories_and_cities(self):
        module = load_module()
        summary = pd.DataFrame(
            {
                "city": ["Beijing", "Shanghai"],
                "source_category": ["building", "landuse"],
                "value": ["residential", "residential"],
                "feature_count": [2, 3],
                "area_m2_sum": [12.0, 8.0],
            }
        )

        catalog = module.build_unique_value_catalog(summary)

        self.assertEqual(catalog.loc[0, "value"], "residential")
        self.assertEqual(catalog.loc[0, "source_categories"], "building|landuse")
        self.assertEqual(catalog.loc[0, "cities"], "Beijing|Shanghai")
        self.assertEqual(catalog.loc[0, "total_count"], 5)

    def test_ensure_priority_blocks_generates_when_missing(self):
        module = load_module()

        with tempfile.TemporaryDirectory() as temp_dir:
            city_root = pathlib.Path(temp_dir)
            city_dir = city_root / "Beijing"
            city_dir.mkdir()
            options = module.PriorityBuildOptions(city_root=city_root)

            def fake_generator(city_name, build_options):
                self.assertEqual(city_name, "Beijing")
                (city_dir / "priority_blocks.gpkg").write_text("", encoding="utf-8")

            path, status = module.ensure_priority_blocks("Beijing", options, generator=fake_generator)

        self.assertEqual(path.name, "priority_blocks.gpkg")
        self.assertEqual(status, "generated")

    def test_process_city_reads_existing_priority_without_generating(self):
        module = load_module()

        with tempfile.TemporaryDirectory() as temp_dir:
            city_root = pathlib.Path(temp_dir)
            city_dir = city_root / "Beijing"
            city_dir.mkdir()
            (city_dir / "priority_blocks.gpkg").write_text("", encoding="utf-8")
            options = module.PriorityBuildOptions(city_root=city_root)

            with mock.patch.object(module, "read_priority_blocks", return_value=pd.DataFrame({"category": ["building"], "value": ["yes"]})):
                rows, result = module.process_city("Beijing", options, generator=mock.Mock())

        self.assertEqual(rows["value"].tolist(), ["yes"])
        self.assertEqual(result.priority_status, "existing")


if __name__ == "__main__":
    unittest.main()
