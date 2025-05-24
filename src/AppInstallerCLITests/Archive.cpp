// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "WorkflowCommon.h"
#include <winget/Archive.h>
#include <Workflows/ShellExecuteInstallerHandler.h>

using namespace AppInstaller::Archive;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Settings;
using namespace TestCommon;

constexpr std::string_view s_ZipFile = "TestZip.zip";
constexpr std::string_view s_TarGzFile = "TestTarGz.tar.gz";
constexpr std::string_view s_Large7zFile = "TestLarge7z.7z";

TEST_CASE("Extract_ZipArchive", "[archive]")
{
    TestCommon::TempDirectory tempDirectory("TempDirectory");
    TestDataFile testZip(s_ZipFile);

    const auto& testZipPath = testZip.GetPath();
    const auto& tempDirectoryPath = tempDirectory.GetPath();

    HRESULT hr = TryExtractArchive(testZipPath, tempDirectoryPath);

    std::filesystem::path expectedPath = tempDirectoryPath / "test.txt";
    REQUIRE(SUCCEEDED(hr));
    REQUIRE(std::filesystem::exists(expectedPath));
}

TEST_CASE("Scan_ZipArchive", "[archive]")
{
    TestDataFile testZip(s_ZipFile);

    const auto& testZipPath = testZip.GetPath();
    bool result = ScanZipFile(testZipPath);
    REQUIRE(result);
}

TEST_CASE("Extract_TarGzArchive", "[archive]")
{
    TestCommon::TempDirectory tempDirectory("TempDirectory");
    TestDataFile testTarGz(s_TarGzFile);

    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::ArchiveExtractionMethod>(AppInstaller::Archive::ExtractionMethod::Tar);

    const auto& testTarGzPath = testTarGz.GetPath();
    const auto& tempDirectoryPath = tempDirectory.GetPath();

    ShellExecuteExtractArchive(testTarGzPath, tempDirectoryPath);

    std::ostringstream extractOutput;
    TestContext context{ extractOutput, std::cin };
    context << ShellExecuteExtractArchive(testTarGzPath, tempDirectoryPath);

    std::filesystem::path expectedPath = tempDirectoryPath / "test.txt";
    REQUIRE(SUCCEEDED(context.GetTerminationHR()));
    REQUIRE(std::filesystem::exists(expectedPath));
    INFO(extractOutput.str());
}

TEST_CASE("Scan_TarGzArchive", "[archive]")
{
    TestDataFile testTarGz(s_TarGzFile);

    const auto& testTarGzPath = testTarGz.GetPath();
    bool result = ScanZipFile(testTarGzPath);
    REQUIRE(result);
}

//TEST_CASE("Scan_Large7zArchive", "[archive]")
//{
//    TestDataFile testLarge7z(s_Large7zFile);
//
//    const auto& testTarGzPath = testLarge7z.GetPath();
//    bool result = ScanZipFile(testTarGzPath);
//    REQUIRE(result);
//}

