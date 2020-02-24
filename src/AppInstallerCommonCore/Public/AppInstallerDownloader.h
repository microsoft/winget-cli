// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerFuture.h>

namespace AppInstaller::Utility
{


    // This is the only method to get a Downloader instance.
    //   url: The url to be downloaded from. http->https redirection is allowed.
    //   dest: The path to local file to be downloaded to.
    //   computeHash: Optional. Indicates if SHA256 hash should be calculated when downloading.
    //   callback: Optional. Pass in an object implementing IDownloaderCallback to receive download updates.
    Future<std::vector<BYTE>> DownloadAsync(
        const std::string& url,
        const std::filesystem::path& dest,
        bool computeHash = false,
        IDownloaderCallback* callback = nullptr);

    // Downloader class to handle 1 file download per instance. The Downloader class supports
    // SHA 256 calculation as downloading happens.
    class Downloader
    {
    public:

        // Cancel the download.
        DownloaderResult Cancel();

        // Wait for the download to finish.
        DownloaderResult Wait();

        // Get download content hash only if download is success and hash calculation is requested.
        std::vector<BYTE> GetDownloadHash();

    private:
        std::shared_future<DownloaderResult> m_downloadTask;
        std::atomic<bool> m_cancelled = false;
        std::vector<BYTE> m_downloadHash;

        Downloader() {};

        // The internal method which does actual downloading.
        DownloaderResult DownloadInternal(
            const std::string& url,
            const std::filesystem::path& dest,
            bool computeHash,
            IDownloaderCallback* callback);
    };
}