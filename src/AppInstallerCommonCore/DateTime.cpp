// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerDateTime.h"

namespace AppInstaller::Utility
{
    // If moved to C++20, this can be replaced with standard library implementations.
    void OutputTimePoint(std::ostream& stream, const std::chrono::system_clock::time_point& time, bool useRFC3339)
    {
        using namespace std::chrono;

        tm localTime{};
        auto tt = system_clock::to_time_t(time);
        _localtime64_s(&localTime, &tt);

        stream
            << std::setw(4) << (1900 + localTime.tm_year) << '-'
            << std::setw(2) << std::setfill('0') << (1 + localTime.tm_mon) << '-'
            << std::setw(2) << std::setfill('0') << localTime.tm_mday << (useRFC3339 ? 'T' : ' ')
            << std::setw(2) << std::setfill('0') << localTime.tm_hour << ':'
            << std::setw(2) << std::setfill('0') << localTime.tm_min << ':'
            << std::setw(2) << std::setfill('0') << localTime.tm_sec << '.';

        // Get partial seconds
        auto sinceEpoch = time.time_since_epoch();
        auto leftoverMillis = duration_cast<milliseconds>(sinceEpoch) - duration_cast<seconds>(sinceEpoch);

        stream << std::setw(3) << std::setfill('0') << leftoverMillis.count();

        if (useRFC3339)
        {
            // RFC 3339 requires adding time zone info.
            // No need to bother getting the actual time zone as we don't need it.
            // -00:00 represents an unspecified time zone, not UTC.
            stream << "-00:00";
        }
    }

    std::string GetCurrentTimeForFilename()
    {
        std::stringstream stream;
        OutputTimePoint(stream, std::chrono::system_clock::now());

        auto result = stream.str();
        std::replace(result.begin(), result.end(), ':', '-');
        std::replace(result.begin(), result.end(), ' ', '-');

        return result;
    }

    int64_t GetCurrentUnixEpoch()
    {
        static_assert(std::is_same_v<int64_t, decltype(time(nullptr))>, "time returns a 64-bit integer");
        time_t now = time(nullptr);
        return static_cast<int64_t>(now);
    }

    int64_t ConvertSystemClockToUnixEpoch(const std::chrono::system_clock::time_point& time)
    {
        static_assert(std::is_same_v<int64_t, decltype(std::chrono::system_clock::to_time_t(time))>, "to_time_t returns a 64-bit integer");
        time_t timeAsTimeT = std::chrono::system_clock::to_time_t(time);
        return static_cast<int64_t>(timeAsTimeT);
    }

    std::chrono::system_clock::time_point ConvertUnixEpochToSystemClock(int64_t epoch)
    {
        return std::chrono::system_clock::from_time_t(static_cast<time_t>(epoch));
    }
}
