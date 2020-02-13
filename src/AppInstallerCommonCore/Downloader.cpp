// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerDownloader.h"
#include "Public/AppInstallerSHA256.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerLogging.h"

using namespace AppInstaller::Runtime;

namespace AppInstaller::Utility
{
    std::unique_ptr<Downloader> Downloader::StartDownloadAsync(
        const std::string& url,
        const std::filesystem::path& dest,
        bool computeHash,
        IDownloaderCallback* callback)
    {
        // std::make_unique cannot access private constructor
        auto downloader = std::unique_ptr<Downloader>(new Downloader());

        downloader->m_downloadTask = std::async(std::launch::async, &Downloader::DownloadInternal, downloader.get(), url, dest, computeHash, callback);

        return downloader;
    }

    DownloaderResult Downloader::DownloadInternal(
        const std::string& url,
        const std::filesystem::path& dest,
        bool computeHash,
        IDownloaderCallback* callback)
    {
        try
        {
            AICLI_LOG(CLI, Info, << "Downloading url: " << url << " , dest: " << dest);

            wil::unique_hinternet session(InternetOpenA(
                "appinstaller-cli",
                INTERNET_OPEN_TYPE_PRECONFIG,
                NULL,
                NULL,
                0));
            THROW_LAST_ERROR_IF_NULL_MSG(session, "InternetOpen() failed.");

            wil::unique_hinternet urlFile(InternetOpenUrlA(
                session.get(),
                url.c_str(),
                NULL,
                0,
                INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS, // This allows http->https redirection
                0));
            THROW_LAST_ERROR_IF_NULL_MSG(urlFile, "InternetOpenUrl() failed.");

            // Check http return status
            DWORD requestStatus = 0;
            DWORD cbRequestStatus = sizeof(requestStatus);

            THROW_LAST_ERROR_IF_MSG(!HttpQueryInfoA(urlFile.get(),
                HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                &requestStatus,
                &cbRequestStatus,
                nullptr), "Query download request status failed.");

            if (requestStatus != HTTP_STATUS_OK)
            {
                AICLI_LOG(CLI, Error, << "Download request failed. Returned status: " << requestStatus);
                THROW_HR_MSG(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, requestStatus), "Download request status is not success.");
            }

            AICLI_LOG(CLI, Verbose, << "Download request status success.");

            // Get content length. Don't fail the download if failed.
            LONGLONG contentLength = 0;
            DWORD cbContentLength = sizeof(contentLength);

            HttpQueryInfoA(
                urlFile.get(),
                HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER64,
                &contentLength,
                &cbContentLength,
                nullptr);
            AICLI_LOG(CLI, Verbose, << "Download size: " << contentLength);

            std::ofstream outfile(dest, std::ofstream::binary);

            // Setup hash engine
            SHA256 hashEngine;
            std::string contentHash;

            const int bufferSize = 1024 * 1024; // 1MB
            auto buffer = std::make_unique<BYTE[]>(bufferSize);

            BOOL readSuccess = true;
            DWORD bytesRead = 0;
            LONGLONG progress = 0;

            if (callback)
            {
                callback->OnStarted(contentLength > 0);
            }

            do
            {
                if (m_cancelled)
                {
                    if (callback)
                    {
                        callback->OnCanceled();
                    }

                    return DownloaderResult::Canceled;
                }

                readSuccess = InternetReadFile(urlFile.get(), buffer.get(), bufferSize, &bytesRead);

                THROW_LAST_ERROR_IF_MSG(!readSuccess, "InternetReadFile() failed.");

                if (computeHash)
                {
                    hashEngine.Add(buffer.get(), bytesRead);
                }

                outfile.write((char*)buffer.get(), bytesRead);

                progress += bytesRead;

                if (callback && bytesRead != 0 && contentLength > 0)
                {
                    callback->OnProgress(progress, contentLength);
                }

            } while (bytesRead != 0);

            outfile.flush();

            if (computeHash)
            {
                m_downloadHash = hashEngine.Get();
                AICLI_LOG(CLI, Info, << "Download hash: " << SHA256::ConvertToString(m_downloadHash));
            }

            if (callback)
            {
                callback->OnCompleted();
            }

            AICLI_LOG(CLI, Info, << "Download completed.");

            return DownloaderResult::Success;
        }
        catch (const wil::ResultException& e)
        {
            AICLI_LOG(Fail, Error, << "Download failed. HResult: " << e.GetErrorCode() << " Reason: " << e.what());
            return DownloaderResult::Failed;
        }
        catch (const std::exception& e)
        {
            AICLI_LOG(Fail, Error, << "Download failed. Reason: " << e.what());
            return DownloaderResult::Failed;
        }
    }

    DownloaderResult Downloader::Cancel()
    {
        if (!m_downloadTask.valid())
        {
            THROW_HR_MSG(E_UNEXPECTED, "No active download found. Cancel failed.");
        }

        if (!m_cancelled)
        {
            m_cancelled = true;
        }

        return m_downloadTask.get();
    }

    DownloaderResult Downloader::Wait()
    {
        if (!m_downloadTask.valid())
        {
            THROW_HR_MSG(E_UNEXPECTED, "No active download found. Wait failed.");
        }

        return m_downloadTask.get();
    }

    std::vector<BYTE> Downloader::GetDownloadHash()
    {
        if (m_downloadHash.size() == 0)
        {
            THROW_HR_MSG(E_UNEXPECTED, "Invalid sha256 length. Download in progress or hash calculation not requested.");
        }

        return m_downloadHash;
    };
}