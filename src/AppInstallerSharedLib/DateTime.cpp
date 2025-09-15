// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerDateTime.h"

using namespace std::chrono;

namespace AppInstaller::Utility
{
    namespace
    {
        struct OutputTimePointContext
        {
            OutputTimePointContext(std::ostream& stream, const std::chrono::system_clock::time_point& time, TimeFacet facet) :
                Stream(stream), Time(time), Facet(facet)
            {
                auto tt = system_clock::to_time_t(time);
                _localtime64_s(&LocalTime, &tt);
            }

            std::ostream& Stream;
            const std::chrono::system_clock::time_point& Time;
            tm LocalTime{};
            TimeFacet Facet;
        };

        struct OutputTimePointFacetInfo
        {
            TimeFacet Facet;
            char FollowingSeparator;
            void (*Action)(const OutputTimePointContext&);
        };
    }

    void OutputTimePoint(std::ostream& stream, const std::chrono::system_clock::time_point& time, bool useRFC3339)
    {
        OutputTimePoint(stream, time, TimeFacet::Default | (useRFC3339 ? TimeFacet::RFC3339 : TimeFacet::None));
    }

    // If moved to C++20, this can be replaced with standard library implementations.
    void OutputTimePoint(std::ostream& stream, const std::chrono::system_clock::time_point& time, TimeFacet facet)
    {
        OutputTimePointContext context{ stream, time, facet };
        using Ctx = const OutputTimePointContext&;

        bool useRFC3339 = WI_IsFlagSet(facet, TimeFacet::RFC3339);
        bool filename = WI_IsFlagSet(facet, TimeFacet::Filename);
        char day_time_separator = useRFC3339 ? 'T' : (filename ? '-' : ' ');
        char time_field_separator = filename ? '-' : ':';

        bool needsSeparator = false;
        char currentSeparator = '-';

        for (const auto& info : {
            OutputTimePointFacetInfo{ TimeFacet::ShortYear, '-', [](Ctx ctx) { ctx.Stream << (ctx.LocalTime.tm_year - 100); }},
            OutputTimePointFacetInfo{ TimeFacet::Year, '-', [](Ctx ctx) { ctx.Stream << (1900 + ctx.LocalTime.tm_year); }},
            OutputTimePointFacetInfo{ TimeFacet::Month, '-', [](Ctx ctx) { ctx.Stream << std::setw(2) << std::setfill('0') << (1 + ctx.LocalTime.tm_mon); }},
            OutputTimePointFacetInfo{ TimeFacet::Day, day_time_separator, [](Ctx ctx) { ctx.Stream << std::setw(2) << std::setfill('0') << ctx.LocalTime.tm_mday; }},
            OutputTimePointFacetInfo{ TimeFacet::Hour, time_field_separator, [](Ctx ctx) { ctx.Stream << std::setw(2) << std::setfill('0') << ctx.LocalTime.tm_hour; }},
            OutputTimePointFacetInfo{ TimeFacet::Minute, time_field_separator, [](Ctx ctx) { ctx.Stream << std::setw(2) << std::setfill('0') << ctx.LocalTime.tm_min; }},
            OutputTimePointFacetInfo{ TimeFacet::Second, '.', [](Ctx ctx) { ctx.Stream << std::setw(2) << std::setfill('0') << ctx.LocalTime.tm_sec; }},
            OutputTimePointFacetInfo{ TimeFacet::Millisecond, '-', [](Ctx ctx)
            {
                // Get partial seconds
                auto sinceEpoch = ctx.Time.time_since_epoch();
                auto leftoverMillis = duration_cast<milliseconds>(sinceEpoch) - duration_cast<seconds>(sinceEpoch);

                ctx.Stream << std::setw(3) << std::setfill('0') << leftoverMillis.count();
            }},
            OutputTimePointFacetInfo{ TimeFacet::RFC3339, '\0', [](Ctx ctx)
            {
                // RFC 3339 requires adding time zone info.
                // No need to bother getting the actual time zone as we don't need it.
                // -00:00 represents an unspecified time zone, not UTC.
                ctx.Stream << "00:00";
            }},
            })
        {
            if (WI_AreAllFlagsSet(facet, info.Facet))
            {
                if (needsSeparator)
                {
                    stream << currentSeparator;
                }

                info.Action(context);
                needsSeparator = true;
            }

            // Getting this right for every mix of facets is probably not possible.
            // Future needs can dictate changes here.
            currentSeparator = info.FollowingSeparator;
        }
    }

    std::string TimePointToString(const std::chrono::system_clock::time_point& time, bool useRFC3339)
    {
        std::ostringstream stream;
        OutputTimePoint(stream, time, useRFC3339);
        return std::move(stream).str();
    }

    std::string TimePointToString(const std::chrono::system_clock::time_point& time, TimeFacet facet)
    {
        std::ostringstream stream;
        OutputTimePoint(stream, time, facet);
        return std::move(stream).str();
    }

    std::string GetCurrentTimeForFilename(bool shortTime)
    {
        return TimePointToString(std::chrono::system_clock::now(), (shortTime ? TimeFacet::ShortYearSecondPrecision : TimeFacet::Default) | TimeFacet::Filename);
    }

    std::string GetCurrentDateForARP()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);

        struct tm newTime;
        localtime_s(&newTime, &tt);

        std::stringstream ss;
        ss << std::put_time(&newTime, "%Y%m%d");
        return ss.str();
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

    std::chrono::system_clock::time_point GetTimePointFromVersion(const UInt64Version& version)
    {
        // Our custom format for converting UTC into a version is:
        //  Major :: `Year` [1, 9999]
        //  Minor :: `Month * 100 + Day` where Month [1, 12] and Day [1, 31]
        //  Build :: `Hour * 100 + Minute` where Hour [1, 24] and Minute [0, 59]
        //  Revision :: Milliseconds, but since no seconds are available we will disregard this

        tm versionTime{};

        // Limit to the range supported by _mkgmtime64, which is 1970 to 3000 (hello to Y3K maintainers from 2023!)
        UINT64 majorVersion = version.Major();
        if (majorVersion < 1970 || majorVersion > 3000)
        {
            return std::chrono::system_clock::time_point::min();
        }
        versionTime.tm_year = static_cast<int>(majorVersion) - 1900;

        UINT64 minorVersion = version.Minor();
        UINT64 monthValue = minorVersion / 100;
        UINT64 dayValue = minorVersion % 100;
        if (monthValue < 1 || monthValue > 12 || dayValue < 1 || dayValue > 31)
        {
            return std::chrono::system_clock::time_point::min();
        }
        versionTime.tm_mon = static_cast<int>(monthValue) - 1;
        versionTime.tm_mday = static_cast<int>(dayValue);

        UINT64 buildVersion = version.Build();
        UINT64 hourValue = buildVersion / 100;
        UINT64 minuteValue = buildVersion % 100;
        if (hourValue < 1 || hourValue > 24 || minuteValue > 59)
        {
            return std::chrono::system_clock::time_point::min();
        }
        versionTime.tm_hour = static_cast<int>(hourValue) - 1;
        versionTime.tm_min = static_cast<int>(minuteValue);

        return std::chrono::system_clock::from_time_t(_mkgmtime64(&versionTime));
    }
}
