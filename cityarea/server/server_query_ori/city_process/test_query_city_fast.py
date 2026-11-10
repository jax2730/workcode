import importlib.util
import pathlib
import sys
import types
import unittest


SERVER_ROOT = pathlib.Path(__file__).resolve().parents[2]
MODULE_PATH = SERVER_ROOT / "server_query_ori" / "city_process" / "query_city_fast.py"

sys.path.insert(0, str(SERVER_ROOT))
sys.path.insert(0, str(SERVER_ROOT / "server_query_ori"))


def load_query_city_fast_module():
    osm_util = types.ModuleType("overpass_query.osm_util")
    osm_util.convert_city_osm_2_gpkg = lambda *args, **kwargs: None
    osm_util.query_osm_data_local = lambda *args, **kwargs: 0
    osm_util.query_osm_data_local_str = lambda *args, **kwargs: ""

    overpass_query = types.ModuleType("overpass_query")
    overpass_query.osm_util = osm_util

    common_db = types.ModuleType("common.db")
    common_db.SessionLocal = lambda: None

    common_model = types.ModuleType("common.model")
    common_model.QueryTask = object

    sys.modules["overpass_query"] = overpass_query
    sys.modules["overpass_query.osm_util"] = osm_util
    sys.modules["common.db"] = common_db
    sys.modules["common.model"] = common_model

    spec = importlib.util.spec_from_file_location("query_city_fast", MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class QueryCityFastTest(unittest.TestCase):
    def test_fast_query_uses_city_area_and_center_radius(self):
        module = load_query_city_fast_module()
        query = module._road_query_by_city_area_and_center("Cangzhou", 38.3, 116.8, 25000)

        self.assertIn('map_to_area -> .city_area', query)
        self.assertIn('nwr(area.city_area)(around:25000,38.3,116.8)', query)
        self.assertIn('node(area.city_area)(around:25000,38.3,116.8)["place"]', query)
        self.assertIn('node(area.city_area)(around:25000,38.3,116.8)["amenity"]', query)

    def test_query_city_center_query_includes_label_role(self):
        module = load_query_city_fast_module()
        query = module._relation_member_node_query("Baoding", "label")

        self.assertIn('node(r.city_rel:"label")', query)

    def test_city_relation_query_accepts_city_suffix_variant(self):
        module = load_query_city_fast_module()
        query = module._city_relation_set_query("Baoding")

        self.assertIn('["name:en"="Baoding"]', query)
        self.assertIn('["name:en"="Baoding City"]', query)

    def test_query_city_center_prefers_label_node(self):
        module = load_query_city_fast_module()
        module.query_osm_data_local_str = lambda query, log=False: """
        <osm>
          <node id="1" lat="38.30" lon="116.80">
            <tag k="name:en" v="Cangzhou"/>
          </node>
          <relation id="2">
            <center lat="39.00" lon="117.00"/>
            <tag k="admin_level" v="5"/>
            <tag k="name:en" v="Cangzhou"/>
          </relation>
        </osm>
        """

        center = module._query_city_admin_centre_info("Cangzhou")
        self.assertEqual((center["lat"], center["lon"]), (38.3, 116.8))
        self.assertEqual(center["source"], "label")

    def test_query_city_center_falls_back_to_admin_centre_node(self):
        module = load_query_city_fast_module()
        responses = iter([
            "",
            """
            <osm>
              <node id="1" lat="38.30" lon="116.80">
                <tag k="name:en" v="Cangzhou"/>
              </node>
              <relation id="2">
                <center lat="39.00" lon="117.00"/>
                <tag k="admin_level" v="5"/>
                <tag k="name:en" v="Cangzhou"/>
              </relation>
            </osm>
            """,
        ])
        module.query_osm_data_local_str = lambda query, log=False: next(responses)

        center = module._query_city_admin_centre_info("Cangzhou")
        self.assertEqual((center["lat"], center["lon"]), (38.3, 116.8))
        self.assertEqual(center["source"], "admin_centre")

    def test_query_city_admin_centre_falls_back_to_relation_center(self):
        module = load_query_city_fast_module()
        responses = iter([
            "",
            "",
            """
            <osm>
              <relation id="2">
                <center lat="39.00" lon="117.00"/>
                <tag k="admin_level" v="5"/>
                <tag k="name:en" v="Cangzhou"/>
              </relation>
            </osm>
            """,
        ])
        module.query_osm_data_local_str = lambda query, log=False: next(responses)

        center = module._query_city_admin_centre_info("Cangzhou")
        self.assertEqual((center["lat"], center["lon"]), (39.0, 117.0))
        self.assertEqual(center["source"], "relation_center")


if __name__ == "__main__":
    unittest.main()
