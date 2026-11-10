import unittest

import pandas as pd

from server.city_process.cluster_primary_merged_archetypes import (
    build_pca_projection,
    build_classification_text,
    compute_clustering_metrics,
    normalize_primary_features,
    to_bestauto_prototypes,
)


class PrimaryMergedArchetypeTests(unittest.TestCase):
    def test_normalize_primary_features_excludes_unmapped_and_rescales(self):
        df = pd.DataFrame(
            {
                "_block_id": ["a", "b"],
                "primary_pct_居住用地": [30.0, 0.0],
                "primary_pct_商业服务业用地": [20.0, 0.0],
                "primary_pct_未映射": [50.0, 0.0],
            }
        )

        norm, feature_cols = normalize_primary_features(df, include_unmapped=False)

        self.assertEqual(feature_cols, ["primary_pct_居住用地", "primary_pct_商业服务业用地"])
        self.assertAlmostEqual(norm.loc[0, "primary_pct_居住用地"], 60.0)
        self.assertAlmostEqual(norm.loc[0, "primary_pct_商业服务业用地"], 40.0)
        self.assertAlmostEqual(norm.loc[1, "primary_pct_居住用地"], 0.0)

    def test_build_classification_text_describes_dominant_and_secondary_basis(self):
        row = pd.Series(
            {
                "primary_pct_居住用地": 62.0,
                "primary_pct_商业服务业用地": 24.0,
                "primary_pct_公共管理与公共服务用地": 14.0,
            }
        )

        result = build_classification_text(
            row,
            [
                "primary_pct_居住用地",
                "primary_pct_商业服务业用地",
                "primary_pct_公共管理与公共服务用地",
            ],
        )

        self.assertEqual(result["macro_class"], "居住用地")
        self.assertEqual(result["micro_class"], "居住用地主导型")
        self.assertIn("居住用地占比62.00%", result["classification_criteria"])
        self.assertIn("商业服务业用地占比24.00%", result["classification_criteria"])

    def test_to_bestauto_prototypes_uses_original_prototype_columns(self):
        catalog = pd.DataFrame(
            {
                "primary_cluster_id": [3],
                "macro_primary": ["居住用地"],
                "block_count": [12],
                "primary_signature": ["居住用地:70.00% | 商业服务业用地:30.00%"],
                "classification_criteria": ["居住用地占比70.00%，构成主要用地。"],
                "primary_pct_居住用地": [70.0],
            }
        )

        result = to_bestauto_prototypes(catalog)

        self.assertEqual(
            list(result.columns[:5]),
            ["cluster_id", "macro_bucket", "block_count_x", "top_signature", "block_count_y"],
        )
        self.assertEqual(result.loc[0, "cluster_id"], 3)
        self.assertEqual(result.loc[0, "macro_bucket"], "居住用地")
        self.assertEqual(result.loc[0, "top_signature"], "居住用地:70.00% | 商业服务业用地:30.00%")

    def test_compute_clustering_metrics_reports_standard_scores(self):
        x = pd.DataFrame(
            {
                "primary_pct_a": [90.0, 86.0, 8.0, 5.0],
                "primary_pct_b": [10.0, 14.0, 92.0, 95.0],
            }
        )
        labels = pd.Series([0, 0, 1, 1])

        metrics, per_cluster = compute_clustering_metrics(x, labels)

        self.assertEqual(metrics["sample_count"], 4)
        self.assertEqual(metrics["cluster_count"], 2)
        self.assertIn("silhouette_score", metrics)
        self.assertGreater(metrics["silhouette_score"], 0.0)
        self.assertEqual(set(per_cluster["cluster_id"]), {0, 1})
        self.assertIn("within_cluster_ss", per_cluster.columns)

    def test_build_pca_projection_returns_axis_metadata(self):
        x = pd.DataFrame(
            {
                "primary_pct_a": [90.0, 86.0, 8.0, 5.0],
                "primary_pct_b": [10.0, 14.0, 92.0, 95.0],
                "primary_pct_c": [0.0, 0.0, 0.0, 0.0],
            }
        )
        labels = pd.Series([0, 0, 1, 1])

        coords, summary = build_pca_projection(x, labels)

        self.assertEqual(list(coords.columns[:3]), ["pca_x", "pca_y", "cluster_id"])
        self.assertEqual(len(coords), 4)
        self.assertIn("pca_x_explained_variance_ratio", summary)
        self.assertGreater(summary["pca_x_explained_variance_ratio"], 0.0)


if __name__ == "__main__":
    unittest.main()
