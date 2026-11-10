import importlib.util
import pathlib
import sys
import unittest
from unittest import mock


MODULE_PATH = pathlib.Path(__file__).resolve().with_name("query_nearby_cities.py")
SERVER_QUERY_ROOT = MODULE_PATH.parents[1]
SERVER_ROOT = MODULE_PATH.parents[2]
sys.path.insert(0, str(SERVER_QUERY_ROOT))
sys.path.insert(0, str(SERVER_ROOT))


def load_module():
    spec = importlib.util.spec_from_file_location("query_nearby_cities_test", MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class QueryNearbyCitiesTests(unittest.TestCase):
    def test_center_query_outputs_named_center_set(self):
        module = load_module()
        query = module._build_center_query("Beijing")

        self.assertIn(")->.center_city;", query)
        self.assertIn(".center_city out tags center;", query)

    def test_candidates_query_uses_admin_level_5_relations_by_default(self):
        module = load_module()
        query = module._build_candidates_query({"lat": 39.9, "lon": 116.4}, 50000, admin_level_pattern="5")

        self.assertIn('relation(around:50000,39.9,116.4)["boundary"="administrative"]', query)
        self.assertIn('["admin_level"~"5"]', query)
        self.assertNotIn('node(around:', query)

    def test_dedupe_and_rank_filters_outside_fixed_radius_and_center_city(self):
        module = load_module()
        center = {
            "id": 1,
            "lat": 39.9,
            "lon": 116.4,
            "name": "Beijing",
            "name_en": "Beijing",
            "name_zh": "Beijing",
        }
        candidates = [
            center,
            {"id": 2, "lat": 39.53, "lon": 116.70, "name_en": "Langfang", "admin_level": "5"},
            {"id": 3, "lat": 34.50, "lon": 109.50, "name_en": "Weinan City", "admin_level": "5"},
        ]

        result = module._dedupe_and_rank(center, candidates, target_count=10, max_distance_m=100000)

        self.assertEqual([item["name_en"] for item in result], ["Langfang"])
        self.assertLessEqual(result[0]["distance_m"], 100000)

    def test_extract_candidates_keeps_relations_only(self):
        module = load_module()
        payload = {
            "elements": [
                {"type": "node", "id": 1, "lat": 39.9, "lon": 116.4, "tags": {"name:en": "Haidian", "place": "city"}},
                {"type": "relation", "id": 2, "center": {"lat": 39.5, "lon": 116.7}, "tags": {"name:en": "Langfang", "admin_level": "5"}},
            ]
        }

        result = module._extract_candidates(payload)

        self.assertEqual([item["name_en"] for item in result], ["Langfang"])

    def test_discover_accumulates_admin_level_5_relations(self):
        module = load_module()
        center_payload = {
            "elements": [
                {
                    "type": "node",
                    "id": 1,
                    "lat": 39.9,
                    "lon": 116.4,
                    "tags": {"name:en": "Beijing", "name": "Beijing"},
                },
            ]
        }
        empty_payload = {"elements": []}
        payload = {
            "elements": [
                {
                    "type": "relation",
                    "id": 2,
                    "center": {"lat": 39.53, "lon": 116.70},
                    "tags": {"name:en": "Langfang", "admin_level": "5"},
                },
                {
                    "type": "relation",
                    "id": 3,
                    "center": {"lat": 34.50, "lon": 109.50},
                    "tags": {"name:en": "Weinan City", "admin_level": "5"},
                },
            ]
        }

        with mock.patch.object(
            module,
            "_query_json_local_or_remote",
            side_effect=[center_payload, empty_payload, payload, payload],
        ) as query_mock:
            result = module.discover_nearby_cities(
                "Beijing",
                target_count=10,
                max_radius_m=150000,
                radius_step_m=50000,
            )

        self.assertEqual(query_mock.call_count, 4)
        self.assertEqual(result["search_radius_used_m"], 150000)
        self.assertEqual([item["name_en"] for item in result["nearby_cities"]], ["Langfang"])


if __name__ == "__main__":
    unittest.main()
