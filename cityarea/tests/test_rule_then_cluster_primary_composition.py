import unittest

import pandas as pd

from server.city_process.rule_then_cluster_primary_composition import split_high_purity_blocks
from server.city_process.rule_then_cluster_primary_composition import eligible_high_purity_names


class RuleThenClusterPrimaryCompositionTests(unittest.TestCase):
    def test_split_high_purity_blocks_uses_strict_threshold(self):
        df = pd.DataFrame(
            {
                "primary_pct_居住用地": [91.0, 90.0, 10.0],
                "primary_pct_商业服务业用地": [9.0, 10.0, 90.0],
            }
        )

        result = split_high_purity_blocks(df, list(df.columns), threshold=90.0)

        self.assertEqual(result["is_high_purity"].tolist(), [True, False, False])
        self.assertEqual(result["dominant_primary"].tolist(), ["居住用地", "居住用地", "商业服务业用地"])
        self.assertEqual(result["dominant_pct"].tolist(), [91.0, 90.0, 90.0])

    def test_eligible_high_purity_names_excludes_singleton_rule_classes(self):
        split = pd.DataFrame(
            {
                "dominant_primary": ["居住用地", "居住用地", "仓储用地"],
                "is_high_purity": [True, True, True],
            }
        )

        result = eligible_high_purity_names(split, min_count=2)

        self.assertEqual(result, {"居住用地"})


if __name__ == "__main__":
    unittest.main()
