// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Archive.h>

using namespace AppInstaller::Archive;
using namespace TestCommon;

constexpr std::string_view s_ZipFile = "TestZip.zip";
constexpr std::string_view s_7zFile = "Test7z.7z";
constexpr std::string_view s_RarFile = "TestRar.rar";
constexpr std::string_view s_TarGzFile = "TestTarGz.tar.gz";
constexpr std::string_view s_TarBz2File = "TestTarBz2.tar.bz2";

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

TEST_CASE("Extract_7zArchive", "[archive]")
{
    TestCommon::TempDirectory tempDirectory("TempDirectory");
    TestDataFile test7z(s_7zFile);

    const auto& test7zPath = test7z.GetPath();
    const auto& tempDirectoryPath = tempDirectory.GetPath();

    HRESULT hr = TryExtractArchive(test7zPath, tempDirectoryPath);

    std::filesystem::path expectedPath = tempDirectoryPath / "test.txt";
    REQUIRE(SUCCEEDED(hr));
    REQUIRE(std::filesystem::exists(expectedPath));
}

TEST_CASE("Scan_7zArchive", "[archive]")
{
    TestDataFile test7z(s_7zFile);

    const auto& test7zPath = test7z.GetPath();
    bool result = ScanZipFile(test7zPath);
    REQUIRE(result);
}

TEST_CASE("Extract_RarArchive", "[archive]")
{
    TestCommon::TempDirectory tempDirectory("TempDirectory");
    TestDataFile testRar(s_RarFile);

    const auto& testRarPath = testRar.GetPath();
    const auto& tempDirectoryPath = tempDirectory.GetPath();

    HRESULT hr = TryExtractArchive(testRarPath, tempDirectoryPath);

    std::filesystem::path expectedPath = tempDirectoryPath / "test.txt";
    REQUIRE(SUCCEEDED(hr));
    REQUIRE(std::filesystem::exists(expectedPath));
}

TEST_CASE("Scan_RarArchive", "[archive]")
{
    TestDataFile testRar(s_RarFile);

    const auto& testRarPath = testRar.GetPath();
    bool result = ScanZipFile(testRarPath);
    REQUIRE(result);
}

TEST_CASE("Extract_TarGzArchive", "[archive]")
{
    TestCommon::TempDirectory tempDirectory("TempDirectory");
    TestDataFile testTarGz(s_TarGzFile);

    const auto& testTarGzPath = testTarGz.GetPath();
    const auto& tempDirectoryPath = tempDirectory.GetPath();

    HRESULT hr = TryExtractArchive(testTarGzPath, tempDirectoryPath);

    std::filesystem::path expectedPath = tempDirectoryPath / "test.txt";
    REQUIRE(SUCCEEDED(hr));
    REQUIRE(std::filesystem::exists(expectedPath));
}

TEST_CASE("Scan_TarGzArchive", "[archive]")
{
    TestDataFile testTarGz(s_TarGzFile);

    const auto& testTarGzPath = testTarGz.GetPath();
    bool result = ScanZipFile(testTarGzPath);
    REQUIRE(result);
}

TEST_CASE("Extract_TarBz2Archive", "[archive]")
{
    TestCommon::TempDirectory tempDirectory("TempDirectory");
    TestDataFile testTarBz2(s_TarBz2File);

    const auto& testTarBz2Path = testTarBz2.GetPath();
    const auto& tempDirectoryPath = tempDirectory.GetPath();

    HRESULT hr = TryExtractArchive(testTarBz2Path, tempDirectoryPath);

    std::filesystem::path expectedPath = tempDirectoryPath / "test.txt";
    REQUIRE(SUCCEEDED(hr));
    REQUIRE(std::filesystem::exists(expectedPath));
}

TEST_CASE("Scan_TarBz2Archive", "[archive]")
{
    TestDataFile testTarBz2(s_TarBz2File);

    const auto& testTarBz2Path = testTarBz2.GetPath();
    bool result = ScanZipFile(testTarBz2Path);
    REQUIRE(result);
}

