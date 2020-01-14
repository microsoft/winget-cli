// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <pch.h>
#include <Public/AppInstallerDownloader.h>
#include <Public/AppInstallerSHA256.h>
#include <Public/AppInstallerStrings.h>
#include <Public/Exceptions.h>
#include <Public/AppInstallerLogging.h>

namespace AppInstaller::Utility
{
    void Downloader::StartDownloadAsync(
        const std::string& url,
        const std::filesystem::path& dest,
        bool computeHash,
        IDownloaderCallback* callback)
    {
        if (m_downloading)
        {
            throw std::runtime_error("Download in progress.");
        }

        if (m_cancelled)
        {
            throw std::runtime_error("Download cancelation in progress.");
        }

        m_downloading = true;

        m_downloadTask = std::async(std::launch::async, &Downloader::DownloadInternal, this, url, dest, computeHash, callback);
    }

    std::pair<unsigned int, std::string> Downloader::DownloadInternal(
        const std::string& url,
        const std::filesystem::path& dest,
        bool computeHash,
        IDownloaderCallback* callback)
    {
        try
        {
            AICLI_LOG(CLI, Info, << "Downloading url: " << url << " , dest: " << dest);
            m_session = InternetOpenA(
                "appinstaller-cli",
                INTERNET_OPEN_TYPE_PRECONFIG,
                NULL,
                NULL,
                0);

            if (!m_session)
            {
                throw HResultException("InternetOpen() failed.", HRESULT_FROM_WIN32(GetLastError()));
            }

            m_urlFile = InternetOpenUrlA(
                m_session,
                url.c_str(),
                NULL,
                0,
                INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS,
                0);

            if (!m_urlFile)
            {
                throw HResultException("InternetOpenUrl() failed.", HRESULT_FROM_WIN32(GetLastError()));
            }

            // Check http return status
            DWORD requestStatus = 0;
            DWORD cbRequestStatus = sizeof(requestStatus);

            if (!HttpQueryInfoA(m_urlFile,
                HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                &requestStatus,
                &cbRequestStatus,
                nullptr) || requestStatus != HTTP_STATUS_OK)
            {
                throw HResultException("Query download request status failed.", HRESULT_FROM_WIN32(GetLastError()));
            }

            AICLI_LOG(CLI, Verbose, << "Download request status success.");

            // Get content length. Don't fail the download if failed.
            LONGLONG contentLength = 0;
            DWORD cbContentLength = sizeof(contentLength);

            if (HttpQueryInfoA(
                m_urlFile,
                HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER64,
                &contentLength,
                &cbContentLength,
                nullptr))
            {
                m_downloadSize = contentLength;
                AICLI_LOG(CLI, Verbose, << "Download size: " << contentLength);
            }

            m_outfile.open(dest, std::ofstream::binary);

            // Setup hash engine
            SHA256 hashEngine;
            std::string downloadHash;

            const int bufferSize = 1024 * 1024; // 1MB
            m_buffer = new BYTE[bufferSize];

            BOOL readSuccess = true;
            DWORD bytesRead = 0;

            if (callback)
            {
                callback->OnStarted();
            }

            do
            {
                if (m_cancelled)
                {
                    if (callback)
                    {
                        callback->OnCanceled();
                    }

                    Cleanup();
                    return std::make_pair(APPINSTALLER_DOWNLOAD_CANCELED, "");
                }

                readSuccess = InternetReadFile(m_urlFile, m_buffer, bufferSize, &bytesRead);

                if (!readSuccess)
                {
                    Cleanup();
                    return std::make_pair(APPINSTALLER_DOWNLOAD_FAILED, "");
                }

                if (computeHash)
                {
                    hashEngine.Add(m_buffer, bytesRead);
                }

                m_outfile.write((char*)m_buffer, bytesRead);

                m_progress += bytesRead;

                if (callback && bytesRead != 0)
                {
                    callback->OnProgress(m_progress, m_downloadSize);
                }

            } while (readSuccess && bytesRead != 0);

            m_outfile.flush();

            if (computeHash)
            {
                downloadHash = hashEngine.GetAsString();
            }

            callback->OnCompleted();

            Cleanup();
            AICLI_LOG(CLI, Info, << "Download completed.");
            if (computeHash)
            {
                AICLI_LOG(CLI, Info, << "Download hash: " << downloadHash);
            }
            return std::make_pair(APPINSTALLER_DOWNLOAD_SUCCESS, downloadHash);
        }
        catch (const HResultException& e)
        {
            AICLI_LOG(Fail, Error, << e.Message() << " hresult: " << e.Code());
            Cleanup();
            return std::make_pair(APPINSTALLER_DOWNLOAD_FAILED, "");
        }
        catch (const NtStatusException& e)
        {
            AICLI_LOG(Fail, Error, << e.Message() << " NtStatus: " << e.Code());
            Cleanup();
            return std::make_pair(APPINSTALLER_DOWNLOAD_FAILED, "");
        }
        catch (const std::exception& e)
        {
            AICLI_LOG(Fail, Error, << e.what());
            Cleanup();
            return std::make_pair(APPINSTALLER_DOWNLOAD_FAILED, "");
        }
    }

    void Downloader::Cleanup()
    {
        if (m_session)
        {
            InternetCloseHandle(m_session);
        }

        if (m_urlFile)
        {
            InternetCloseHandle(m_urlFile);
        }

        if (m_buffer)
        {
            delete m_buffer;
        }

        if (m_outfile.is_open())
        {
            m_outfile.close();
        }

        m_cancelled = false;
        m_downloading = false;
        m_downloadSize = 0;
        m_progress = 0;
    }

    void Downloader::Cancel()
    {
        if (m_downloadTask.valid() && m_downloading && !m_cancelled)
        {
            m_cancelled = true;
            m_downloadTask.get();
        }
    }

    std::pair<unsigned int, std::string> Downloader::Wait()
    {
        if (!m_downloadTask.valid() || !m_downloading)
        {
            throw std::runtime_error("No active download found");
        }

        return m_downloadTask.get();
    }
}