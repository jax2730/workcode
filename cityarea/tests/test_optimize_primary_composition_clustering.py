import unittest

import numpy as np
import pandas as pd

from server.city_process.optimize_primary_composition_clustering import (
    build_candidate_score,
    transform_composition,
)


class OptimizePrimaryCompositionClusteringTests(unittest.TestCase):
    def test_hellinger_transform_maps_percentages_to_unit_length_rows(self):
        x = pd.DataFrame(
            {
                "primary_pct_a": [100.0, 25.0],
                "primary_pct_b": [0.0, 75.0],
            }
        )

        transformed = transform_composition(x, method="hellinger")

        row_norms = np.sqrt((transformed**2).sum(axis=1))
        self.assertTrue(np.allclose(row_norms, 1.0))
        self.assertAlmostEqual(transformed.iloc[1, 0], 0.5)

    def test_clr_transform_centers_each_row_in_log_ratio_space(self):
        x = pd.DataFrame(
            {
                "primary_pct_a": [80.0, 30.0],
                "primary_pct_b": [20.0, 70.0],
            }
        )

        transformed = transform_composition(x, method="clr")

        self.assertTrue(np.allclose(transformed.mean(axis=1), 0.0))
        self.assertGreater(transformed.iloc[0, 0], transformed.iloc[0, 1])

    def test_candidate_score_penalizes_unstable_small_cluster_solutions(self):
        candidate = {
            "silhouette_score": 0.45,
            "davies_bouldin_score": 0.9,
            "calinski_harabasz_score": 500.0,
            "small_cluster_sample_share": 0.25,
            "cluster_size_cv": 2.0,
            "stability_ari_mean": 0.4,
        }
        score = build_candidate_score(candidate)

        self.assertGreaterEqual(score, 0.0)
        self.assertLess(score, 0.45)


if __name__ == "__main__":
    unittest.main()
