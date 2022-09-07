// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Archive.h>

using namespace AppInstaller::Archive;
using namespace TestCommon;

constexpr std::string_view s_ZipFile = "TestZip.zip";

TEST_CASE("Extract_ZipArchive", "[archive]")
{
    TestCommon::TempDirectory tempDirectory("TempDirectory");
    TestDataFile testZip(s_ZipFile);

    const auto& testZipPath = testZip.GetPath();
    const auto& tempDirectoryPath = tempDirectory.GetPath();

    std::vector<std::filesystem::path> extractedItems;
    HRESULT hr = TryExtractArchive(testZipPath, tempDirectoryPath, extractedItems);

    REQUIRE(SUCCEEDED(hr));
    REQUIRE(std::filesystem::exists(tempDirectoryPath / "test.txt"));
    REQUIRE(extractedItems.size() == 1);
}