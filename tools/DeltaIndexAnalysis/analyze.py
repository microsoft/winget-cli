#!/usr/bin/env python3
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
"""
Predict baseline refresh timing using measured transfer sizes (Python 3.10+).

Usage:
    python analyze.py --growth growth.json --distribution distribution.json
    python analyze.py --growth growth.json --distribution weekly --output-chart chart.png
    python analyze.py --growth growth.json --distribution weekly --max-days 180 --block-days 2

Growth JSON (sizes are transferred bytes, not original/uncompressed bytes):
    {
      "baselines": [
        {
          "timestamp": "2026-10-01T00:00:00Z",
          "size_bytes": 13000000,
          "deltas": [
            {"timestamp": "2026-10-01T00:00:00Z", "size_bytes": 1000},
            {"timestamp": "2026-10-01T02:00:00Z", "size_bytes": 50000},
            {"timestamp": "2026-10-02T00:00:00Z", "size_bytes": 600000}
          ]
        }
      ]
    }

Supply one or more baseline histories, which may overlap. Timestamps must include a UTC
offset (or Z). Samples are sorted by timestamp; duplicates are rejected. Each
history must include a delta at the baseline timestamp and at least one later
sample. The fitted growth curve is extrapolated through --max-days (default 365).
The initial delta's measured transfer size is required, not assumed to be zero.

Download-frequency JSON (unchanged from the original analyzer):
    {
      "description": "Download events by age of the previous index",
      "buckets": [
        {"days": 0.5, "weight": 0.30},
        {"days": 7, "weight": 0.50},
        {"days": 30, "weight": 0.15},
        {"new_client": true, "weight": 0.05}
      ]
    }

Weights describe download events, not users; frequency is already incorporated.
Nonnegative weights are normalized. Built-in presets: daily_heavy, weekly, monthly.

For period P days, a download by a client D days stale needs a baseline with
probability min(D, P) / P. New clients always need one. Expected bytes/download:

    time_average_delta(P) + probability_needing_baseline(P) * baseline_bytes

FORECAST AND OPTIMIZATION
-------------------------
Each history gets a time-weighted least-squares line: size(age) = intercept + slope*age.
Both coefficients are constrained nonnegative; shrinking histories cannot imply
negative future sizes. Observation weights use half the time to each neighboring
sample. Histories are weighted equally when combining coefficients and baseline
sizes. The average delta over a P-day cycle is intercept + slope*P/2.

The optimizer searches --min-days (default one hour) through --max-days. Within
each staleness-bucket interval, expected cost is A + slope*P/2 + B/P. Its endpoints
and stationary point cover all possible minima, including between observations.
Near-optimal ranges use --near-optimal-percent (default 1% above minimum cost).
They can be disjoint; they are cost-tolerance ranges, not confidence intervals.

UNCERTAINTY AND BACKTESTING
---------------------------
--bootstrap-samples (default 500) controls reproducible simulations (--seed 42).
A calendar-block wild residual bootstrap uses one shared random sign per
--block-days (default 1) block across all overlapping histories. Entire groups of
overlapping histories are also resampled together to capture between-group
variation without treating overlapping histories as independent. Every simulated
set of fits is re-optimized to estimate uncertainty in the recommended interval.

Mean-growth bands describe uncertainty in the expected curve. Pointwise prediction
bands additionally sample a lineage and its time-weighted, centered residuals.
These assume stationary residual variation and independence between calendar
blocks; they do not model an accumulating random walk. At least four samples per
history and four calendar blocks are required for intervals. Few blocks/groups
still give weak estimates. A single overlap group cannot establish uncertainty
about the population of future lineages. --confidence defaults to 0.95.

Rolling-origin backtests use calendar cutoffs at 50%, 65%, and 80% of the observed
calendar span. No fit or bootstrap sees observations after its cutoff. Histories
need three training samples; only already-started histories are evaluated. Reports
include elapsed-time-weighted prediction errors and pointwise prediction-band
coverage by forecast horizon, plus errors in held-out window average transfer size.
Held-out averages treat publications as step functions. Folds can overlap and are
not independent experiments. Backtesting does not validate unobserved horizons.

The model assumes downloads are uniformly distributed within a refresh cycle and
the supplied histories represent future growth. It models one active baseline,
not overlapping baseline lineages or out-of-band baseline delivery. Savings use
the mean supplied baseline size as the full-index-per-download reference, not a
separately measured full-index timeline. --baseline-mb overrides every baseline
size for sensitivity analysis without scaling deltas (MB here means 1024**2 bytes,
as in the original analyzer).

All reported intervals are approximate and conditional on linear growth, the
block/residual assumptions, and the supplied download distribution. They cannot
quantify uncertainty about an unobserved change in growth pattern. Search-boundary
hits are reported explicitly; a bounded optimum is not evidence that longer
periods are worse. Only --output-chart requires matplotlib; everything else uses
the standard library. Other growth models and automatic model selection are deferred.
"""

