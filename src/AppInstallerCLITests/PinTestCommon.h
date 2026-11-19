// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

// Named UTC epoch constants (seconds since Unix epoch) shared across pinning tests.
// Use with Utility::ConvertUnixEpochToSystemClock to obtain a time_point.
namespace PinTestEpoch
{
    constexpr int64_t Jan2026_01_0000 = 1767225600LL; // 2026-01-01 00:00:00 UTC
    constexpr int64_t Jan2026_15_1000 = 1768471200LL; // 2026-01-15 10:00:00 UTC
    constexpr int64_t Jan2026_15_1030 = 1768473000LL; // 2026-01-15 10:30:00 UTC
    constexpr int64_t Jan2026_15_1100 = 1768474800LL; // 2026-01-15 11:00:00 UTC
    constexpr int64_t Mar2026_10_1200 = 1773144000LL; // 2026-03-10 12:00:00 UTC
    constexpr int64_t May2026_01_0800 = 1777622400LL; // 2026-05-01 08:00:00 UTC
    constexpr int64_t Jun2026_01_0900 = 1780304400LL; // 2026-06-01 09:00:00 UTC
}
