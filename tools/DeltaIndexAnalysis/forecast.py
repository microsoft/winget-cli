# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
"""Linear growth forecasts and model-conditional uncertainty, using only the standard library."""

from bisect import bisect_left, bisect_right
from dataclasses import dataclass, replace
from datetime import datetime, timedelta
import math
import random


SECONDS_PER_DAY = 86400


@dataclass(frozen=True)
class LinearFit:
    intercept: float
    slope: float
    rmse: float
    r_squared: float | None
    residuals: tuple
    weights: tuple
    constrained: bool

    def predict(self, age):
        return self.intercept + self.slope * age

    def average_delta(self, period):
        return self.intercept + self.slope * period / 2


def observation_weights(ages):
    """Trapezoidal time weights keep dense publication bursts from dominating a fit."""
    duration = ages[-1] - ages[0]
    return tuple(
        ((ages[index + 1] - ages[index] if index + 1 < len(ages) else 0)
         + (ages[index] - ages[index - 1] if index else 0)) / (2 * duration)
        for index in range(len(ages)))


def fit_linear(ages, sizes):
    if len(ages) < 2 or len(ages) != len(sizes) or any(
            right <= left for left, right in zip(ages, ages[1:])):
        raise ValueError("A linear fit needs at least two distinct, increasing sample ages.")
    weights = observation_weights(ages)
    mean_x = math.fsum(weight * age for weight, age in zip(weights, ages))
    mean_y = math.fsum(weight * size for weight, size in zip(weights, sizes))
    variance = math.fsum(weight * (age - mean_x) ** 2 for weight, age in zip(weights, ages))
    covariance = math.fsum(weight * (age - mean_x) * (size - mean_y)
                           for weight, age, size in zip(weights, ages, sizes))
    slope = covariance / variance
    intercept = mean_y - slope * mean_x

    # Solve the two-parameter nonnegative least-squares problem, including its boundaries.
    candidates = [(max(0, mean_y), 0)]
    x_squared = math.fsum(weight * age ** 2 for weight, age in zip(weights, ages))
    candidates.append((0, max(0, math.fsum(
        weight * age * size for weight, age, size in zip(weights, ages, sizes)) / x_squared)))
    if intercept >= 0 and slope >= 0:
        candidates.append((intercept, slope))
    fitted_intercept, fitted_slope = min(candidates, key=lambda parameters: math.fsum(
        weight * (size - parameters[0] - parameters[1] * age) ** 2
        for weight, age, size in zip(weights, ages, sizes)))
    residuals = tuple(size - fitted_intercept - fitted_slope * age for age, size in zip(ages, sizes))
    error = math.fsum(weight * residual ** 2 for weight, residual in zip(weights, residuals))
    total = math.fsum(weight * (size - mean_y) ** 2 for weight, size in zip(weights, sizes))
    return LinearFit(fitted_intercept, fitted_slope, math.sqrt(error),
                     1 - error / total if total > 0 else None, residuals, weights,
                     intercept < 0 or slope < 0)


@dataclass(frozen=True)
class LinearForecast:
    histories: tuple
    fits: tuple
    baseline_bytes: float
    intercept: float
    slope: float
    baseline_override: float | None

    def predict(self, age):
        return self.intercept + self.slope * age

    def average_delta(self, period):
        return self.intercept + self.slope * period / 2

    def stationary_periods(self, moment, left, right):
        if self.slope == 0:
            return ()
        period = math.sqrt(2 * self.baseline_bytes * moment / self.slope)
        return (period,) if left < period < right else ()


def make_forecast(histories, fits, baseline_override=None):
    count = len(histories)
    return LinearForecast(
        tuple(histories), tuple(fits),
        baseline_override if baseline_override is not None else
        math.fsum(history.baseline_bytes / count for history in histories),
        math.fsum(fit.intercept / count for fit in fits),
        math.fsum(fit.slope / count for fit in fits), baseline_override)


def fit_model(histories, baseline_override=None):
    return make_forecast(histories, [
        fit_linear(history.ages_days, history.delta_bytes) for history in histories], baseline_override)


