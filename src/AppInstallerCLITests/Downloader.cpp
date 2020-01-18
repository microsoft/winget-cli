// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "AppInstallerDownloader.h"
#include "AppInstallerSHA256.h"

using namespace AppInstaller::Utility;

TEST_CASE("DownloadVSCodeAndVerifyHash", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test", ".test");
    INFO("Using temporary file named: " << tempFile.GetPath());

    auto downloader = Downloader::StartDownloadAsync("https://aka.ms/win32-x64-user-stable", tempFile.GetPath(), true);

    auto result = downloader->Wait();

    REQUIRE(result == DownloaderResult::Success);

    auto expectedHash = SHA256::ConvertToBytes("65DB2F2AC2686C7F2FD69D4A4C6683B888DC55BFA20A0E32CA9F838B51689A3B");
    REQUIRE(std::equal(
        expectedHash.begin(),
        expectedHash.end(),
        downloader->GetDownloadHash().begin()));

    REQUIRE(std::filesystem::file_size(tempFile.GetPath()) > 0);
}

TEST_CASE("DownloadVSCodeAndCancel", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test", ".test");
    INFO("Using temporary file named: " << tempFile.GetPath());

    auto downloader = Downloader::StartDownloadAsync("https://aka.ms/win32-x64-user-stable", tempFile.GetPath(), true);

    auto result = downloader->Cancel();

    REQUIRE(result == DownloaderResult::Canceled);

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