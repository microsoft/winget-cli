// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerMsixInfo.h>
#include "AppInstallerDownloader.h"

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;

constexpr std::string_view s_MsixFile_1 = "index.1.0.0.0.msix";
constexpr std::string_view s_MsixFile_2 = "index.2.0.0.0.msix";
constexpr std::string_view s_MsixFileSigned_1 = "index.1.0.0.0.signed.msix";

TEST_CASE("MsixInfo_GetPackageFamilyName", "[msixinfo]")
{
    TestDataFile index(s_MsixFile_1);
    Msix::MsixInfo msix(index.GetPath().u8string());

    std::string expectedFullName = "AppInstallerCLITestsFakeIndex_1.0.0.0_neutral__125rzkzqaqjwj";
    std::string actualFullName = msix.GetPackageFullName();

    REQUIRE(expectedFullName == actualFullName);
}

TEST_CASE("MsixInfo_CompareToSelf", "[msixinfo]")
{
    TestDataFile index(s_MsixFile_1);
    Msix::MsixInfo msix(index.GetPath().u8string());

    REQUIRE(!msix.IsNewerThan(index.GetPath().u8string()));
}

TEST_CASE("MsixInfo_CompareToOlder", "[msixinfo]")
{
    TestDataFile index1(s_MsixFile_1);
    TestDataFile index2(s_MsixFile_2);
    Msix::MsixInfo msix2(index2.GetPath().u8string());

    REQUIRE(msix2.IsNewerThan(index1));
}

TEST_CASE("MsixInfo_WriteFile", "[msixinfo]")
{
    TestDataFile index(s_MsixFile_1);
    Msix::MsixInfo msix(index.GetPath().u8string());

    TempFile file{ "msixtest_file"s, ".bin"s };
    ProgressCallback callback;

    msix.WriteToFile("Public\\index.db", file, callback);

    REQUIRE(1 == std::filesystem::file_size(file));
}

TEST_CASE("MsixInfo_ValidateMsixTrustInfo", "[msixinfo]")
{
    TestDataFile notSigned{ s_MsixFile_1 };
    REQUIRE_FALSE(Msix::ValidateMsixTrustInfo(notSigned));

    TestDataFile testSigned{ s_MsixFileSigned_1 };

    // Remove the cert if already trusted
    bool certExistsBeforeTest = UninstallCertFromSignedPackage(testSigned);

    REQUIRE_FALSE(Msix::ValidateMsixTrustInfo(testSigned));

    // Add the cert to trusted
    InstallCertFromSignedPackage(testSigned);
    REQUIRE(Msix::ValidateMsixTrustInfo(testSigned));
    REQUIRE_FALSE(Msix::ValidateMsixTrustInfo(testSigned, true));

    TestCommon::TempFile microsoftSigned{ "testIndex"s, ".msix"s };
    ProgressCallback callback;
    Utility::Download("https://cdn.winget.microsoft.com/cache/source.msix", microsoftSigned.GetPath(), Utility::DownloadType::Index, callback);
    REQUIRE(Msix::ValidateMsixTrustInfo(microsoftSigned, true));

    if (!certExistsBeforeTest)
    {
        UninstallCertFromSignedPackage(testSigned);
    }
}