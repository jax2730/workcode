import unittest

import pandas as pd

from server.city_process.topk_signature_reconstruction import (
    evaluate_reconstruction,
    make_topk_signature,
)


class TopKSignatureReconstructionTests(unittest.TestCase):
    def test_make_topk_signature_keeps_ordered_major_components(self):
        row = pd.Series(
            {
                "primary_pct_a": 62.0,
                "primary_pct_b": 4.0,
                "primary_pct_c": 34.0,
            }
        )

        result = make_topk_signature(row, list(row.index), top_k=3, min_component_pct=5.0)

        self.assertEqual(result, "a + c")

    def test_evaluate_reconstruction_measures_top_overlap_and_tv_error(self):
        x = pd.DataFrame(
            {
                "primary_pct_a": [80.0, 70.0, 5.0],
                "primary_pct_b": [20.0, 25.0, 90.0],
                "primary_pct_c": [0.0, 5.0, 5.0],
            }
        )
        labels = pd.Series(["a+b", "a+b", "b"])

        metrics, per_cluster = evaluate_reconstruction(x, labels, list(x.columns))

        self.assertGreater(metrics["top1_accuracy"], 0.9)
        self.assertGreater(metrics["top2_overlap_mean"], 0.8)
        self.assertLess(metrics["tv_error_mean"], 10.0)
        self.assertEqual(set(per_cluster["cluster_key"]), {"a+b", "b"})


if __name__ == "__main__":
    unittest.main()