import argparse
from bisect import bisect_left
from dataclasses import dataclass
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import sys
import textwrap

import forecast


BYTES_PER_MB = 1024 ** 2
SECONDS_PER_DAY = 86400

PRESETS = {
    "daily_heavy": {
        "description": "Assumption (download events): heavy automated/CI usage",
        "buckets": [
            {"days": 1, "weight": 0.80},
            {"days": 7, "weight": 0.15},
            {"days": 30, "weight": 0.04},
            {"days": 90, "weight": 0.01},
        ],
    },
    "weekly": {
        "description": "Assumption (download events): weekly updaters dominate",
        "buckets": [
            {"days": 1, "weight": 0.30},
            {"days": 7, "weight": 0.50},
            {"days": 30, "weight": 0.15},
            {"days": 90, "weight": 0.05},
        ],
    },
    "monthly": {
        "description": "Assumption (download events): infrequent updaters dominate",
        "buckets": [
            {"days": 1, "weight": 0.05},
            {"days": 7, "weight": 0.25},
            {"days": 30, "weight": 0.45},
            {"days": 90, "weight": 0.25},
        ],
    },
}


@dataclass(frozen=True)
class GrowthHistory:
    timestamp: datetime
    baseline_bytes: float
    ages_days: tuple
    delta_bytes: tuple
    cumulative_byte_days: tuple

    def average_delta(self, period_days):
        if not 0 < period_days <= self.ages_days[-1]:
            raise ValueError("Refresh period must be positive and within the history.")
        index = bisect_left(self.ages_days, period_days) - 1
        area = self.cumulative_byte_days[index]
        area += self.delta_bytes[index] * (period_days - self.ages_days[index])
        return area / period_days


def load_json(path):
    with Path(path).open(encoding="utf-8-sig") as stream:
        return json.load(stream)


def nonnegative_number(value, label):
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{label} must be a finite nonnegative number.")
    try:
        number = float(value)
    except OverflowError as error:
        raise ValueError(f"{label} is too large.") from error
    if not math.isfinite(number) or number < 0:
        raise ValueError(f"{label} must be a finite nonnegative number.")
    return number


def parse_timestamp(value, label):
    if not isinstance(value, str):
        raise ValueError(f"{label} must be an ISO 8601 timestamp with a UTC offset.")
    try:
        timestamp = datetime.fromisoformat(
            value[:-1] + "+00:00" if value.endswith("Z") else value)
    except ValueError as error:
        raise ValueError(f"{label} is not a valid ISO 8601 timestamp.") from error
    if timestamp.utcoffset() is None:
        raise ValueError(f"{label} must include a UTC offset or Z.")
    return timestamp.astimezone(timezone.utc)


def load_histories(path):
    data = load_json(path)
    baselines = data.get("baselines") if isinstance(data, dict) else None
    if not isinstance(baselines, list) or not baselines:
        raise ValueError("Growth JSON must contain a nonempty 'baselines' array.")

    histories = []
    for index, baseline in enumerate(baselines):
        label = f"baselines[{index}]"
        if not isinstance(baseline, dict):
            raise ValueError(f"{label} must be an object.")
        timestamp = parse_timestamp(baseline.get("timestamp"), f"{label}.timestamp")
        size = nonnegative_number(baseline.get("size_bytes"), f"{label}.size_bytes")
        if size == 0:
            raise ValueError(f"{label}.size_bytes must be positive.")
        deltas = baseline.get("deltas")
        if not isinstance(deltas, list) or len(deltas) < 2:
            raise ValueError(f"{label}.deltas must contain an initial and a later sample.")

        samples = []
        for sample_index, delta in enumerate(deltas):
            sample_label = f"{label}.deltas[{sample_index}]"
            if not isinstance(delta, dict):
                raise ValueError(f"{sample_label} must be an object.")
            sample_time = parse_timestamp(delta.get("timestamp"), f"{sample_label}.timestamp")
            sample_size = nonnegative_number(delta.get("size_bytes"), f"{sample_label}.size_bytes")
            age = (sample_time - timestamp).total_seconds() / SECONDS_PER_DAY
            if age < 0:
                raise ValueError(f"{sample_label} predates its baseline.")
            samples.append((age, sample_size))
        samples.sort()
        if samples[0][0] != 0:
            raise ValueError(f"{label} needs a delta at the baseline timestamp.")
        if any(left[0] == right[0] for left, right in zip(samples, samples[1:])):
            raise ValueError(f"{label} has duplicate delta timestamps.")

        areas = [0.0]
        for left, right in zip(samples, samples[1:]):
            areas.append(areas[-1] + left[1] * (right[0] - left[0]))
        if not math.isfinite(areas[-1]):
            raise ValueError(f"{label} sizes and duration are too large.")
        histories.append(GrowthHistory(
            timestamp, size, tuple(age for age, _ in samples),
            tuple(size for _, size in samples), tuple(areas)))
    return histories


