// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerMsixInfo.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;

constexpr std::string_view s_MsixFile_1 = "index.1.0.0.0.msix";
constexpr std::string_view s_MsixFile_2 = "index.2.0.0.0.msix";

TEST_CASE("MsixInfo_GetPackageFamilyName", "[msixinfo]")
{
    TestDataFile index(s_MsixFile_1);
    Msix::MsixInfo msix(index.GetPath().u8string());

    std::string expectedFullName = "AppInstallerCLITestsFakeIndex_1.0.0.0_neutral__125rzkzqaqjwj";
    std::string actualFullName = msix.GetPackageFullName();

    REQUIRE(expectedFullName == actualFullName);
}

TEST_CASE("MsixInfo_WriteManifestAndCompareToSelf", "[msixinfo]")
{
    TestDataFile index(s_MsixFile_1);
    Msix::MsixInfo msix(index.GetPath().u8string());

    TempFile manifest{ "msixtest_manifest"s, ".xml"s };
    ProgressCallback callback;

    msix.WriteManifestToFile(manifest, callback);

    REQUIRE(!msix.IsNewerThan(manifest));
}

TEST_CASE("MsixInfo_WriteManifestAndCompareToOlder", "[msixinfo]")
{
    TestDataFile index1(s_MsixFile_1);
    Msix::MsixInfo msix1(index1.GetPath().u8string());

    TempFile manifest{ "msixtest_manifest"s, ".xml"s };
    ProgressCallback callback;

    msix1.WriteManifestToFile(manifest, callback);

    TestDataFile index2(s_MsixFile_2);
    Msix::MsixInfo msix2(index2.GetPath().u8string());

    REQUIRE(msix2.IsNewerThan(manifest));
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
