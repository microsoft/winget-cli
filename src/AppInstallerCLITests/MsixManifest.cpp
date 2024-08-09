// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerMsixInfo.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerRuntime.h>
#include <winget/MsixManifest.h>

using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Msix;
using namespace Microsoft::WRL;

namespace
{
    // Input values
    constexpr std::string_view installerGoodMsix = "Installer-Good.msix";
    constexpr std::string_view installerGoodMsixBundle = "Installer-Good.msixbundle";

    // Expected
    constexpr std::string_view expectedFamilyName = "FakeInstallerForTesting_125rzkzqaqjwj";
    PackageVersion expectedPackageVersion { 0xAAAABBBBCCCCDDDD };
    constexpr std::string_view expectedWindowsDesktopName = "Windows.Desktop";
    OSVersion expectedWindowsDesktopMinVersion { "10.0.16299.0" };
    OSVersion expectedWindowsUniversalMinVersion { "10.0.0.0" };
}

TEST_CASE("MsixManifest_ValidateFieldsParsedFromManifestReader", "[MsixManifest]")
{
    ComPtr<IAppxManifestReader> manifestReader;
    REQUIRE(GetMsixPackageManifestReader(installerGoodMsix, &manifestReader));

    Msix::MsixPackageManifest msixManifest(manifestReader);
    REQUIRE(expectedFamilyName == msixManifest.GetIdentity().GetPackageFamilyName());
    REQUIRE(expectedPackageVersion == msixManifest.GetIdentity().GetVersion());
    REQUIRE(2 == msixManifest.GetTargetDeviceFamilies().size());
    REQUIRE(expectedWindowsUniversalMinVersion == msixManifest.GetMinimumOSVersionForSupportedPlatforms().value());

    auto targets = msixManifest.GetTargetDeviceFamilies();
    auto windowsDesktop = std::find_if(targets.begin(), targets.end(), [](auto& t) { return t.GetMinVersion() == expectedWindowsDesktopMinVersion; });
    REQUIRE(windowsDesktop != targets.end());

    auto windowsUniversal = std::find_if(targets.begin(), targets.end(), [](auto& t) { return t.GetMinVersion() == expectedWindowsUniversalMinVersion; });
    REQUIRE(windowsUniversal != targets.end());
}

TEST_CASE("MsixManifest_ValidateFieldsParsedFromMsix", "[MsixManifest]")
{
    TestDataFile testFile(installerGoodMsix);
    MsixInfo msixInfo(testFile.GetPath());

    auto appPackageManifests = msixInfo.GetAppPackageManifests();
    REQUIRE(1 == appPackageManifests.size());

    auto &appPackageManifest = appPackageManifests[0];
    REQUIRE(expectedFamilyName == appPackageManifest.GetIdentity().GetPackageFamilyName());
    REQUIRE(expectedPackageVersion == appPackageManifest.GetIdentity().GetVersion());
    REQUIRE(2 == appPackageManifest.GetTargetDeviceFamilies().size());
    REQUIRE(expectedWindowsUniversalMinVersion == appPackageManifest.GetMinimumOSVersionForSupportedPlatforms().value());

    auto targets = appPackageManifest.GetTargetDeviceFamilies();
    auto windowsDesktop = std::find_if(targets.begin(), targets.end(), [](auto& t) { return t.GetMinVersion() == expectedWindowsDesktopMinVersion; });
    REQUIRE(windowsDesktop != targets.end());

    auto windowsUniversal = std::find_if(targets.begin(), targets.end(), [](auto& t) { return t.GetMinVersion() == expectedWindowsUniversalMinVersion; });
    REQUIRE(windowsUniversal != targets.end());
}

TEST_CASE("MsixManifest_ValidateFieldsParsedFromMsixBundle", "[MsixManifest]")
{
    TestDataFile testFile(installerGoodMsixBundle);
    MsixInfo msixInfo(testFile.GetPath());
    
    auto appPackageManifests = msixInfo.GetAppPackageManifests();
    REQUIRE(2 == appPackageManifests.size());

    for (auto& appPackageManifest : appPackageManifests)
    {
        REQUIRE(expectedFamilyName == appPackageManifest.GetIdentity().GetPackageFamilyName());
        REQUIRE(expectedPackageVersion == appPackageManifest.GetIdentity().GetVersion());
        REQUIRE(1 == appPackageManifest.GetTargetDeviceFamilies().size());
        REQUIRE(expectedWindowsDesktopName == appPackageManifest.GetTargetDeviceFamilies().front().GetName());
        REQUIRE(expectedWindowsDesktopMinVersion == appPackageManifest.GetTargetDeviceFamilies().front().GetMinVersion());
        REQUIRE(expectedWindowsDesktopMinVersion == appPackageManifest.GetMinimumOSVersionForSupportedPlatforms().value());
    }
}