def normalize_buckets(buckets):
    if not isinstance(buckets, list) or not buckets:
        raise ValueError("Distribution must contain a nonempty 'buckets' array.")
    normalized = []
    for index, bucket in enumerate(buckets):
        label = f"buckets[{index}]"
        if not isinstance(bucket, dict):
            raise ValueError(f"{label} must be an object.")
        result = {"weight": nonnegative_number(bucket.get("weight"), f"{label}.weight")}
        if "new_client" in bucket:
            if bucket["new_client"] is not True or "days" in bucket:
                raise ValueError(f"{label} must specify either days or new_client: true.")
            result["new_client"] = True
        else:
            result["days"] = nonnegative_number(bucket.get("days"), f"{label}.days")
        normalized.append(result)
    # Scale first so large, otherwise valid counts can also be normalized safely.
    largest = max(bucket["weight"] for bucket in normalized)
    if largest == 0:
        raise ValueError("Distribution must have a positive total weight.")
    total = math.fsum(bucket["weight"] / largest for bucket in normalized)
    if not math.isclose(largest, 1 / total, rel_tol=0.01):
        print("Warning: normalizing download-event weights to sum to 1.", file=sys.stderr)
    for bucket in normalized:
        bucket["weight"] = (bucket["weight"] / largest) / total
    return normalized


def load_distribution(argument):
    distribution = PRESETS[argument] if argument in PRESETS else load_json(argument)
    if not isinstance(distribution, dict):
        raise ValueError("Distribution JSON must be an object.")
    description = distribution.get("description", argument)
    if not isinstance(description, str):
        raise ValueError("Distribution description must be a string.")
    return normalize_buckets(distribution.get("buckets")), description


def fmt_days(days):
    if days < 1:
        return f"{days * 24:.3g}h"
    if days >= 7 and days % 7 == 0:
        return f"{days / 7:g}wk"
    return f"{days:.4g}d"


def _mb_precision(ticks):
    """Decimal places needed so adjacent axis ticks render as distinct labels."""
    spacing = min((abs(b - a) for a, b in zip(ticks, ticks[1:])), default=1.0)
    if spacing >= 1.0:
        return 0
    if spacing >= 0.1:
        return 1
    return 2


def print_backtest(result):
    print("\nChronological backtests (overlapping folds, not independent trials):")
    if not result.points:
        print("  Unavailable: insufficient training and held-out observations at the calendar cutoffs.")
        return
    print(f"  {result.folds} folds; errors are weighted by elapsed time within each held-out history.")
    print("  Horizon        Samples    MAE MiB    RMSE MiB    Prediction-band coverage")
    for lower, upper in ((0, 1), (1, 7), (7, 30), (30, math.inf)):
        points = [point for point in result.points if lower < point.horizon <= upper]
        if not points:
            continue
        weight = math.fsum(point.weight for point in points)
        mae = math.fsum(point.weight * abs(point.error) for point in points) / weight
        rmse = math.sqrt(math.fsum(point.weight * point.error ** 2 for point in points) / weight)
        covered = [point for point in points if point.covered is not None]
        coverage = (f"{100 * math.fsum(point.weight * point.covered for point in covered) / math.fsum(point.weight for point in covered):.1f}%"
                    if covered else "unavailable")
        label = f"{lower:g}-{upper:g}d" if math.isfinite(upper) else f">{lower:g}d"
        print(f"  {label:>10}    {len(points):>7}    {mae / BYTES_PER_MB:>7.4f}"
              f"    {rmse / BYTES_PER_MB:>8.4f}    {coverage} ({len(covered)} samples)")
    mean_mae = math.fsum(abs(error) for error in result.mean_size_errors) / len(result.mean_size_errors)
    print(f"  Held-out window average-size MAE: {mean_mae / BYTES_PER_MB:.4f} MiB (equal history-window weight)")
    print(f"  Longest backtested forecast horizon: {fmt_days(max(point.horizon for point in result.points))}")
    for reason in result.unavailable_intervals:
        print(f"  Prediction intervals unavailable at {reason}")


