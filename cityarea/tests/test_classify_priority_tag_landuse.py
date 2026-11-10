import importlib.util
import json
import pathlib
import sys
import unittest

import pandas as pd


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULE_PATH = REPO_ROOT / "tools" / "classify_priority_tag_landuse.py"


def load_module():
    spec = importlib.util.spec_from_file_location("classify_priority_tag_landuse", MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class ClassifyPriorityTagLanduseTests(unittest.TestCase):
    def test_classify_tag_maps_core_residential(self):
        module = load_module()

        result = module.classify_tag("residential", "building|landuse")

        self.assertEqual(result["primary_code"], "07")
        self.assertTrue(result["use_for_landuse_classification"])
        self.assertTrue(str(result["matched_by"]).startswith("semantic"))

    def test_classify_tag_maps_public_service(self):
        module = load_module()

        result = module.classify_tag("school", "amenity|building")

        self.assertEqual(result["primary_code"], "08")
        self.assertIn("教育", result["landuse_hint"])

    def test_classify_tag_maps_transport(self):
        module = load_module()

        result = module.classify_tag("parking_space", "amenity")

        self.assertEqual(result["primary_code"], "12")

    def test_legacy_prior_corrects_apartment_to_residential(self):
        module = load_module()

        result = module.classify_tag("apartment", "tourism")

        self.assertEqual(result["primary_code"], "07")
        self.assertEqual(result["secondary_code"], "0701")
        self.assertEqual(result["tertiary_code"], "070102")
        self.assertEqual(result["matched_by"], "legacy_prior")

    def test_legacy_prior_corrects_warehouse_to_storage(self):
        module = load_module()

        result = module.classify_tag("warehouse", "building|landuse")

        self.assertEqual(result["primary_code"], "11")
        self.assertEqual(result["secondary_code"], "1101")
        self.assertEqual(result["tertiary_code"], "110101")
        self.assertEqual(result["matched_by"], "legacy_prior")

    def test_legacy_prior_corrects_greenhouse_to_agricultural_facility(self):
        module = load_module()

        result = module.classify_tag("greenhouse", "building")

        self.assertEqual(result["primary_code"], "06")
        self.assertEqual(result["secondary_code"], "0602")
        self.assertEqual(result["tertiary_code"], "060201")
        self.assertEqual(result["matched_by"], "legacy_prior")

    def test_guest_house_uses_real_planning_tertiary(self):
        module = load_module()

        result = module.classify_tag("guest_house", "tourism|building")

        self.assertEqual(result["primary_code"], "09")
        self.assertEqual(result["secondary_code"], "0901")
        self.assertEqual(result["tertiary_code"], "090104")

    def test_detached_refines_to_residential_tertiary(self):
        module = load_module()

        result = module.classify_tag("detached", "building")

        self.assertEqual(result["primary_code"], "07")
        self.assertEqual(result["secondary_code"], "0701")
        self.assertEqual(result["tertiary_code"], "070101")

    def test_courthouse_keeps_secondary_without_fabricated_tertiary(self):
        module = load_module()

        result = module.classify_tag("courthouse", "building|amenity")

        self.assertEqual(result["primary_code"], "08")
        self.assertEqual(result["secondary_code"], "0801")
        self.assertEqual(result["tertiary_code"], "")

    def test_classify_tag_maps_apartment_to_residential(self):
        module = load_module()

        result = module.classify_tag("apartment", "building")

        self.assertEqual(result["primary_code"], "07")
        self.assertTrue(str(result["matched_by"]).startswith("reference") or str(result["matched_by"]).startswith("semantic"))

    def test_classify_tag_maps_house_like_tags_to_residential(self):
        module = load_module()

        for value in ["house", "detached", "bungalow"]:
            result = module.classify_tag(value, "building")
            self.assertEqual(result["primary_code"], "07", value)

    def test_curated_baseline_keeps_apartment_residential(self):
        module = load_module()

        result = module.classify_tag("apartment", "building")

        self.assertEqual(result["primary_code"], "07")
        self.assertTrue(str(result["matched_by"]).startswith("semantic") or str(result["matched_by"]) == "legacy_prior")

    def test_curated_baseline_keeps_house_residential(self):
        module = load_module()

        result = module.classify_tag("house", "building")

        self.assertEqual(result["primary_code"], "07")

    def test_classify_tag_keeps_weak_semantics_unclassified(self):
        module = load_module()

        result = module.classify_tag("bench", "amenity")

        self.assertEqual(result["primary_code"], "00")
        self.assertFalse(result["use_for_landuse_classification"])
        self.assertEqual(result["matched_by"], "semantic_exclusion")

    def test_classify_dataframe_preserves_input_metrics(self):
        module = load_module()
        source = pd.DataFrame(
            [
                {
                    "value": "residential",
                    "source_categories": "building|landuse",
                    "cities": "Beijing|Tianjin",
                    "total_count": 10,
                    "area_m2_sum": 1000.0,
                },
                {
                    "value": "bench",
                    "source_categories": "amenity",
                    "cities": "Beijing",
                    "total_count": 2,
                    "area_m2_sum": 10.0,
                },
            ]
        )

        mapped = module.classify_dataframe(source)

        self.assertEqual(len(mapped), 2)
        self.assertEqual(mapped.loc[0, "value"], "residential")
        self.assertEqual(mapped.loc[0, "primary_code"], "07")
        self.assertEqual(mapped.loc[1, "primary_code"], "00")

    def test_build_export_mapping_dataframe_exports_hierarchical_landuse_columns(self):
        module = load_module()
        source = pd.DataFrame(
            [
                {
                    "value": "restaurant",
                    "source_categories": "amenity",
                    "cities": "Beijing|Tianjin",
                    "total_count": 10,
                    "area_m2_sum": 1000.0,
                },
                {
                    "value": "bench",
                    "source_categories": "amenity",
                    "cities": "Beijing",
                    "total_count": 2,
                    "area_m2_sum": 10.0,
                },
            ]
        )

        mapped = module.classify_dataframe(source)
        export_df = module.build_export_mapping_dataframe(mapped)

        self.assertEqual(
            list(export_df.columns),
            [
                "tag",
                "tag_zh",
                "primary_code",
                "primary",
                "secondary_code",
                "secondary",
                "tertiary_code",
                "tertiary",
                "weight_l1",
                "weight_l2",
                "weight_l3",
                "alt_candidates_json",
                "use_for_landuse_classification",
                "matched_by",
                "comment",
            ],
        )
        self.assertNotIn("cities", export_df.columns)
        self.assertNotIn("total_count", export_df.columns)
        self.assertNotIn("area_m2_sum", export_df.columns)
        restaurant = export_df[export_df["tag"] == "restaurant"].iloc[0]
        self.assertEqual(restaurant["tag_zh"], "餐馆")
        self.assertEqual(restaurant["primary_code"], "09")
        self.assertEqual(restaurant["secondary_code"], "0901")
        self.assertEqual(restaurant["tertiary_code"], "090103")
        self.assertEqual(restaurant["tertiary"], "餐饮用地")

    def test_export_includes_chinese_tag_name_for_building_tags(self):
        module = load_module()
        source = pd.DataFrame(
            [
                {
                    "value": "apartment",
                    "source_categories": "building",
                    "cities": "Beijing",
                    "total_count": 5,
                    "area_m2_sum": 100.0,
                },
                {
                    "value": "warehouse",
                    "source_categories": "building",
                    "cities": "Beijing",
                    "total_count": 2,
                    "area_m2_sum": 80.0,
                },
            ]
        )

        mapped = module.classify_dataframe(source)
        export_df = module.build_export_mapping_dataframe(mapped)

        by_tag = export_df.set_index("tag")
        self.assertEqual(by_tag.loc["apartment", "tag_zh"], "公寓住宅")
        self.assertEqual(by_tag.loc["warehouse", "tag_zh"], "仓库")

    def test_all_source_tags_have_chinese_names(self):
        module = load_module()
        source = pd.read_csv(module.INPUT_CSV)

        untranslated = [
            value
            for value in source["value"].astype(str).unique()
            if module.tag_to_chinese_name(value).startswith("未翻译标签")
        ]

        self.assertEqual(untranslated, [])

    def test_export_keeps_tertiary_empty_when_tag_cannot_support_it(self):
        module = load_module()
        source = pd.DataFrame(
            [
                {
                    "value": "government",
                    "source_categories": "amenity|building",
                    "cities": "Beijing",
                    "total_count": 3,
                    "area_m2_sum": 100.0,
                }
            ]
        )

        mapped = module.classify_dataframe(source)
        export_df = module.build_export_mapping_dataframe(mapped)
        government = export_df.iloc[0]

        self.assertEqual(government["secondary_code"], "0801")
        self.assertEqual(government["secondary"], "机关团体用地")
        self.assertEqual(government["tertiary_code"], "")
        self.assertEqual(government["tertiary"], "")
        self.assertEqual(government["weight_l3"], "")

    def test_low_weight_export_includes_alternative_candidates(self):
        module = load_module()
        source = pd.DataFrame(
            [
                {
                    "value": "garage",
                    "source_categories": "building|amenity",
                    "cities": "Beijing",
                    "total_count": 2,
                    "area_m2_sum": 10.0,
                }
            ]
        )

        mapped = module.classify_dataframe(source)
        export_df = module.build_export_mapping_dataframe(mapped)
        garage = export_df.iloc[0]
        alternatives = json.loads(garage["alt_candidates_json"])

        self.assertLess(float(garage["weight_l1"]), 0.75)
        self.assertGreaterEqual(len(alternatives), 1)
        self.assertTrue({"primary_code", "secondary_code", "weight_l1", "basis"}.issubset(alternatives[0]))

    def test_llm_fallback_overrides_low_confidence_tag(self):
        module = load_module()
        calls = []

        def fake_request(config, value, source_categories, semantic_result):
            calls.append((value, source_categories, semantic_result["primary_code"]))
            return {
                "primary_code": "12",
                "secondary_code": "1208",
                "tertiary_code": "120803",
                "weight_l1": 0.93,
                "weight_l2": 0.91,
                "weight_l3": 0.9,
                "alt_candidates_json": [],
                "comment": "LLM judged garage here as social parking/transport support.",
            }

        module.request_llm_landuse_classification = fake_request
        config = module.LLMFallbackConfig(
            base_url="http://test/v1",
            api_key="test-key",
            model="gpt-test",
        )

        result = module.classify_tag("garage", "building|amenity", llm_config=config)

        self.assertEqual(result["primary_code"], "12")
        self.assertEqual(result["secondary_code"], "1208")
        self.assertEqual(result["tertiary_code"], "120803")
        self.assertEqual(result["matched_by"], "llm_openai_fallback")
        self.assertEqual(len(calls), 1)

    def test_llm_fallback_skips_high_confidence_tag(self):
        module = load_module()
        calls = []

        def fake_request(config, value, source_categories, semantic_result):
            calls.append(value)
            raise AssertionError("LLM should not be called for high-confidence tags")

        module.request_llm_landuse_classification = fake_request
        config = module.LLMFallbackConfig(
            base_url="http://test/v1",
            api_key="test-key",
            model="gpt-test",
        )

        result = module.classify_tag("restaurant", "amenity", llm_config=config)

        self.assertEqual(result["primary_code"], "09")
        self.assertEqual(result["tertiary_code"], "090103")
        self.assertTrue(str(result["matched_by"]).startswith("semantic"))
        self.assertEqual(calls, [])


if __name__ == "__main__":
    unittest.main()
