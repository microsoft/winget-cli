// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Archive.h>

// Defined in PureLib, which is compiled into AppInstallerCommonCore.
int pure_zip(const uint8_t* buffer, const uint64_t size, const uint64_t flags);
const char* pure_error_code(const int error);

using namespace AppInstaller::Archive;
using namespace TestCommon;

constexpr std::string_view s_ZipFile = "TestZip.zip";
constexpr std::string_view s_SymlinkName = "link";
constexpr std::string_view s_SymlinkTarget = "target.txt";

namespace
{
    void AppendUInt16(std::vector<uint8_t>& zip, uint16_t value)
    {
        zip.push_back(static_cast<uint8_t>(value));
        zip.push_back(static_cast<uint8_t>(value >> 8));
    }

    void AppendUInt32(std::vector<uint8_t>& zip, uint32_t value)
    {
        AppendUInt16(zip, static_cast<uint16_t>(value));
        AppendUInt16(zip, static_cast<uint16_t>(value >> 16));
    }

    void AppendString(std::vector<uint8_t>& zip, std::string_view value)
    {
        zip.insert(zip.end(), value.begin(), value.end());
    }

    // Fields shared by the local file header and the central directory header.
    void AppendEntryFields(std::vector<uint8_t>& zip, uint32_t dataSize)
    {
        AppendUInt16(zip, 20); // Version needed
        AppendUInt16(zip, 0); // Flags
        AppendUInt16(zip, 0); // Stored
        AppendUInt16(zip, 0); // Time
        AppendUInt16(zip, 0x5C21); // Date
        AppendUInt32(zip, 0x00A4BA49); // CRC-32 of s_SymlinkTarget
        AppendUInt32(zip, dataSize); // Compressed size
        AppendUInt32(zip, dataSize); // Uncompressed size
        AppendUInt16(zip, static_cast<uint16_t>(s_SymlinkName.size()));
        AppendUInt16(zip, 0); // Extra length
    }

    // Creates a ZIP with a stored Unix symlink whose headers declare dataSize bytes of target data.
    std::vector<uint8_t> CreateSymlinkZip(uint32_t dataSize)
    {
        std::vector<uint8_t> zip;

        AppendUInt32(zip, 0x04034B50); // Local file header
        AppendEntryFields(zip, dataSize);
        AppendString(zip, s_SymlinkName);
        AppendString(zip, s_SymlinkTarget);

        uint32_t centralDirectoryOffset = static_cast<uint32_t>(zip.size());
        AppendUInt32(zip, 0x02014B50); // Central directory header
        AppendUInt16(zip, 0x0314); // Made by Unix
        AppendEntryFields(zip, dataSize);
        AppendUInt16(zip, 0); // Comment length
        AppendUInt16(zip, 0); // Disk
        AppendUInt16(zip, 0); // Internal attributes
        AppendUInt32(zip, 0xA1FF0000); // Unix symlink mode
        AppendUInt32(zip, 0); // Local header offset
        AppendString(zip, s_SymlinkName);

        uint32_t centralDirectorySize = static_cast<uint32_t>(zip.size()) - centralDirectoryOffset;
        AppendUInt32(zip, 0x06054B50); // End of central directory
        AppendUInt16(zip, 0); // Disk
        AppendUInt16(zip, 0); // Central directory disk
        AppendUInt16(zip, 1); // Entries on disk
        AppendUInt16(zip, 1); // Total entries
        AppendUInt32(zip, centralDirectorySize);
        AppendUInt32(zip, centralDirectoryOffset);
        AppendUInt16(zip, 0); // Comment length

        return zip;
    }

    std::string_view GetScanResult(const std::vector<uint8_t>& zip)
    {
        return pure_error_code(pure_zip(zip.data(), zip.size(), 0));
    }
}

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

TEST_CASE("Scan_ZipArchive_Symlink", "[archive]")
{
    auto zip = CreateSymlinkZip(static_cast<uint32_t>(s_SymlinkTarget.size()));
    REQUIRE(GetScanResult(zip) == "PURE_E_OK");
}

TEST_CASE("Scan_ZipArchive_SymlinkDataPastEnd", "[archive]")
{
    auto zip = CreateSymlinkZip(static_cast<uint32_t>(s_SymlinkTarget.size()));

    // Declare one byte more than the buffer holds after the 30-byte local file header and name.
    auto dataSize = zip.size() - 30 - s_SymlinkName.size() + 1;
    zip = CreateSymlinkZip(static_cast<uint32_t>(dataSize));

    REQUIRE(GetScanResult(zip) == "PURE_E_ZIP_LFH_DATA_OVERFLOW");
}