class DownloadDistribution:
    def __init__(self, buckets):
        weights = {}
        self.new_client_weight = 0
        for bucket in buckets:
            if bucket.get("new_client"):
                self.new_client_weight += bucket["weight"]
            else:
                weights[bucket["days"]] = weights.get(bucket["days"], 0) + bucket["weight"]
        self.ages = tuple(sorted(weights))
        self.mass = [0.0]
        self.moment = [0.0]
        for age in self.ages:
            self.mass.append(self.mass[-1] + weights[age])
            self.moment.append(self.moment[-1] + weights[age] * age)

    def probability(self, period):
        index = bisect_left(self.ages, period)
        return (self.new_client_weight + self.mass[-1] - self.mass[index]
                + self.moment[index] / period)

    def cost(self, model, period):
        return model.average_delta(period) + self.probability(period) * model.baseline_bytes

    def candidates(self, model, minimum, maximum):
        boundaries = sorted({minimum, maximum}
                            | {age for age in self.ages if minimum < age < maximum})
        candidates = set(boundaries)
        for left, right in zip(boundaries, boundaries[1:]):
            moment = self.moment[bisect_right(self.ages, left)]
            candidates.update(model.stationary_periods(moment, left, right))
        return sorted(candidates)


@dataclass(frozen=True)
class Optimum:
    period: float
    cost: float
    boundary: str | None


def optimize(model, distribution, minimum, maximum):
    candidates = distribution.candidates(model, minimum, maximum)
    costs = [(period, distribution.cost(model, period)) for period in candidates]
    if not all(math.isfinite(cost) for _, cost in costs):
        raise ValueError("Forecast exceeds the finite numerical range; reduce sizes or --max-days.")
    period, cost = min(costs, key=lambda item: (item[1], item[0]))
    boundary = "lower" if period == minimum else "upper" if period == maximum else None
    return Optimum(period, cost, boundary)


def near_optimal_ranges(model, distribution, minimum, maximum, tolerance):
    threshold = optimize(model, distribution, minimum, maximum).cost * (1 + tolerance)
    points = distribution.candidates(model, minimum, maximum)
    ranges = []
    for left, right in zip(points, points[1:]):
        left_inside = distribution.cost(model, left) <= threshold
        right_inside = distribution.cost(model, right) <= threshold
        if not (left_inside or right_inside):
            continue
        if left_inside != right_inside:
            lower, upper = left, right
            for _ in range(60):
                middle = (lower + upper) / 2
                if (distribution.cost(model, middle) <= threshold) == left_inside:
                    lower = middle
                else:
                    upper = middle
            crossing = (lower + upper) / 2
            left, right = (left, crossing) if left_inside else (crossing, right)
        if ranges and math.isclose(ranges[-1][1], left, rel_tol=1e-12):
            ranges[-1] = (ranges[-1][0], right)
        else:
            ranges.append((left, right))
    return ranges


def quantile(values, probability):
    ordered = sorted(values)
    position = probability * (len(ordered) - 1)
    lower = int(position)
    upper = min(lower + 1, len(ordered) - 1)
    return ordered[lower] + (ordered[upper] - ordered[lower]) * (position - lower)


def interval(values, confidence):
    tail = (1 - confidence) / 2
    return quantile(values, tail), quantile(values, 1 - tail)


def history_groups(histories):
    """Overlap-connected lineages are resampled together, never as independent histories."""
    groups = []
    end = None
    for index in sorted(range(len(histories)), key=lambda index: histories[index].timestamp):
        history = histories[index]
        if end is None or history.timestamp > end:
            groups.append([])
        groups[-1].append(index)
        history_end = history.timestamp + timedelta(days=history.ages_days[-1])
        end = history_end if end is None else max(end, history_end)
    return groups


@dataclass(frozen=True)
class BootstrapDraw:
    model: LinearForecast
    prediction_fit: LinearFit
    prediction_residual: float

    def prediction(self, age):
        return max(0, self.prediction_fit.predict(age) + self.prediction_residual)


@dataclass(frozen=True)
class Ensemble:
    draws: tuple
    block_count: int
    group_count: int
    reason: str | None

    def mean_interval(self, age, confidence):
        return interval([draw.model.predict(age) for draw in self.draws], confidence)

    def prediction_interval(self, age, confidence):
        return interval([draw.prediction(age) for draw in self.draws], confidence)


