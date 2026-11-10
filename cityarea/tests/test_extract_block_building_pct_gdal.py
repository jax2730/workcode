from __future__ import annotations

import importlib.util
import pathlib
import sys
import tempfile
import types
import unittest

import pandas as pd


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULE_PATH = REPO_ROOT / "server" / "city_process" / "extract_block_building_pct_gdal.py"


def load_module_with_fake_gis():
    fake_geopandas = types.ModuleType("geopandas")
    fake_osgeo = types.ModuleType("osgeo")
    fake_gdal = types.ModuleType("gdal")
    fake_ogr = types.ModuleType("ogr")
    fake_osgeo.gdal = fake_gdal
    fake_osgeo.ogr = fake_ogr

    original = {name: sys.modules.get(name) for name in ["geopandas", "osgeo", "osgeo.gdal", "osgeo.ogr"]}
    sys.modules["geopandas"] = fake_geopandas
    sys.modules["osgeo"] = fake_osgeo
    sys.modules["osgeo.gdal"] = fake_gdal
    sys.modules["osgeo.ogr"] = fake_ogr
    try:
        spec = importlib.util.spec_from_file_location("extract_block_building_pct_gdal", MODULE_PATH)
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        return module
    finally:
        for name, value in original.items():
            if value is None:
                sys.modules.pop(name, None)
            else:
                sys.modules[name] = value


class ExtractBlockBuildingPctGdalTests(unittest.TestCase):
    def test_default_runtime_paths_use_pixel_geo_tools_layout(self):
        module = load_module_with_fake_gis()

        config = module.resolve_runtime_config("Beijing")

        self.assertEqual(config.city_dir, pathlib.Path("/home/pixel/geo_tools/data/city/Beijing"))
        self.assertEqual(config.block_path, pathlib.Path("/home/pixel/geo_tools/data/city/Beijing/city_street_block.gpkg"))
        self.assertEqual(
            config.building_path,
            pathlib.Path("/home/pixel/geo_tools/data/city/Beijing/priority_blocks_incremental_man_made.gpkg"),
        )
        self.assertEqual(config.output_dir, pathlib.Path("/home/pixel/geo_tools/data/city/Beijing"))
        self.assertEqual(config.building_layer, "priority_blocks")
        self.assertFalse(config.all_touched)

    def test_runtime_prefers_building_complete_priority_file(self):
        module = load_module_with_fake_gis()

        with tempfile.TemporaryDirectory() as temp_dir:
            city_dir = pathlib.Path(temp_dir) / "city" / "Beijing"
            city_dir.mkdir(parents=True)
            complete_path = city_dir / "priority_blocks_incremental_man_made.gpkg"
            complete_path.write_text("placeholder", encoding="utf-8")
            module.DATA_DIR = pathlib.Path(temp_dir)

            config = module.resolve_runtime_config("Beijing")

            self.assertEqual(config.building_path, complete_path)

    def test_add_building_share_columns_normalizes_within_building_total(self):
        module = load_module_with_fake_gis()
        df = pd.DataFrame(
            [
                {"_block_id": "1", "val_pct_apartment": 30.0, "val_pct_retail": 10.0, "building_pct_total": 40.0},
                {"_block_id": "2", "val_pct_apartment": 0.0, "val_pct_retail": 0.0, "building_pct_total": 0.0},
            ]
        )

        result = module.add_building_share_columns(df)
        share_cols = [col for col in result.columns if col.startswith("building_share_")]

        self.assertEqual(set(share_cols), {"building_share_apartment", "building_share_retail"})
        self.assertAlmostEqual(result.loc[0, share_cols].sum(), 100.0)
        self.assertEqual(result.loc[0, "building_share_apartment"], 75.0)
        self.assertEqual(result.loc[0, "building_share_retail"], 25.0)
        self.assertEqual(result.loc[1, share_cols].sum(), 0.0)

    def test_compute_value_composition_uses_category_weight(self):
        module = load_module_with_fake_gis()

        ratio, val_pct = module.compute_value_composition(
            pixel_count=25,
            category_total_pixels=100,
            block_total_pixels=200,
            category_pct=40.0,
        )

        self.assertAlmostEqual(ratio, 0.25)
        self.assertAlmostEqual(val_pct, 10.0)

    def test_compute_value_composition_falls_back_to_all_tag_pixels(self):
        module = load_module_with_fake_gis()

        ratio, val_pct = module.compute_value_composition(
            pixel_count=25,
            category_total_pixels=100,
            block_total_pixels=200,
            category_pct=None,
        )

        self.assertAlmostEqual(ratio, 0.25)
        self.assertAlmostEqual(val_pct, 12.5)

    def test_resolve_effective_category_weights_renormalizes_observed_categories(self):
        module = load_module_with_fake_gis()

        result = module.resolve_effective_category_weights(
            {"landuse": 80, "amenity": 20},
            {"landuse": 60.0, "amenity": 30.0},
        )

        self.assertAlmostEqual(result["landuse"], 66.6666666667)
        self.assertAlmostEqual(result["amenity"], 33.3333333333)
        self.assertAlmostEqual(sum(result.values()), 100.0)

    def test_resolve_effective_category_weights_falls_back_when_weight_fields_incomplete(self):
        module = load_module_with_fake_gis()

        result = module.resolve_effective_category_weights(
            {"landuse": 80, "building": 20},
            {"landuse": 60.0, "building": None},
        )

        self.assertAlmostEqual(result["landuse"], 80.0)
        self.assertAlmostEqual(result["building"], 20.0)
        self.assertAlmostEqual(sum(result.values()), 100.0)

    def test_split_feature_matrix_tables_separates_val_pct_and_building_share(self):
        module = load_module_with_fake_gis()
        df = pd.DataFrame(
            [
                {
                    "_block_id": "1",
                    "sector_id": "s1",
                    "block_area_m2": 100.0,
                    "val_pct_apartment": 30.0,
                    "building_pct_total": 40.0,
                    "building_share_apartment": 75.0,
                }
            ]
        )

        val_pct_df, building_share_df = module.split_feature_matrix_tables(df)

        self.assertEqual(
            list(val_pct_df.columns),
            ["_block_id", "sector_id", "block_area_m2", "val_pct_apartment", "building_pct_total"],
        )
        self.assertEqual(
            list(building_share_df.columns),
            ["_block_id", "sector_id", "block_area_m2", "building_pct_total", "building_share_apartment"],
        )

    def test_filter_building_evidence_keeps_source_category_separate(self):
        module = load_module_with_fake_gis()
        df = pd.DataFrame(
            [
                {"building": "yes", "amenity": "school", "geometry": "geom-a"},
                {"source_category": "building", "value": "apartments", "geometry": "geom-b"},
            ]
        )

        result = module.filter_building_evidence(
            df,
            value_columns=["value", "building", "amenity", "landuse", "leisure", "tourism", "man_made"],
            category_columns=["source_category", "category", "tag_key", "key"],
            allowed_categories={"building", "amenity", "landuse", "leisure", "tourism", "man_made"},
        )

        self.assertEqual(
            set(result["_tag_value"].tolist()),
            {"yes", "school", "apartments"},
        )
        self.assertEqual(
            set(result["_tag_key"].tolist()),
            {"building__yes", "amenity__school", "building__apartments"},
        )
        self.assertEqual(
            set(zip(result["_source_category"], result["_raw_tag_value"])),
            {("building", "yes"), ("amenity", "school"), ("building", "apartments")},
        )


if __name__ == "__main__":
    unittest.main()
