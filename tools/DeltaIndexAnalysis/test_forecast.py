# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

from datetime import datetime, timedelta, timezone
import math
from types import SimpleNamespace
import unittest

import analyze
import forecast


class ForecastTests(unittest.TestCase):
    TIMESTAMP = datetime(2026, 10, 1, tzinfo=timezone.utc)

    def history(self, ages=(0, 1, 2, 3), sizes=None, baseline=100, timestamp=None):
        sizes = tuple(2 * age for age in ages) if sizes is None else tuple(sizes)
        areas = [0]
        for index in range(1, len(ages)):
            areas.append(areas[-1] + sizes[index - 1] * (ages[index] - ages[index - 1]))
        return analyze.GrowthHistory(
            self.TIMESTAMP if timestamp is None else timestamp, baseline, tuple(ages), sizes, tuple(areas))

    def distribution(self):
        return forecast.DownloadDistribution([{"days": 1, "weight": 1}])

    def test_linear_fit_weights_elapsed_time_and_bounds_growth(self):
        fit = forecast.fit_linear((0, 1, 4), (0, 1, 2))
        self.assertAlmostEqual(fit.intercept, 0.45)
        self.assertAlmostEqual(fit.slope, 0.4)
        decreasing = forecast.fit_linear((0, 1, 2), (3, 2, 1))
        self.assertTrue(decreasing.constrained)
        self.assertAlmostEqual(decreasing.predict(100), 2)
        negative_intercept = forecast.fit_linear((0, 1, 2), (0, 1, 4))
        self.assertTrue(negative_intercept.constrained)
        self.assertAlmostEqual(negative_intercept.intercept, 0)
        self.assertAlmostEqual(negative_intercept.slope, 5 / 3)

    def test_histories_have_equal_weight_not_equal_sample_weight(self):
        first = self.history()
        ages = tuple(range(9))
        second = self.history(ages=ages, sizes=[20 + 4 * age for age in ages], baseline=300)
        model = forecast.fit_model([first, second])
        self.assertAlmostEqual(model.intercept, 10)
        self.assertAlmostEqual(model.slope, 3)
        self.assertAlmostEqual(model.baseline_bytes, 200)
        override = forecast.fit_model([first, second], baseline_override=500)
        self.assertAlmostEqual(override.baseline_bytes, 500)
        self.assertAlmostEqual(override.average_delta(10), model.average_delta(10))

    def test_optimum_is_forecast_beyond_observations_and_between_buckets(self):
        model = forecast.fit_model([self.history()])
        optimum = forecast.optimize(model, self.distribution(), 0.25, 30)
        # With size(t)=2t, a 100-byte baseline, and daily downloads: cost(P)=P+100/P for P>=1.
        self.assertAlmostEqual(optimum.period, 10)
        self.assertAlmostEqual(optimum.cost, 20)
        self.assertIsNone(optimum.boundary)

    def test_search_boundary_is_reported_instead_of_claiming_global_optimum(self):
        model = forecast.fit_model([self.history()])
        maximum = 5
        optimum = forecast.optimize(model, self.distribution(), 0.25, maximum)
        self.assertEqual(optimum.boundary, "upper")
        self.assertAlmostEqual(optimum.period, maximum)

    def test_download_probability_includes_new_clients_and_zero_staleness(self):
        stale_weight, new_weight = 0.6, 0.2
        distribution = forecast.DownloadDistribution([
            {"days": 0, "weight": 0.2},
            {"days": 1, "weight": stale_weight},
            {"new_client": True, "weight": new_weight},
        ])
        self.assertAlmostEqual(distribution.probability(4), stale_weight / 4 + new_weight)

    def test_near_optimal_range_uses_cost_tolerance_not_bootstrap_spread(self):
        model = forecast.fit_model([self.history()])
        ranges = forecast.near_optimal_ranges(model, self.distribution(), 0.25, 30, 0.01)
        self.assertEqual(len(ranges), 1)
        threshold = 20 * 1.01
        root = math.sqrt(threshold ** 2 - 400)
        self.assertAlmostEqual(ranges[0][0], (threshold - root) / 2)
        self.assertAlmostEqual(ranges[0][1], (threshold + root) / 2)

    def test_bootstrap_preserves_overlap_dependence_and_is_reproducible(self):
        ages = tuple(range(9))
        history = self.history(ages=ages, sizes=[2 * age + age % 2 for age in ages])
        model = forecast.fit_model([history, history])
        options = dict(samples=100, block_days=2, seed=7)
        ensemble = forecast.bootstrap(model, **options)
        repeated = forecast.bootstrap(model, **options)
        self.assertIsNone(ensemble.reason)
        self.assertEqual(ensemble.group_count, 1)
        self.assertEqual(ensemble.draws, repeated.draws)
        for draw in ensemble.draws:
            self.assertEqual(draw.model.fits[0], draw.model.fits[1])
        lower, upper = ensemble.mean_interval(30, 0.95)
        self.assertGreater(upper, lower)

    def test_overlap_connected_groups_are_kept_together(self):
        first = self.history()
        overlapping = self.history(timestamp=self.TIMESTAMP + timedelta(days=2))
        separate = self.history(timestamp=self.TIMESTAMP + timedelta(days=10))
        self.assertEqual(forecast.history_groups([first, overlapping, separate]), [[0, 1], [2]])

    def test_insufficient_uncertainty_data_is_explicit(self):
        model = forecast.fit_model([self.history(ages=(0, 1, 2))])
        ensemble = forecast.bootstrap(model, samples=100, block_days=1, seed=7)
        self.assertFalse(ensemble.draws)
        self.assertIn("four samples", ensemble.reason)
        model = forecast.fit_model([self.history()])
        ensemble = forecast.bootstrap(model, samples=100, block_days=10, seed=7)
        self.assertFalse(ensemble.draws)
        self.assertIn("four calendar blocks", ensemble.reason)

    def test_calendar_cutoff_excludes_future_samples_and_lineages(self):
        history = self.history(ages=tuple(range(11)))
        future = self.history(timestamp=self.TIMESTAMP + timedelta(days=8))
        cutoff_days = 5
        cutoff = self.TIMESTAMP + timedelta(days=cutoff_days)
        training = forecast.training_histories([history, future], cutoff)
        self.assertEqual([index for index, _ in training], [0])
        self.assertEqual(training[0][1].ages_days[-1], cutoff_days)

    def test_backtesting_detects_a_change_not_visible_before_cutoff(self):
        ages = tuple(range(11))
        jump, cutoff_days = 100, 5
        history = self.history(ages=ages, sizes=[
            2 * age + (jump if age > cutoff_days else 0) for age in ages])
        result = forecast.backtest([history], samples=100, block_days=1, confidence=0.95, seed=7)
        first_cutoff = self.TIMESTAMP + timedelta(days=cutoff_days)
        points = [point for point in result.points if point.cutoff == first_cutoff]
        self.assertTrue(points)
        for point in points:
            self.assertAlmostEqual(point.error, -jump)
            self.assertFalse(point.covered)
        self.assertAlmostEqual(result.mean_size_errors[0], -79)

    def test_invalid_forecast_options_are_rejected(self):
        options = dict(min_days=1 / 24, max_days=365, block_days=1, confidence=0.95,
                       near_optimal_percent=1, bootstrap_samples=100)
        for invalid in ({"max_days": 0}, {"block_days": float("nan")},
                        {"confidence": 1}, {"bootstrap_samples": 99}):
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                analyze.validate_options(SimpleNamespace(**(options | invalid)))


if __name__ == "__main__":
    unittest.main()
