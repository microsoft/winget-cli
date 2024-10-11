// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "AppInstallerDownloader.h"
#include "AppInstallerSHA256.h"
#include "HttpStream/HttpLocalCache.h"

using namespace AppInstaller;
using namespace AppInstaller::Utility;
using namespace std::string_literals;

TEST_CASE("DownloadValidFileAndVerifyHash", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test"s, ".test"s);
    INFO("Using temporary file named: " << tempFile.GetPath());

    // Todo: point to files from our repo when the repo goes public
    ProgressCallback callback;
    auto result = Download("https://raw.githubusercontent.com/microsoft/msix-packaging/master/LICENSE", tempFile.GetPath(), DownloadType::Manifest, callback);

    REQUIRE(!result.Sha256Hash.empty());
    auto resultHash = result.Sha256Hash;

    auto expectedHash = SHA256::ConvertToBytes("d2a45116709136462ee7a1c42f0e75f0efa258fe959b1504dc8ea4573451b759");
    REQUIRE(std::equal(
        expectedHash.begin(),
        expectedHash.end(),
        resultHash.begin()));

    uint64_t expectedFileSize = 1119;
    REQUIRE(result.SizeInBytes == expectedFileSize);
    REQUIRE(std::filesystem::file_size(tempFile.GetPath()) == expectedFileSize);

    REQUIRE(result.ContentType);
    REQUIRE(!result.ContentType.value().empty());

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

    DownloadResult waitResult;
    std::thread waitThread([&]
        {
            waitResult = Download("https://aka.ms/win32-x64-user-stable", tempFile.GetPath(), DownloadType::Installer, callback);
        });

    callback.Cancel();

    waitThread.join();

    REQUIRE(waitResult.Sha256Hash.empty());
}

TEST_CASE("DownloadInvalidUrl", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test"s, ".test"s);
    INFO("Using temporary file named: " << tempFile.GetPath());

    ProgressCallback callback;

    REQUIRE_THROWS(Download("blargle-flargle-fluff", tempFile.GetPath(), DownloadType::Installer, callback));
}

TEST_CASE("HttpStream_ReadLastFullPage", "[HttpStream]")
{
    Microsoft::WRL::ComPtr<IStream> stream;
    STATSTG stat = { 0 };

    for (size_t i = 0; i < 10; ++i)
    {
        stream = GetReadOnlyStreamFromURI("https://cdn.winget.microsoft.com/cache/source.msix");

        stat = { 0 };
        REQUIRE(stream->Stat(&stat, STATFLAG_NONAME) == S_OK);

        if (stat.cbSize.QuadPart > 0)
        {
            break;
        }

        Sleep(500);
    }

    {
        INFO("https://cdn.winget.microsoft.com/cache/source.msix gave back a 0 byte file");
        REQUIRE(stream);
    }

    LARGE_INTEGER seek;
    seek.QuadPart = (stat.cbSize.QuadPart / HttpStream::HttpLocalCache::PAGE_SIZE) * HttpStream::HttpLocalCache::PAGE_SIZE;
    REQUIRE(stream->Seek(seek, STREAM_SEEK_SET, nullptr) == S_OK);

    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(HttpStream::HttpLocalCache::PAGE_SIZE);
    ULONG read = 0;
    REQUIRE(stream->Read(buffer.get(), static_cast<ULONG>(HttpStream::HttpLocalCache::PAGE_SIZE), &read) >= S_OK);
    REQUIRE(read == (stat.cbSize.QuadPart % HttpStream::HttpLocalCache::PAGE_SIZE));
}
