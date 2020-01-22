// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "AppInstallerDownloader.h"
#include "AppInstallerSHA256.h"

using namespace AppInstaller::Utility;

TEST_CASE("DownloadValidFileAndVerifyHash", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test", ".test");
    INFO("Using temporary file named: " << tempFile.GetPath());

    // Todo: point to files from our repo when the repo goes public
    auto downloader = Downloader::StartDownloadAsync("https://raw.githubusercontent.com/microsoft/msix-packaging/master/LICENSE", tempFile.GetPath(), true);

    auto result = downloader->Wait();

    REQUIRE(result == DownloaderResult::Success);

    auto expectedHash = SHA256::ConvertToBytes("d2a45116709136462ee7a1c42f0e75f0efa258fe959b1504dc8ea4573451b759");
    REQUIRE(std::equal(
        expectedHash.begin(),
        expectedHash.end(),
        downloader->GetDownloadHash().begin()));

    REQUIRE(std::filesystem::file_size(tempFile.GetPath()) > 0);
}

TEST_CASE("DownloadValidFileAndCancel", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test", ".test");
    INFO("Using temporary file named: " << tempFile.GetPath());

    auto downloader = Downloader::StartDownloadAsync("https://aka.ms/win32-x64-user-stable", tempFile.GetPath(), true);

    DownloaderResult cancelResult;
    std::thread cancelThread([&downloader, &cancelResult] { cancelResult = downloader->Cancel();});
    cancelThread.join();

    auto downloadResult = downloader->Wait();

    REQUIRE(downloadResult == cancelResult);
    REQUIRE(downloadResult == DownloaderResult::Canceled);

    REQUIRE_THROWS(downloader->GetDownloadHash());
}

TEST_CASE("DownloadUnreachableUrl", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test", ".test");
    INFO("Using temporary file named: " << tempFile.GetPath());

    auto downloader = Downloader::StartDownloadAsync("https://does_not_exist.com/", tempFile.GetPath(), true);

    auto result = downloader->Wait();

    REQUIRE(result == DownloaderResult::Failed);

    REQUIRE_THROWS(downloader->GetDownloadHash());
}