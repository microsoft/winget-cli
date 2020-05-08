// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerRuntime.h>
#include <AppInstallerVersions.h>

using namespace AppInstaller;
using namespace AppInstaller::Utility;


TEST_CASE("VersionParse", "[versions]")
{
    Version version("1.2.3.4-alpha");
    const auto& parts = version.GetParts();
    REQUIRE(parts.size() == 4);
    for (size_t i = 0; i < parts.size(); ++i)
    {
        INFO(i);
        REQUIRE(parts[i].Integer == static_cast<uint64_t>(i + 1));
        if (i != 3)
        {
            REQUIRE(parts[i].Other == "");
        }
        else
        {
            REQUIRE(parts[i].Other == "-alpha");
        }
    }
}

TEST_CASE("VersionParsePlusDash", "[versions]")
{
    Version version("1.2.3.4-alpha", ".-");
    const auto& parts = version.GetParts();
    REQUIRE(parts.size() == 5);
    for (size_t i = 0; i < 4; ++i)
    {
        INFO(i);
        REQUIRE(parts[i].Integer == static_cast<uint64_t>(i + 1));
        REQUIRE(parts[i].Other == "");
    }
    REQUIRE(parts[4].Other == "alpha");
}

TEST_CASE("VersionParseCorner", "[versions]")
{
    Version version1("");
    auto parts = version1.GetParts();
    REQUIRE(parts.size() == 0);

    Version version2(".");
    parts = version2.GetParts();
    REQUIRE(parts.size() == 0);

    Version version3(".0");
    parts = version3.GetParts();
    REQUIRE(parts.size() == 0);

    Version version4(".1");
    parts = version4.GetParts();
    REQUIRE(parts.size() == 2);
    REQUIRE(parts[0].Integer == 0);
    REQUIRE(parts[0].Other == "");
    REQUIRE(parts[1].Integer == 1);
    REQUIRE(parts[1].Other == "");

    Version version5("version");
    parts = version5.GetParts();
    REQUIRE(parts.size() == 1);
    REQUIRE(parts[0].Integer == 0);
    REQUIRE(parts[0].Other == "version");
}

void RequireLessThan(std::string_view a, std::string_view b)
{
    Version vA{ std::string(a) };
    Version vB{ std::string(b) };

    REQUIRE(vA < vB);
    REQUIRE_FALSE(vB < vA);
    REQUIRE(vA <= vB);
    REQUIRE_FALSE(vB <= vA);
    REQUIRE(vB > vA);
    REQUIRE_FALSE(vA > vB);
    REQUIRE(vB >= vA);
    REQUIRE_FALSE(vA >= vB);
    REQUIRE_FALSE(vA == vB);
    REQUIRE(vA != vB);
}

void RequireEqual(std::string_view a, std::string_view b)
{
    Version vA{ std::string(a) };
    Version vB{ std::string(b) };

    REQUIRE(vA == vB);
    REQUIRE_FALSE(vA != vB);
    REQUIRE(vA <= vB);
    REQUIRE(vA >= vB);
    REQUIRE_FALSE(vA < vB);
    REQUIRE_FALSE(vA > vB);
}

TEST_CASE("VersionCompare", "[versions]")
{
    RequireLessThan("1", "2");
    RequireLessThan("1.0.0", "2.0.0");
    RequireLessThan("0.0.1", "0.0.2");
    RequireLessThan("0.0.1-alpha", "0.0.2-alpha");
    RequireLessThan("0.0.1-beta", "0.0.2-alpha");
    RequireLessThan("0.0.1-beta", "0.0.2-alpha");
    RequireLessThan("13.9.8", "14.1");

    RequireEqual("1.0", "1.0.0");
}

TEST_CASE("VersionAndChannelSort", "[versions]")
{
    std::vector<VersionAndChannel> sortedList =
    {
        { Version("15.0.0"), Channel("") },
        { Version("14.0.0"), Channel("") },
        { Version("13.2.0-bugfix"), Channel("") },
        { Version("13.2.0"), Channel("") },
        { Version("13.0.0"), Channel("") },
        { Version("16.0.0"), Channel("alpha") },
        { Version("15.8.0"), Channel("alpha") },
        { Version("15.1.0"), Channel("beta") },
    };

    std::vector<size_t> reorderList = { 4, 2, 1, 7, 6, 3, 5, 0 };
    REQUIRE(sortedList.size() == reorderList.size());

    std::vector<VersionAndChannel> jumbledList;
    for (auto i : reorderList)
    {
        jumbledList.emplace_back(sortedList[i]);
    }

    std::sort(jumbledList.begin(), jumbledList.end());

    for (size_t i = 0; i < jumbledList.size(); ++i)
    {
        const VersionAndChannel& sortedVAC = sortedList[i];
        const VersionAndChannel& jumbledVAC = jumbledList[i];

        INFO(i);
        REQUIRE(sortedVAC.GetVersion().ToString() == jumbledVAC.GetVersion().ToString());
        REQUIRE(sortedVAC.GetChannel().ToString() == jumbledVAC.GetChannel().ToString());
    }
}

TEST_CASE("MinOsVersion_Check", "[versions]")
{
    // Just verify that we are greater than Win 7 and less than far future Win 10.
    // Unfortunately, an unmanifested process will also pass these validations,
    // but an unmanifested process also can't use Windows APIs to determine the actual version.
    REQUIRE(Runtime::IsCurrentOSVersionGreaterThanOrEqual(Version("6.1")));
    REQUIRE(!Runtime::IsCurrentOSVersionGreaterThanOrEqual(Version("10.0.65535")));
}
