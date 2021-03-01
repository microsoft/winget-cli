// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <chrono>
#include <ostream>

namespace AppInstaller::Utility
{
    // Writes the given time to the given stream.
    // Assumes that system_clock uses Linux epoch (as required by C++20 standard).
    // Time is also assumed to be after the epoch.
    void OutputTimePoint(std::ostream& stream, const std::chrono::system_clock::time_point& time, bool useRFC3339 = false);

    // Gets the current time as a string. Can be used as a file name.
    std::string GetCurrentTimeForFilename();

    // Gets the current time as a unix epoch value.
    int64_t GetCurrentUnixEpoch();

    // Converts the given unix epoch time to a system_clock::time_point.
    int64_t ConvertSystemClockToUnixEpoch(const std::chrono::system_clock::time_point& time);

    // Converts the given unix epoch time to a system_clock::time_point.
    std::chrono::system_clock::time_point ConvertUnixEpochToSystemClock(int64_t epoch);
}
