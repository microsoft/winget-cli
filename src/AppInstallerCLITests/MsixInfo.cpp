// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerMsixInfo.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;

constexpr std::string_view s_Msix_url = "https://github.com/microsoft/msix-packaging/blob/master/src/test/testData/unpack/TestAppxPackage_x64.appx?raw=true";

TEST_CASE("MsixInfo_GetPackageFamilyName", "[msixinfo]")
{
    Msix::MsixInfo msix(s_Msix_url);

    std::string expectedFamilyName = "20477fca-282d-49fb-b03e-371dca074f0f_8wekyb3d8bbwe";
    std::string actualFamilyName = msix.GetPackageFamilyName();

    REQUIRE(expectedFamilyName == actualFamilyName);
}

TEST_CASE("MsixInfo_WriteManifestAndCompare", "[msixinfo]")
{
    Msix::MsixInfo msix(s_Msix_url);

    TempFile manifest{ "msixtest_manifest"s, ".xml"s };
    ProgressCallback callback;

    msix.WriteManifestToFile(manifest, callback);

    REQUIRE(!msix.IsNewerThan(manifest));
}

TEST_CASE("MsixInfo_WriteFile", "[msixinfo]")
{
    Msix::MsixInfo msix(s_Msix_url);

    TempFile file{ "msixtest_file"s, ".bin"s };
    ProgressCallback callback;

    msix.WriteToFile("TestAppxPackage.winmd", file, callback);

    wil::unique_handle actualFile(CreateFile(file.GetPath().c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    LARGE_INTEGER size{};
    REQUIRE(GetFileSizeEx(actualFile.get(), &size));

    REQUIRE(3072 == size.QuadPart);
}
