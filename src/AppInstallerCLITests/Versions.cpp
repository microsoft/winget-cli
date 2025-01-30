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

TEST_CASE("VersionParseWithWhitespace", "[versions]")
{
    Version version("1. 2.3 . 4 ");
    const auto& parts = version.GetParts();
    REQUIRE(parts.size() == 4);
    for (size_t i = 0; i < parts.size(); ++i)
    {
        INFO(i);
        REQUIRE(parts[i].Integer == static_cast<uint64_t>(i + 1));
        REQUIRE(parts[i].Other == "");
    }
}

TEST_CASE("VersionParseWithPreamble", "[versions]")
{
    Version version("v1.2.3.4");
    const auto& parts = version.GetParts();
    REQUIRE(parts.size() == 4);
    for (size_t i = 0; i < parts.size(); ++i)
    {
        INFO(i);
        REQUIRE(parts[i].Integer == static_cast<uint64_t>(i + 1));
        REQUIRE(parts[i].Other == "");
    }
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

    Version version6(". 1 ");
    parts = version6.GetParts();
    REQUIRE(parts.size() == 2);
    REQUIRE(parts[0].Integer == 0);
    REQUIRE(parts[0].Other == "");
    REQUIRE(parts[1].Integer == 1);
    REQUIRE(parts[1].Other == "");

    Version version7("v1.2a");
    parts = version7.GetParts();
    REQUIRE(parts.size() == 2);
    REQUIRE(parts[0].Integer == 1);
    REQUIRE(parts[0].Other == "");
    REQUIRE(parts[1].Integer == 2);
    REQUIRE(parts[1].Other == "a");
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

    // Ensure that versions with non-digit characters in their parts are sorted correctly
    RequireLessThan("1-rc", "1");
    RequireLessThan("1.2-rc", "1.2");
    RequireLessThan("1.0-rc", "1.0");
    RequireLessThan("1.0.0-rc", "1");
    RequireLessThan("22.0.0-rc.1", "22.0.0");
    RequireLessThan("22.0.0-rc.1", "22.0.0.1");
    RequireLessThan("22.0.0-rc.1", "22.0.0.1-rc");

    // Ensure that Sub-RC versions are sorted correctly
    RequireLessThan("22.0.0-rc.1", "22.0.0-rc.1.1");
    RequireLessThan("22.0.0-rc.1.1", "22.0.0-rc.1.2");
    RequireLessThan("22.0.0-rc.1.2", "22.0.0-rc.2");

    RequireEqual("1.0", "1.0.0");

    // Ensure that integers are parsed correctly when there is a leading zero
    RequireEqual("1.2.00.3", "1.2.0.3");
    RequireEqual("1.2.003.4", "1.2.3.4");
    RequireEqual("01.02.03.04", "1.2.3.4");
    RequireEqual("1.2.03-beta", "1.2.3-beta");

    // Ensure whitespace doesn't affect equality
    RequireEqual("1.0", "1.0 ");
    RequireEqual("1.0", "1. 0");
    RequireEqual("1.0", "1.0.");

    // Ensure versions with preambles are sorted correctly
    RequireEqual("1.0", "Version 1.0");
    RequireEqual("foo1", "bar1");
    RequireLessThan("v0.0.1", "0.0.2");
    RequireLessThan("v0.0.1", "v0.0.2");
    RequireLessThan("1.a2", "1.b1");
    RequireLessThan("alpha", "beta");
}