def _write_chart(path, model, distribution, optimal, ensemble, description, args):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        import matplotlib.ticker as ticker
    except ImportError as error:
        raise ValueError("--output-chart requires matplotlib; install it with pip install matplotlib.") from error

    figure, (growth_axis, cost_axis) = plt.subplots(2, 1, figsize=(13, 9))
    for history in model.histories:
        label = history.timestamp.isoformat()
        line, = growth_axis.step(
            history.ages_days, [size / BYTES_PER_MB for size in history.delta_bytes],
            where="post", label=f"Delta: {label}")
        size = history.baseline_bytes if model.baseline_override is None else model.baseline_override
        growth_axis.axhline(size / BYTES_PER_MB, linestyle=":", color=line.get_color(),
                           label=f"Baseline: {label}")
    ages = [args.max_days * index / 240 for index in range(241)]
    growth_axis.plot(ages, [model.predict(age) / BYTES_PER_MB for age in ages],
                     color="black", label="Fitted mean / linear forecast")
    if ensemble.draws:
        for label, method, color in (
                ("Pointwise prediction band", ensemble.prediction_interval, "#95A5A6"),
                ("Mean-growth confidence band", ensemble.mean_interval, "#2980B9")):
            bands = [method(age, args.confidence) for age in ages]
            growth_axis.fill_between(
                ages, [lower / BYTES_PER_MB for lower, _ in bands],
                [upper / BYTES_PER_MB for _, upper in bands],
                alpha=0.25, color=color, label=f"{args.confidence:.0%} {label}")
    observed_end = max(history.ages_days[-1] for history in model.histories)
    if args.max_days > observed_end:
        for axis in (growth_axis, cost_axis):
            axis.axvspan(observed_end, args.max_days, color="#F39C12", alpha=0.08,
                         label="Beyond all measured ages")
    growth_axis.axvline(optimal.period, linestyle="--", color="#2980B9", label="Recommended period")
    growth_axis.set_xlabel("Baseline age (days)")
    growth_axis.set_ylabel("Transfer size (MiB)")
    growth_axis.set_title("Measured growth, fitted mean, and model-conditional forecast")
    growth_axis.legend(loc="upper left")
    periods = sorted({args.min_days * (args.max_days / args.min_days) ** (index / 240)
                      for index in range(241)}
                     | set(distribution.candidates(model, args.min_days, args.max_days)))
    cost_axis.plot(periods, [distribution.cost(model, period) / BYTES_PER_MB for period in periods],
                   label="Expected transfer")
    if ensemble.draws:
        bands = [forecast.interval(
            [distribution.cost(draw.model, period) for draw in ensemble.draws], args.confidence)
            for period in periods]
        cost_axis.fill_between(periods, [lower / BYTES_PER_MB for lower, _ in bands],
                               [upper / BYTES_PER_MB for _, upper in bands], alpha=0.2,
                               label=f"{args.confidence:.0%} model-conditional cost band")
    cost_axis.axhline(model.baseline_bytes / BYTES_PER_MB, linestyle=":", color="#7F8C8D",
                     label="Full-index-per-download reference")
    cost_axis.axvline(optimal.period, linestyle="--", color="#2980B9",
                     label=f"Recommended: {fmt_days(optimal.period)}")
    cost_axis.set_xlabel("Baseline refresh period (days)")
    cost_axis.set_ylabel("MiB / download event")
    cost_axis.legend(loc="upper left")
    for axis in (growth_axis, cost_axis):
        axis.yaxis.set_major_formatter(ticker.FuncFormatter(
            lambda value, _, axis=axis: f"{value:.{_mb_precision(axis.get_yticks())}f}"))
    figure.suptitle("Baseline Timing Analysis\n" + "\n".join(textwrap.wrap(description, 110)))
    figure.tight_layout()
    try:
        figure.savefig(path, dpi=150)
    finally:
        plt.close(figure)
    print(f"Chart saved to: {path}")


