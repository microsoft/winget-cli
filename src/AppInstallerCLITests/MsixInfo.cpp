// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerMsixInfo.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerRuntime.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;

constexpr std::string_view s_MsixFile_1 = "index.1.0.0.0.msix";
constexpr std::string_view s_MsixFile_2 = "index.2.0.0.0.msix";
constexpr std::string_view s_MsixFileSigned_1 = "index.1.0.0.0.signed.msix";

TEST_CASE("MsixInfo_GetPackageFullName", "[msixinfo]")
{
    TestDataFile index(s_MsixFile_1);
    Msix::MsixInfo msix(index.GetPath());

    std::string expectedFullName = "AppInstallerCLITestsFakeIndex_1.0.0.0_neutral__125rzkzqaqjwj";
    std::string actualFullName = msix.GetPackageFullName();

    REQUIRE(expectedFullName == actualFullName);
}

TEST_CASE("MsixInfo_CompareToSelf", "[msixinfo]")
{
    TestDataFile index(s_MsixFile_1);
    Msix::MsixInfo msix(index.GetPath());

    REQUIRE(!msix.IsNewerThan(index.GetPath().u8string()));
}

TEST_CASE("MsixInfo_CompareToOlder", "[msixinfo]")
{
    TestDataFile index1(s_MsixFile_1);
    TestDataFile index2(s_MsixFile_2);
    Msix::MsixInfo msix2(index2.GetPath());

    REQUIRE(msix2.IsNewerThan(index1));
}

TEST_CASE("MsixInfo_WriteFile", "[msixinfo]")
{
    TestDataFile index(s_MsixFile_1);
    Msix::MsixInfo msix(index.GetPath());

    TempFile file{ "msixtest_file"s, ".bin"s };
    ProgressCallback callback;

    msix.WriteToFile("Public\\index.db", file, callback);

    REQUIRE(1 == std::filesystem::file_size(file));
}

TEST_CASE("MsixInfo_ValidateMsixTrustInfo", "[msixinfo]")
{
    if (!Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestDataFile notSigned{ s_MsixFile_1 };
    Msix::WriteLockedMsixFile notSignedWriteLocked{ notSigned };
    REQUIRE_FALSE(notSignedWriteLocked.ValidateTrustInfo(false));

    TestDataFile testSigned{ s_MsixFileSigned_1 };
    Msix::WriteLockedMsixFile testSignedWriteLocked{ testSigned };

    // Remove the cert if already trusted
    bool certExistsBeforeTest = UninstallCertFromSignedPackage(testSigned);

    REQUIRE_FALSE(testSignedWriteLocked.ValidateTrustInfo(false));

    // Add the cert to trusted
    InstallCertFromSignedPackage(testSigned);

    REQUIRE(testSignedWriteLocked.ValidateTrustInfo(false));
    REQUIRE_FALSE(testSignedWriteLocked.ValidateTrustInfo(true));

    TestCommon::TempFile microsoftSigned{ "testIndex"s, ".msix"s };
    ProgressCallback callback;
    Utility::Download("https://cdn.winget.microsoft.com/cache/source.msix", microsoftSigned.GetPath(), Utility::DownloadType::Index, callback);

    Msix::WriteLockedMsixFile microsoftSignedWriteLocked{ microsoftSigned };
    REQUIRE(microsoftSignedWriteLocked.ValidateTrustInfo(true));

    if (!certExistsBeforeTest)
    {
        UninstallCertFromSignedPackage(testSigned);
    }
}

TEST_CASE("MsixInfo_GetPackageIdInfoFromFullName", "[msixinfo]")
{
    auto testPackageIdInfo = Msix::GetPackageIdInfoFromFullName("Microsoft.NET.Native.Framework.2.2_2.2.29512.0_arm64__8wekyb3d8bbwe");
    REQUIRE(testPackageIdInfo.Name == "Microsoft.NET.Native.Framework.2.2");
    REQUIRE(testPackageIdInfo.Version == Utility::UInt64Version{ "2.2.29512.0" });

    auto testPackageIdInfo2 = Msix::GetPackageIdInfoFromFullName("Microsoft.DoesNotExist_1.2.3.4_neutral_~_8wekyb3d8bbwe");
    REQUIRE(testPackageIdInfo2.Name == "Microsoft.DoesNotExist");
    REQUIRE(testPackageIdInfo2.Version == Utility::UInt64Version{ "1.2.3.4" });
}
