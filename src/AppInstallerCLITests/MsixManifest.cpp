// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerMsixInfo.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerRuntime.h>
#include <winget/MsixManifest.h>

using namespace std;
using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Msix;
using namespace Microsoft::WRL;

// Input values
constexpr std::string_view installerManifestValidationMsix = "InstallerManifestValidation.msix";
constexpr std::string_view installerManifestValidationMsixBundle = "InstallerManifestValidation.msixbundle";

// Expected 
constexpr std::string_view expectedFamilyName = "FakeInstallerForTesting_125rzkzqaqjwj";
PackageVersion expectedPackageVersion = { 0xAAAABBBBCCCCDDDD };
OSVersion expectedWindowsDesktopMinVersion = { 0x000a00003FAB0000 }; // 10.0.16299.0
OSVersion expectedWindowsDesktopMaxVersionTested = { 0x000a0000476F0000 }; // 10.0.18287.0
OSVersion expectedWindowsUniversalMinVersion = { 0x000a000000000000 }; // 10.0.0.0
OSVersion expectedWindowsUniversalMaxVersionTested = { 0x000a000000000000 }; // 10.0.0.0

TEST_CASE("MsixManifest_ValidateFieldsParsedFromManifestReader", "[MsixManifest]")
{
    ComPtr<IAppxManifestReader> manifestReader;
    if (!GetMsixPackageManifestReader(installerManifestValidationMsix, &manifestReader))
    {
        FAIL();
    }

    Msix::MsixPackageManifest msixManifest(manifestReader);
    REQUIRE(expectedFamilyName == msixManifest.Identity.PackageFamilyName);
    REQUIRE(expectedPackageVersion == msixManifest.Identity.Version);
    REQUIRE(2 == msixManifest.Dependencies.TargetDeviceFamilies.size());

    auto& targets = msixManifest.Dependencies.TargetDeviceFamilies;
    auto windowsDesktop = std::find_if(targets.begin(), targets.end(), [](auto& t) { return Platform::IsWindowsDesktop(t.Name); });
    REQUIRE(expectedWindowsDesktopMinVersion == windowsDesktop->MinVersion);
    REQUIRE(expectedWindowsDesktopMaxVersionTested == windowsDesktop->MaxVersionTested);

    auto windowsUniversal = std::find_if(targets.begin(), targets.end(), [](auto& t) { return Platform::IsWindowsUniversal(t.Name); });
    REQUIRE(expectedWindowsUniversalMinVersion == windowsUniversal->MinVersion);
    REQUIRE(expectedWindowsUniversalMaxVersionTested == windowsUniversal->MaxVersionTested);
}

TEST_CASE("MsixManifest_ValidateFieldsParsedFromMsix", "[MsixManifest]")
{
    TestDataFile testFile(installerManifestValidationMsix);
    MsixInfo msixInfo(testFile.GetPath());

    auto appPackageManifests = msixInfo.GetAppPackageManifests();
    REQUIRE(1 == appPackageManifests.size());

    auto &appPackageManifest = appPackageManifests[0];
    REQUIRE(expectedFamilyName == appPackageManifest.Identity.PackageFamilyName);
    REQUIRE(expectedPackageVersion == appPackageManifest.Identity.Version);
    REQUIRE(2 == appPackageManifest.Dependencies.TargetDeviceFamilies.size());

    auto& targets = appPackageManifest.Dependencies.TargetDeviceFamilies;
    auto windowsDesktop = std::find_if(targets.begin(), targets.end(), [](auto& t) { return Platform::IsWindowsDesktop(t.Name); });
    REQUIRE(expectedWindowsDesktopMinVersion == windowsDesktop->MinVersion);
    REQUIRE(expectedWindowsDesktopMaxVersionTested == windowsDesktop->MaxVersionTested);

    auto windowsUniversal = std::find_if(targets.begin(), targets.end(), [](auto& t) { return Platform::IsWindowsUniversal(t.Name); });
    REQUIRE(expectedWindowsUniversalMinVersion == windowsUniversal->MinVersion);
    REQUIRE(expectedWindowsUniversalMaxVersionTested == windowsUniversal->MaxVersionTested);
}

TEST_CASE("MsixManifest_ValidateFieldsParsedFromMsixBundle", "[MsixManifest]")
{
    TestDataFile testFile(installerManifestValidationMsixBundle);
    MsixInfo msixInfo(testFile.GetPath());
    
    auto appPackageManifests = msixInfo.GetAppPackageManifests();
    REQUIRE(2 == appPackageManifests.size());

    for (auto& appPackageManifest : appPackageManifests)
    {
        REQUIRE(expectedFamilyName == appPackageManifest.Identity.PackageFamilyName);
        REQUIRE(expectedPackageVersion == appPackageManifest.Identity.Version);
        REQUIRE(1 == appPackageManifest.Dependencies.TargetDeviceFamilies.size());
        REQUIRE(Platform::IsWindowsDesktop(appPackageManifest.Dependencies.TargetDeviceFamilies.front().Name));
        REQUIRE(expectedWindowsDesktopMinVersion == appPackageManifest.Dependencies.TargetDeviceFamilies.front().MinVersion);
        REQUIRE(expectedWindowsDesktopMaxVersionTested == appPackageManifest.Dependencies.TargetDeviceFamilies.front().MaxVersionTested);
    }
}

TEST_CASE("Platform_IsWindowsDesktop_Success", "[MsixManifest]")
{
    REQUIRE(Msix::Platform::IsWindowsDesktop("Windows.Desktop"));
    REQUIRE(Msix::Platform::IsWindowsDesktop("WINDOWS.DESKTOP"));
    REQUIRE(Msix::Platform::IsWindowsDesktop("windows.desktop"));
}

TEST_CASE("Platform_IsWindowsDesktop_Fail", "[MsixManifest]")
{
    REQUIRE_FALSE(Msix::Platform::IsWindowsDesktop("mock"));
    REQUIRE_FALSE(Msix::Platform::IsWindowsDesktop(""));
}

TEST_CASE("Platform_IsWindowsUniversal_Success", "[MsixManifest]")
{
    REQUIRE(Msix::Platform::IsWindowsUniversal("Windows.Universal"));
    REQUIRE(Msix::Platform::IsWindowsUniversal("WINDOWS.UNIVERSAL"));
    REQUIRE(Msix::Platform::IsWindowsUniversal("windows.universal"));
}

TEST_CASE("Platform_IsWindowsUniversal_Fail", "[MsixManifest]")
{
    REQUIRE_FALSE(Msix::Platform::IsWindowsUniversal("mock"));
    REQUIRE_FALSE(Msix::Platform::IsWindowsUniversal(""));
}