def validate_options(args):
    for name in ("min_days", "max_days", "block_days", "confidence", "near_optimal_percent"):
        value = nonnegative_number(getattr(args, name), "--" + name.replace("_", "-"))
        if value == 0 and name != "near_optimal_percent":
            raise ValueError(f"--{name.replace('_', '-')} must be positive.")
    if args.max_days <= args.min_days:
        raise ValueError("--max-days must be greater than --min-days.")
    if args.confidence >= 1:
        raise ValueError("--confidence must be between 0 and 1.")
    if args.bootstrap_samples < 100:
        raise ValueError("--bootstrap-samples must be at least 100.")


def print_analysis(model, distribution, ensemble, optimal, description, args):
    observed = [history.ages_days[-1] for history in model.histories]
    print("\nBaseline Timing Analysis")
    print(f"Growth histories: {args.growth} ({len(model.histories)}; equal weight)")
    print(f"Distribution: {description} (fixed; its uncertainty is not modeled)")
    print(f"Baseline reference: {model.baseline_bytes / BYTES_PER_MB:.4f} MiB"
          + (" (override)" if model.baseline_override is not None else " (mean measured transfer size)"))
    print(f"Observed ages: common coverage {fmt_days(min(observed))}; longest history {fmt_days(max(observed))}")
    print(f"Search: {fmt_days(args.min_days)} to {fmt_days(args.max_days)}; linear extrapolation enabled")
    print("Model: time-weighted nonnegative linear fits; uniform download phase within a refresh cycle")
    for index, (history, fit) in enumerate(zip(model.histories, model.fits), 1):
        r_squared = f"{fit.r_squared:.4f}" if fit.r_squared is not None else "undefined (constant sizes)"
        print(f"  History {index}: {history.timestamp.isoformat()}, {len(history.ages_days)} samples")
        print(f"    intercept={fit.intercept / BYTES_PER_MB:.4f} MiB;"
              f" slope={fit.slope / BYTES_PER_MB:.4f} MiB/day;"
              f" weighted RMSE={fit.rmse / BYTES_PER_MB:.4f} MiB; R^2={r_squared}"
              + ("; nonnegative constraint active" if fit.constrained else ""))
    print(f"\nRecommendation within search range: {fmt_days(optimal.period)}")
    print(f"  {optimal.cost / BYTES_PER_MB:.4f} MiB / download event;"
          f" predicted reduction {100 * (1 - optimal.cost / model.baseline_bytes):.1f}%")
    beyond = sum(optimal.period > duration for duration in observed)
    print(f"  Exceeds measured ages of {beyond}/{len(observed)} histories.")
    if optimal.boundary:
        print(f"  WARNING: optimum hits the {optimal.boundary} search boundary; optimum outside the range is unresolved.")
    ranges = forecast.near_optimal_ranges(
        model, distribution, args.min_days, args.max_days, args.near_optimal_percent / 100)
    print(f"  Within {args.near_optimal_percent:g}% of minimum expected cost: "
          + ", ".join(f"{fmt_days(lower)} to {fmt_days(upper)}" for lower, upper in ranges))
    print("  This is a cost-tolerance range, not a confidence interval.")
    print(f"\nUncertainty: {ensemble.block_count} calendar blocks of {fmt_days(args.block_days)};"
          f" {ensemble.group_count} nonoverlapping lineage group(s).")
    if not ensemble.draws:
        print(f"  UNAVAILABLE: {ensemble.reason}")
        return
    print(f"  {len(ensemble.draws)} bootstrap draws; seed {args.seed}; nominal level {args.confidence:.0%}.")
    if ensemble.block_count < 10:
        print("  WARNING: few calendar blocks; uncertainty estimates may be unstable.")
    if ensemble.group_count == 1:
        print("  WARNING: one lineage group; between-group population uncertainty is not estimable.")
    elif ensemble.group_count < 5:
        print("  WARNING: few independent lineage groups; between-group estimates may be unstable.")
    optima = [forecast.optimize(draw.model, distribution, args.min_days, args.max_days)
              for draw in ensemble.draws]
    lower, upper = forecast.interval([result.period for result in optima], args.confidence)
    print(f"  Approximate model-conditional interval for optimal period: {fmt_days(lower)} to {fmt_days(upper)}")
    for boundary in ("lower", "upper"):
        hits = sum(result.boundary == boundary for result in optima)
        print(f"  Draws hitting {boundary} search boundary: {hits}/{len(optima)}"
              + (" (uncertainty is censored by the search limit)" if hits else ""))
    lower, upper = forecast.interval([
        100 * (1 - distribution.cost(draw.model, optimal.period) / draw.model.baseline_bytes)
        for draw in ensemble.draws], args.confidence)
    print(f"  Reduction at recommended period: {lower:.1f}% to {upper:.1f}% (model-conditional)")
    for label, values in (
            ("Mean slope (MiB/day)", [draw.model.slope / BYTES_PER_MB for draw in ensemble.draws]),
            ("Mean intercept (MiB)", [draw.model.intercept / BYTES_PER_MB for draw in ensemble.draws])):
        lower, upper = forecast.interval(values, args.confidence)
        print(f"  {label}: {lower:.4f} to {upper:.4f}")
    lower, upper = ensemble.mean_interval(optimal.period, args.confidence)
    print(f"  Mean delta size at recommended age: {lower / BYTES_PER_MB:.4f} to {upper / BYTES_PER_MB:.4f} MiB")
    lower, upper = ensemble.prediction_interval(optimal.period, args.confidence)
    print(f"  Future delta size at recommended age: {lower / BYTES_PER_MB:.4f} to {upper / BYTES_PER_MB:.4f} MiB")