def bootstrap(model, samples, block_days, seed):
    origin = min(history.timestamp for history in model.histories)
    keys = [tuple(math.floor(
        ((history.timestamp - origin).total_seconds() / SECONDS_PER_DAY + age) / block_days)
        for age in history.ages_days) for history in model.histories]
    blocks = sorted({key for history_keys in keys for key in history_keys})
    groups = history_groups(model.histories)
    reason = None
    if min(len(history.ages_days) for history in model.histories) < 4:
        reason = "Each history needs at least four samples to estimate residual uncertainty."
    elif len(blocks) < 4:
        reason = "Fewer than four calendar blocks; residual uncertainty is not estimable."
    if reason:
        return Ensemble((), len(blocks), len(groups), reason)

    centered_residuals = []
    for fit in model.fits:
        mean = math.fsum(weight * residual for weight, residual in zip(fit.weights, fit.residuals))
        centered_residuals.append(tuple(residual - mean for residual in fit.residuals))
    rng = random.Random(seed)
    draws = []
    for _ in range(samples):
        # A calendar-block wild bootstrap shares each sign across all overlapping lineages.
        signs = {key: rng.choice((-1, 1)) for key in blocks}
        fits = []
        for index, (history, fit) in enumerate(zip(model.histories, model.fits)):
            pseudo_sizes = [fit.predict(age) + signs[key] * residual
                            for age, key, residual in zip(
                                history.ages_days, keys[index], centered_residuals[index])]
            fits.append(fit_linear(history.ages_days, pseudo_sizes))
        selected = [index for group in rng.choices(groups, k=len(groups)) for index in group]
        histories = [model.histories[index] for index in selected]
        draw_model = make_forecast(histories, [fits[index] for index in selected], model.baseline_override)
        prediction_index = rng.choice(selected)
        residual_index = rng.choices(range(len(model.fits[prediction_index].residuals)),
                                     weights=model.fits[prediction_index].weights, k=1)[0]
        draws.append(BootstrapDraw(
            draw_model, fits[prediction_index], centered_residuals[prediction_index][residual_index]))
    return Ensemble(tuple(draws), len(blocks), len(groups), None)


def training_histories(histories, cutoff):
    training = []
    for index, history in enumerate(histories):
        age = (cutoff - history.timestamp).total_seconds() / SECONDS_PER_DAY
        count = bisect_right(history.ages_days, age)
        if count >= 3:
            training.append((index, replace(
                history, ages_days=history.ages_days[:count], delta_bytes=history.delta_bytes[:count],
                cumulative_byte_days=history.cumulative_byte_days[:count])))
    return training


@dataclass(frozen=True)
class BacktestPoint:
    cutoff: datetime
    horizon: float
    error: float
    weight: float
    covered: bool | None


@dataclass(frozen=True)
class Backtest:
    points: tuple
    mean_size_errors: tuple
    folds: int
    unavailable_intervals: tuple


def backtest(histories, samples, block_days, confidence, seed):
    start = min(history.timestamp for history in histories)
    end = max(history.timestamp + timedelta(days=history.ages_days[-1]) for history in histories)
    points, mean_errors, unavailable = [], [], []
    signatures = set()
    folds = 0
    for fold, fraction in enumerate((0.5, 0.65, 0.8)):
        cutoff = start + (end - start) * fraction
        training = training_histories(histories, cutoff)
        signature = tuple((index, len(history.ages_days)) for index, history in training)
        if not training or signature in signatures:
            continue
        signatures.add(signature)
        eligible = [(index, history) for index, history in training
                    if len(history.ages_days) < len(histories[index].ages_days)]
        if not eligible:
            continue
        model = fit_model([history for _, history in training])
        ensemble = bootstrap(model, samples, block_days, seed + fold + 1)
        if ensemble.reason:
            unavailable.append(f"{cutoff.isoformat()}: {ensemble.reason}")
        folds += 1
        for index, prefix in eligible:
            history = histories[index]
            cutoff_age = (cutoff - history.timestamp).total_seconds() / SECONDS_PER_DAY
            end_age = history.ages_days[-1]
            duration = end_age - cutoff_age
            start_area = history.average_delta(cutoff_age) * cutoff_age
            observed_mean = (history.cumulative_byte_days[-1] - start_area) / duration
            predicted_mean = model.intercept + model.slope * (cutoff_age + end_age) / 2
            mean_errors.append(predicted_mean - observed_mean)
            for sample in range(len(prefix.ages_days), len(history.ages_days)):
                age = history.ages_days[sample]
                actual = history.delta_bytes[sample]
                weight = (age - max(cutoff_age, history.ages_days[sample - 1])) / duration / len(eligible)
                covered = None
                if ensemble.draws:
                    lower, upper = ensemble.prediction_interval(age, confidence)
                    covered = lower <= actual <= upper
                points.append(BacktestPoint(
                    cutoff, age - cutoff_age, model.predict(age) - actual, weight, covered))
    return Backtest(tuple(points), tuple(mean_errors), folds, tuple(unavailable))
