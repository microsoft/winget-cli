
#include <pch.h>
#include <Public/AppInstallerDownloader.h>
#include <Public/AppInstallerSHA256.h>
#include <Public/AppInstallerStrings.h>

namespace AppInstaller::Utility
{
    std::future<std::pair<unsigned int, std::string>> Downloader::DownloadAsync(
        const std::string& url,
        const std::filesystem::path& dest,
        bool computeHash,
        IDownloaderCallback* callback)
    {
        if (m_downloading)
        {
            throw std::runtime_error("Download already started");
        }

        m_downloading = true;

        return std::async(std::launch::async, &Downloader::DownloadInternal, this, url, dest, computeHash, callback);
    }

    std::pair<unsigned int, std::string> Downloader::DownloadInternal(
        const std::string& url,
        const std::filesystem::path& dest,
        bool computeHash,
        IDownloaderCallback* callback)
    {
        try
        {
            m_session = InternetOpenA(
                "appinstaller-cli",
                INTERNET_OPEN_TYPE_PRECONFIG,
                NULL,
                NULL,
                0);

            if (!m_session)
            {
                Cleanup();
                return std::make_pair(APPINSTALLER_DOWNLOAD_FAILED, "");
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
                Cleanup();
                return std::make_pair(APPINSTALLER_DOWNLOAD_FAILED, "");
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
                Cleanup();
                return std::make_pair(APPINSTALLER_DOWNLOAD_FAILED, "");
            }

            // Get content length. Don't fail the download if failed.
            LONGLONG contentLength;
            DWORD cbContentLength = sizeof(contentLength);

            if (HttpQueryInfo(
                m_urlFile,
                HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER64,
                &contentLength,
                &cbContentLength,
                nullptr))
            {
                m_downloadSize = contentLength;
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
                callback->OnStarted(m_downloadSize);
            }

            do
            {
                if (m_isCanceled)
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

                if (callback)
                {
                    callback->OnProgress(m_progress);
                }

            } while (readSuccess && bytesRead != 0);

            m_outfile.flush();

            if (computeHash)
            {
                downloadHash = hashEngine.GetAsString();
            }

            callback->OnCompleted();

            Cleanup();
            return std::make_pair(APPINSTALLER_DOWNLOAD_SUCCESS, downloadHash);
        }
        catch (const std::exception& e)
        {
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

        m_isCanceled = false;
        m_downloading = false;
        m_downloadSize = 0;
        m_progress = 0;
    }

    void Downloader::Cancel()
    {
        if (m_downloading)
        {
            m_isCanceled = true;
        }
    }
}