def main():
    parser = argparse.ArgumentParser(
        description="Optimize baseline refresh timing using measured transfer sizes.",
        epilog="Built-in distribution presets: " + ", ".join(PRESETS))
    parser.add_argument("--growth", required=True, help="JSON file containing baseline growth histories.")
    parser.add_argument("--distribution", required=True,
                        help="Download-event distribution JSON file or preset name.")
    parser.add_argument("--baseline-mb", type=float,
                        help="Override all baseline transfer sizes in MiB; does not scale deltas.")
    parser.add_argument("--output-chart", help="Save a chart image (requires matplotlib).")
    parser.add_argument("--min-days", type=float, default=1 / 24,
                        help="Shortest refresh period to consider (default one hour).")
    parser.add_argument("--max-days", type=float, default=365,
                        help="Longest period to forecast and consider (default 365 days).")
    parser.add_argument("--block-days", type=float, default=1,
                        help="Shared calendar-block duration for the bootstrap (default 1 day).")
    parser.add_argument("--bootstrap-samples", type=int, default=500,
                        help="Number of bootstrap simulations, at least 100 (default 500).")
    parser.add_argument("--confidence", type=float, default=0.95,
                        help="Nominal model-conditional interval level (default 0.95).")
    parser.add_argument("--seed", type=int, default=42, help="Random seed (default 42).")
    parser.add_argument("--near-optimal-percent", type=float, default=1,
                        help="Cost tolerance above the minimum for near-optimal ranges (default 1%%).")
    args = parser.parse_args()

    try:
        validate_options(args)
        histories = load_histories(args.growth)
        buckets, description = load_distribution(args.distribution)
        baseline_override = None
        if args.baseline_mb is not None:
            baseline_override = nonnegative_number(args.baseline_mb, "--baseline-mb") * BYTES_PER_MB
            if baseline_override == 0 or not math.isfinite(baseline_override):
                raise ValueError("--baseline-mb must specify a finite positive transfer size.")
        model = forecast.fit_model(histories, baseline_override)
        distribution = forecast.DownloadDistribution(buckets)
        optimal = forecast.optimize(model, distribution, args.min_days, args.max_days)
        ensemble = forecast.bootstrap(model, args.bootstrap_samples, args.block_days, args.seed)
        print_analysis(model, distribution, ensemble, optimal, description, args)
        validation = forecast.backtest(
            histories, args.bootstrap_samples, args.block_days, args.confidence, args.seed)
        print_backtest(validation)
        print("\nIntervals assume linear growth, stationary residuals, and independence between calendar blocks.")
        print("They do not measure uncertainty about a future change in growth pattern.")
        print("Overlapping baseline delivery is not modeled; unobserved forecast horizons are not backtested.")
        if args.output_chart:
            _write_chart(args.output_chart, model, distribution, optimal, ensemble, description, args)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    except OverflowError:
        parser.error("Values exceed numerical limits; reduce sizes or the forecast range.")


if __name__ == "__main__":
    main()
