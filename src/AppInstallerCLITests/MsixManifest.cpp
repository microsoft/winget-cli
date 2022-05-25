// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerMsixInfo.h>
#include <AppInstallerMsixManifest.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerRuntime.h>

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
UINT64 expectedPackageVersion = 0xAAAABBBBCCCCDDDD;
UINT64 expectedWindowsDesktopMinVersion = 0x000a00003FAB0000; // 10.0.16299.0
UINT64 expectedWindowsDesktopMaxVersionTested = 0x000a0000476F0000; // 10.0.18287.0
UINT64 expectedWindowsUniversalMinVersion = 0x000a000000000000; // 10.0.0.0
UINT64 expectedWindowsUniversalMaxVersionTested = 0x000a000000000000; // 10.0.0.0

TEST_CASE("MsixManifest_ValidateFieldsParsedFromManifestReader", "[MsixManifest]")
{
    ComPtr<IAppxManifestReader> manifestReader;
    if (!GetMsixPackageManifestReader(installerManifestValidationMsix, &manifestReader))
    {
        FAIL();
    }

    Msix::MsixPackageManifest msixManifest(manifestReader);
    REQUIRE(msixManifest.Identity.PackageFamilyName == expectedFamilyName);
    REQUIRE(msixManifest.Identity.Version == expectedPackageVersion);
    REQUIRE(msixManifest.Dependencies.TargetDeviceFamilies.size() == 2);

    auto& targets = msixManifest.Dependencies.TargetDeviceFamilies;
    auto windowsDesktop = std::find_if(targets.begin(), targets.end(), [](auto& t) { return Platform::IsWindowsDesktop(t.Name); });
    REQUIRE(windowsDesktop->MinVersion == expectedWindowsDesktopMinVersion);
    REQUIRE(windowsDesktop->MaxVersionTested == expectedWindowsDesktopMaxVersionTested);

    auto windowsUniversal = std::find_if(targets.begin(), targets.end(), [](auto& t) { return Platform::IsWindowsUniversal(t.Name); });
    REQUIRE(windowsUniversal->MinVersion == expectedWindowsUniversalMinVersion);
    REQUIRE(windowsUniversal->MaxVersionTested == expectedWindowsUniversalMaxVersionTested);
}

TEST_CASE("MsixManifest_ValidateFieldsParsedFromMsixBundle", "[MsixManifest]")
{
    TestDataFile testFile(installerManifestValidationMsixBundle);
    MsixInfo msixInfo(testFile.GetPath());
    
    auto appPackageManifests = msixInfo.GetAppPackageManifests();
    REQUIRE(appPackageManifests.size() == 2);

    for (auto& appPackageManifest : appPackageManifests)
    {
        REQUIRE(appPackageManifest.Identity.PackageFamilyName == expectedFamilyName);
        REQUIRE(appPackageManifest.Identity.Version == expectedPackageVersion);
        REQUIRE(appPackageManifest.Dependencies.TargetDeviceFamilies.size() == 1);
        REQUIRE(Platform::IsWindowsDesktop(appPackageManifest.Dependencies.TargetDeviceFamilies.front().Name));
        REQUIRE(appPackageManifest.Dependencies.TargetDeviceFamilies.front().MinVersion == expectedWindowsDesktopMinVersion);
        REQUIRE(appPackageManifest.Dependencies.TargetDeviceFamilies.front().MaxVersionTested == expectedWindowsDesktopMaxVersionTested);
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
