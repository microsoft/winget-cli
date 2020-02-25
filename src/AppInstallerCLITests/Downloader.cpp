// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "AppInstallerDownloader.h"
#include "AppInstallerSHA256.h"

using namespace AppInstaller::Utility;
using namespace std::string_literals;

TEST_CASE("DownloadValidFileAndVerifyHash", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test"s, ".test"s);
    INFO("Using temporary file named: " << tempFile.GetPath());

    // Todo: point to files from our repo when the repo goes public
    auto future = DownloadAsync("https://raw.githubusercontent.com/microsoft/msix-packaging/master/LICENSE", tempFile.GetPath(), true);

    auto result = future.Get();

    REQUIRE(result.has_value());
    auto resultHash = result.value();

    auto expectedHash = SHA256::ConvertToBytes("d2a45116709136462ee7a1c42f0e75f0efa258fe959b1504dc8ea4573451b759");
    REQUIRE(std::equal(
        expectedHash.begin(),
        expectedHash.end(),
        resultHash.begin()));

    REQUIRE(std::filesystem::file_size(tempFile.GetPath()) > 0);
}

TEST_CASE("DownloadValidFileAndCancel", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test"s, ".test"s);
    INFO("Using temporary file named: " << tempFile.GetPath());

    auto future = DownloadAsync("https://aka.ms/win32-x64-user-stable", tempFile.GetPath(), true);

    std::optional<std::vector<BYTE>> waitResult;
    std::thread waitThread([&future, &waitResult] { waitResult = future.Get(); });

    future.Cancel();

    waitThread.join();

    REQUIRE(!waitResult.has_value());
}

TEST_CASE("DownloadUnreachableUrl", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test"s, ".test"s);
    INFO("Using temporary file named: " << tempFile.GetPath());

    auto future = DownloadAsync("https://does_not_exist.com/", tempFile.GetPath(), true);

    REQUIRE_THROWS_HR(future.Get(), WININET_E_NAME_NOT_RESOLVED);
}
