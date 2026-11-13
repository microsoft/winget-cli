# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

import json
from pathlib import Path
import tempfile
import unittest

import analyze


class AnalysisTests(unittest.TestCase):
    BASELINE_TIMESTAMP = "2026-10-01T00:00:00Z"

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / "growth.json"

    def load(self, baselines):
        self.path.write_text(json.dumps({"baselines": baselines}), encoding="utf-8")
        return analyze.load_histories(self.path)

    def baseline(self, size=100, deltas=None):
        return {
            "timestamp": self.BASELINE_TIMESTAMP,
            "size_bytes": size,
            "deltas": deltas if deltas is not None else [
                {"timestamp": self.BASELINE_TIMESTAMP, "size_bytes": 2},
                {"timestamp": "2026-10-01T02:00:00Z", "size_bytes": 14},
                {"timestamp": "2026-10-01T08:00:00Z", "size_bytes": 30},
            ],
        }

    def test_irregular_samples_use_elapsed_time_not_sample_count(self):
        history, = self.load([self.baseline()])
        self.assertAlmostEqual(history.average_delta(8 / 24), (2 * 2 + 14 * 6) / 8)
        self.assertAlmostEqual(history.average_delta(4 / 24), (2 * 2 + 14 * 2) / 4)
        self.assertAlmostEqual(history.average_delta(2 / 24), 2)

    def test_timezone_equivalence_and_unordered_samples(self):
        baseline = self.baseline()
        baseline["deltas"].reverse()
        baseline["deltas"][-1]["timestamp"] = "2026-09-30T17:00:00-07:00"
        history, = self.load([baseline])
        self.assertEqual(history.ages_days, (0, 2 / 24, 8 / 24))

    def test_invalid_growth_histories_are_rejected(self):
        cases = [
            self.baseline(size=0),
            self.baseline(size=float("nan")),
            self.baseline(deltas=[]),
            self.baseline(deltas=[
                {"timestamp": "2026-10-01T02:00:00Z", "size_bytes": 2},
                {"timestamp": "2026-10-01T08:00:00Z", "size_bytes": 30},
            ]),
        ]
        for value in (True, -1, float("inf")):
            baseline = self.baseline()
            baseline["deltas"][0]["size_bytes"] = value
            cases.append(baseline)
        for timestamp in ("2026-10-01T00:00:00", "2026-10-01T00:00:00+00:00",
                          "2026-09-30T23:00:00Z"):
            baseline = self.baseline()
            baseline["deltas"][1]["timestamp"] = timestamp
            cases.append(baseline)
        for baseline in cases:
            with self.subTest(baseline=baseline), self.assertRaises(ValueError):
                self.load([baseline])
        with self.assertRaises(ValueError):
            self.load([])

    def test_existing_distribution_json_is_normalized_by_events(self):
        distribution = {
            "description": "Measured downloads",
            "buckets": [{"days": 0.5, "weight": 30}, {"new_client": True, "weight": 10}],
        }
        self.path.write_text(json.dumps(distribution), encoding="utf-8")
        buckets, description = analyze.load_distribution(str(self.path))
        self.assertEqual(description, distribution["description"])
        self.assertEqual(buckets, [
            {"days": 0.5, "weight": 0.75}, {"new_client": True, "weight": 0.25}])

    def test_invalid_distributions_are_rejected(self):
        for buckets in ([], [{"days": 1, "weight": 0}], [{"days": -1, "weight": 1}],
                        [{"new_client": True, "days": 1, "weight": 1}],
                        [{"days": 1, "weight": float("inf")}]):
            with self.subTest(buckets=buckets), self.assertRaises(ValueError):
                analyze.normalize_buckets(buckets)


if __name__ == "__main__":
    unittest.main()
