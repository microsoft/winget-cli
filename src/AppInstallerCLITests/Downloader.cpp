// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "AppInstallerDownloader.h"
#include "AppInstallerSHA256.h"

using namespace AppInstaller;
using namespace AppInstaller::Utility;
using namespace std::string_literals;

TEST_CASE("DownloadValidFileAndVerifyHash", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test"s, ".test"s);
    INFO("Using temporary file named: " << tempFile.GetPath());

    // Todo: point to files from our repo when the repo goes public
    ProgressCallback callback;
    auto result = Download("https://raw.githubusercontent.com/microsoft/msix-packaging/master/LICENSE", tempFile.GetPath(), DownloadType::Manifest, callback, true);

    REQUIRE(result.has_value());
    auto resultHash = result.value();

    auto expectedHash = SHA256::ConvertToBytes("d2a45116709136462ee7a1c42f0e75f0efa258fe959b1504dc8ea4573451b759");
    REQUIRE(std::equal(
        expectedHash.begin(),
        expectedHash.end(),
        resultHash.begin()));

    REQUIRE(std::filesystem::file_size(tempFile.GetPath()) > 0);

    // Verify motw content
    std::filesystem::path motwFile(tempFile);
    motwFile += ":Zone.Identifier:$data";
    std::ifstream motwStream(motwFile);
    std::stringstream motwContent;
    motwContent << motwStream.rdbuf();
    std::string motwContentStr = motwContent.str();
    REQUIRE(motwContentStr.find("ZoneId=3") != std::string::npos);
}

TEST_CASE("DownloadValidFileAndCancel", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test"s, ".test"s);
    INFO("Using temporary file named: " << tempFile.GetPath());

    ProgressCallback callback;

    std::optional<std::vector<BYTE>> waitResult;
    std::thread waitThread([&]
        {
            waitResult = Download("https://aka.ms/win32-x64-user-stable", tempFile.GetPath(), DownloadType::Installer, callback, true);
        });

    callback.Cancel();

    waitThread.join();

    REQUIRE(!waitResult.has_value());
}

TEST_CASE("DownloadInvalidUrl", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test"s, ".test"s);
    INFO("Using temporary file named: " << tempFile.GetPath());

    ProgressCallback callback;

    REQUIRE_THROWS(Download("blargle-flargle-fluff", tempFile.GetPath(), DownloadType::Installer, callback, true));
}
