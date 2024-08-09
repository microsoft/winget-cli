// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerDateTime.h>

using namespace AppInstaller::Utility;
using namespace TestCommon;
using namespace std::chrono;

namespace Catch
{
    template<>
    struct StringMaker<std::chrono::system_clock::time_point>
    {
        static std::string convert(const std::chrono::system_clock::time_point& value)
        {
            std::ostringstream stream;
            OutputTimePoint(stream, value);
            return std::move(stream).str();
        }
    };
}

void VerifyGetTimePointFromVersion(std::string_view version, int year, int month, int day, int hour, int minute)
{
    system_clock::time_point result = GetTimePointFromVersion(UInt64Version{ std::string{ version } });

    tm time{};
    auto tt = system_clock::to_time_t(result);
    _gmtime64_s(&time, &tt);

    REQUIRE(year == time.tm_year + 1900);
    REQUIRE(month == time.tm_mon + 1);
    REQUIRE(day == time.tm_mday);
    REQUIRE(hour == time.tm_hour);
    REQUIRE(minute == time.tm_min);
}

std::string StringFromTimePoint(system_clock::time_point input)
{
    tm time{};
    auto tt = system_clock::to_time_t(input);
    _gmtime64_s(&time, &tt);

    std::ostringstream stream;
    stream << time.tm_year + 1900 << '.' << ((time.tm_mon + 1) * 100) + time.tm_mday << '.' << ((time.tm_hour + 1) * 100) + time.tm_min;
    return std::move(stream).str();
}

TEST_CASE("GetTimePointFromVersion", "[datetime]")
{
    // Years out of range
    REQUIRE(GetTimePointFromVersion(UInt64Version{ "1969.1231.2459.0" }) == system_clock::time_point::min());
    REQUIRE(GetTimePointFromVersion(UInt64Version{ "3001.101.100.0" }) == system_clock::time_point::min());

    // Months out of range
    REQUIRE(GetTimePointFromVersion(UInt64Version{ "2023.1.100.0" }) == system_clock::time_point::min());
    REQUIRE(GetTimePointFromVersion(UInt64Version{ "2023.1301.100.0" }) == system_clock::time_point::min());

    // Days out of range
    REQUIRE(GetTimePointFromVersion(UInt64Version{ "2023.100.100.0" }) == system_clock::time_point::min());
    REQUIRE(GetTimePointFromVersion(UInt64Version{ "2023.132.100.0" }) == system_clock::time_point::min());

    // Hours out of range
    REQUIRE(GetTimePointFromVersion(UInt64Version{ "2023.101.0.0" }) == system_clock::time_point::min());
    REQUIRE(GetTimePointFromVersion(UInt64Version{ "2023.101.2500.0" }) == system_clock::time_point::min());

    // Minutes out of range
    REQUIRE(GetTimePointFromVersion(UInt64Version{ "2023.101.160.0" }) == system_clock::time_point::min());

    // In range baseline
    VerifyGetTimePointFromVersion("2023.101.100.0", 2023, 1, 1, 0, 0);

    // Time for presents!
    VerifyGetTimePointFromVersion("2023.1225.814.0", 2023, 12, 25, 7, 14);

    // Epoch time
    REQUIRE(GetTimePointFromVersion(UInt64Version{ "1970.101.100.0" }) == system_clock::time_point{});

    // Round trip now
    system_clock::time_point now = system_clock::now();
    REQUIRE(GetTimePointFromVersion(UInt64Version{ StringFromTimePoint(now) }) == time_point_cast<minutes>(now));
}