TEST_CASE("VersionAndChannelSort", "[versions]")
{
    std::vector<VersionAndChannel> sortedList =
    {
        { Version("15.0.0"), Channel("") },
        { Version("14.0.0"), Channel("") },
        { Version("13.2.1-bugfix"), Channel("") },
        { Version("13.2.0"), Channel("") },
        { Version("13.2.0-rc"), Channel("") },
        { Version("13.0.0"), Channel("") },
        { Version("16.0.0"), Channel("alpha") },
        { Version("15.8.0"), Channel("alpha") },
        { Version("15.1.0"), Channel("beta") },
    };

    std::vector<size_t> reorderList = { 4, 2, 1, 7, 6, 3, 8, 5, 0 };
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

TEST_CASE("VersionLatest", "[versions]")
{
    REQUIRE(Version::CreateLatest().IsLatest());
    REQUIRE(Version("latest").IsLatest());
    REQUIRE(Version("LATEST").IsLatest());
    REQUIRE(!Version("1.0").IsLatest());

    RequireLessThan("1.0", "latest");
    RequireLessThan("100", "latest");
    RequireLessThan("943849587389754876.1", "latest");

    RequireEqual("latest", "LATEST");
}

TEST_CASE("VersionUnknown", "[versions]")
{
    REQUIRE(Version::CreateUnknown().IsUnknown());
    REQUIRE(Version("unknown").IsUnknown());
    REQUIRE(Version("UNKNOWN").IsUnknown());
    REQUIRE(!Version("1.0").IsUnknown());

    RequireLessThan("unknown", "1.0");
    RequireLessThan("unknown", "1.fork");

    RequireEqual("unknown", "UNKNOWN");
}

TEST_CASE("VersionUnknownLessThanLatest", "[versions]")
{
    REQUIRE(Version::CreateUnknown() < Version::CreateLatest());
}

TEST_CASE("VersionIsEmpty", "[versions]")
{
    REQUIRE(Version{}.IsEmpty());
    REQUIRE(Version{""}.IsEmpty());
    REQUIRE(!Version{"1"}.IsEmpty());
    REQUIRE(!Version{"0"}.IsEmpty());

    Version v{ "1" };
    REQUIRE(!v.IsEmpty());
    v.Assign("");
    REQUIRE(v.IsEmpty());
}

TEST_CASE("VersionPartAt", "[versions]")
{
    REQUIRE(Version{}.PartAt(0).Integer == 0);
    REQUIRE(Version{"1"}.PartAt(0).Integer == 1);
    REQUIRE(Version{"1"}.PartAt(1).Integer == 0);
    REQUIRE(Version{"1"}.PartAt(9999).Integer == 0);
}

TEST_CASE("UInt64Version_Success_FourParts", "[versions]")
{
    Version expectedVersion("1.2.3.4");
    UInt64Version versionNumberFromNumber(0x0001000200030004);
    UInt64Version versionNumberFromString("1.2.3.4");
    REQUIRE(expectedVersion == versionNumberFromNumber);
    REQUIRE(expectedVersion == versionNumberFromString);
    REQUIRE(expectedVersion.ToString() == versionNumberFromNumber.ToString());
    REQUIRE(expectedVersion.ToString() == versionNumberFromString.ToString());
}

TEST_CASE("UInt64Version_Success_LessThanFourParts", "[versions]")
{
    UInt64Version versionNumberFromNumber(0x0001000200030000);
    UInt64Version versionNumberFromString("1.2.3");
    REQUIRE(versionNumberFromNumber == versionNumberFromString);
}

TEST_CASE("UInt64Version_Success_NoOverflow", "[versions]")
{
    REQUIRE_NOTHROW(UInt64Version("65535.65535.65535.65535")); // 65535 => 0xffff
    REQUIRE_NOTHROW(UInt64Version(0xffffffffffffffff));
}

TEST_CASE("UInt64Version_Fail_Overflow", "[versions]")
{
    REQUIRE_THROWS(UInt64Version("1.0.0.65536")); // 65536 => 0x10000
}

TEST_CASE("UInt64Version_Fail_MoreThanFourParts", "[versions]")
{
    REQUIRE_THROWS(UInt64Version("1.0.0.0.1"));
}

TEST_CASE("UInt64Version_Fail_NonNumeric", "[versions]")
{
    REQUIRE_THROWS(UInt64Version("1.0.0.a"));
}

TEST_CASE("ApproximateVersionParse", "[versions]")
{
    Version v1_0{ "1.0" };
    Version v1_0_LessThan{ v1_0, Version::ApproximateComparator::LessThan };
    Version v1_0_GreaterThan{ v1_0, Version::ApproximateComparator::GreaterThan };

    Version v1_0_LessThanFromString = Version{ "< 1.0" };
    Version v1_0_GreaterThanFromString = Version{ "> 1.0" };

    REQUIRE_FALSE(v1_0.IsApproximate());
    REQUIRE(v1_0_LessThanFromString.IsApproximate());
    REQUIRE(v1_0_GreaterThanFromString.IsApproximate());

    REQUIRE(v1_0_LessThan == v1_0_LessThanFromString);
    REQUIRE(v1_0_GreaterThan == v1_0_GreaterThanFromString);

    REQUIRE_THROWS(Version{ "< Unknown" });
    REQUIRE_THROWS(Version{ v1_0_LessThan, Version::ApproximateComparator::LessThan });
    REQUIRE_THROWS(Version{ Version::CreateUnknown(), Version::ApproximateComparator::LessThan });
}

TEST_CASE("ApproximateVersionCompare", "[versions]")
{
    RequireEqual("< 1.0", "< 1.0");
    RequireEqual("< 1.0", "< 1.0.0");
    RequireEqual("> 1.0", "> 1.0");
    RequireEqual("> 1.0", "> 1.0.0");

    RequireLessThan("< 1.0", "1.0");
    RequireLessThan("< 1.0", "> 1.0");
    RequireLessThan("1.0", "> 1.0");
    RequireLessThan("0.9", "< 1.0");
    RequireLessThan("> 1.0", "1.1");

    // With latest
    RequireLessThan("< latest", "latest");
    RequireLessThan("latest", "> latest");
    RequireLessThan("9999", "< latest");
}

TEST_CASE("VersionRange", "[versions]")
{
    // Create
    REQUIRE_NOTHROW(VersionRange{ Version{ "1.0" }, Version{ "2.0" } });
    REQUIRE_NOTHROW(VersionRange{ Version{ "1.0" }, Version{ "1.0" } });
    REQUIRE_THROWS(VersionRange{ Version{ "2.0" }, Version{ "1.0" } });

    // Overlaps
    REQUIRE(VersionRange{ Version{ "1.0" }, Version{ "2.0" } }.Overlaps(VersionRange{ Version{ "2.0" }, Version{ "3.0" } }));
    REQUIRE(VersionRange{ Version{ "1.0" }, Version{ "2.0" } }.Overlaps(VersionRange{ Version{ "1.0" }, Version{ "1.0" } }));
    REQUIRE(VersionRange{ Version{ "1.0" }, Version{ "2.0" } }.Overlaps(VersionRange{ Version{ "0.5" }, Version{ "1.5" } }));
    REQUIRE_FALSE(VersionRange{ Version{ "1.0" }, Version{ "2.0" } }.Overlaps(VersionRange{ Version{ "2.1" }, Version{ "3.0" } }));
    REQUIRE_FALSE(VersionRange{ Version{ "1.0" }, Version{ "2.0" } }.Overlaps(VersionRange{}));

    // Empty
    REQUIRE(VersionRange{}.IsEmpty());
    REQUIRE_THROWS(VersionRange{}.GetMinVersion());
    REQUIRE_THROWS(VersionRange{}.GetMaxVersion());

    // Less than compare
    REQUIRE_THROWS(VersionRange{ Version{ "0.5" }, Version{ "1.0" } } < VersionRange{ Version{ "1.0" }, Version{ "2.0" } });
    REQUIRE_THROWS(VersionRange{} < VersionRange{ Version{ "1.0" }, Version{ "2.0" } });
    REQUIRE(VersionRange{ Version{ "0.5" }, Version{ "1.0" } } < VersionRange{ Version{ "1.5" }, Version{ "2.0" } });
    REQUIRE_FALSE(VersionRange{ Version{ "1.5" }, Version{ "2.0" } } < VersionRange{ Version{ "0.5" }, Version{ "1.0" } });
}

TEST_CASE("GatedVersion", "[versions]")
{
    REQUIRE(GatedVersion("1.0.*"sv).IsValidVersion({ "1.0.1" }));
    REQUIRE(GatedVersion("1.0.*"sv).IsValidVersion({ "1.0" }));
    REQUIRE(GatedVersion("1.0.*"sv).IsValidVersion({ "1" }));
    REQUIRE(GatedVersion("1.0.*"sv).IsValidVersion({ "1.0.alpha" }));
    REQUIRE(GatedVersion("1.0.*"sv).IsValidVersion({ "1.0.1.2.3" }));
    REQUIRE(GatedVersion("1.0.*"sv).IsValidVersion({ "1.0.*" }));
    REQUIRE_FALSE(GatedVersion("1.0.*"sv).IsValidVersion({ "1.1.1" }));

    REQUIRE(GatedVersion("1.*.*"sv).IsValidVersion({ "1.*.1" }));
    REQUIRE(GatedVersion("1.*.*"sv).IsValidVersion({ "1.*.*" }));
    REQUIRE_FALSE(GatedVersion("1.*.*"sv).IsValidVersion({ "1.1.1" }));

    REQUIRE(GatedVersion("1.0.1"sv).IsValidVersion({ "1.0.1" }));
    REQUIRE_FALSE(GatedVersion("1.0.1"sv).IsValidVersion({ "1.1.1" }));
}

TEST_CASE("SemanticVersion", "[versions]")
{
    REQUIRE_THROWS_HR(SemanticVersion("1.2.3.4"), E_INVALIDARG);
    REQUIRE_THROWS_HR(SemanticVersion("1.2abc.3"), E_INVALIDARG);

    SemanticVersion version = SemanticVersion("1.2.3-alpha");
    REQUIRE(version.IsPrerelease());
    REQUIRE(version.PrereleaseVersion() == Version("alpha"));
    REQUIRE(!version.HasBuildMetadata());
    REQUIRE(version.PartAt(2).Other == "-alpha");

    version = SemanticVersion("1.2.3-4.5.6");
    REQUIRE(version.IsPrerelease());
    REQUIRE(version.PrereleaseVersion() == Version("4.5.6"));
    REQUIRE(!version.HasBuildMetadata());
    REQUIRE(version.PartAt(2).Other == "-4.5.6");

    // Really shouldn't be allowed, but we are loose here
    version = SemanticVersion("1.2+build");
    REQUIRE(!version.IsPrerelease());
    REQUIRE(version.HasBuildMetadata());
    REQUIRE(version.BuildMetadata() == Version("build"));
    REQUIRE(version.PartAt(2).Other == "+build");

    version = SemanticVersion("1.2.3-beta+4.5.6");
    REQUIRE(version.IsPrerelease());
    REQUIRE(version.PrereleaseVersion() == Version("beta"));
    REQUIRE(version.HasBuildMetadata());
    REQUIRE(version.BuildMetadata() == Version("4.5.6"));
    REQUIRE(version.PartAt(2).Other == "-beta+4.5.6");
}

TEST_CASE("OpenTypeFontVersion", "[versions]")
{
    // Valid font version.
    OpenTypeFontVersion version = OpenTypeFontVersion("Version 1.234");
    REQUIRE(version.ToString() == "1.234");
    REQUIRE(version.GetParts().size() == 2);
    REQUIRE(version.PartAt(0).Integer == 1);
    REQUIRE(version.PartAt(1).Integer == 234);

    // Font version with additional metadata.
    version = OpenTypeFontVersion("Version 9.876.54 ;2024");
    REQUIRE(version.ToString() == "9.876");
    REQUIRE(version.GetParts().size() == 2);
    REQUIRE(version.PartAt(0).Integer == 9);
    REQUIRE(version.PartAt(1).Integer == 876);

    // Invalid version. Font version must have at least 2 parts.
    REQUIRE_NOTHROW(version = OpenTypeFontVersion("1234567"));
    REQUIRE(version.IsUnknown());
    REQUIRE(version.ToString() == "Unknown");

    // Major and minor parts must have digits.
    REQUIRE_NOTHROW(version = OpenTypeFontVersion(" abc.def "));
    REQUIRE(version.IsUnknown());
    REQUIRE(version.ToString() == "Unknown");
}
