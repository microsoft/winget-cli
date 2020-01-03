// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DateTime.h"

namespace AppInstaller::Utility
{
    // If moved to C++20, this can be replaced with standard library implementations.
    void OutputTimepoint(std::ostream& stream, const std::chrono::system_clock::time_point& time)
    {
        using namespace std::chrono;

        tm localTime{};
        auto tt = system_clock::to_time_t(time);
        _localtime64_s(&localTime, &tt);

        // Don't bother with fill chars for dates, as most of the time this won't be an issue.
        stream << (1900 + localTime.tm_year) << '-' << (1 + localTime.tm_mon) << '-' << localTime.tm_mday << ' '
            << std::setw(2) << std::setfill('0') << localTime.tm_hour << ':' 
            << std::setw(2) << std::setfill('0') << localTime.tm_min << ':' 
            << std::setw(2) << std::setfill('0') << localTime.tm_sec << '.';
        
        // Get partial seconds
        auto sinceEpoch = time.time_since_epoch();
        auto leftoverMillis = duration_cast<milliseconds>(sinceEpoch) - duration_cast<seconds>(sinceEpoch);

        stream << std::setw(3) << std::setfill('0') << leftoverMillis.count();
    }
}
