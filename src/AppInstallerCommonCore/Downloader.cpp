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
    std::optional<std::vector<BYTE>> DownloadToStream(
        const std::string& url,
        std::ostream& dest,
        IProgressCallback& progress,
        bool computeHash)
    {
        THROW_HR_IF(E_INVALIDARG, url.empty());

        AICLI_LOG(Core, Info, << "Downloading from url: " << url);

        wil::unique_hinternet session(InternetOpenA(
            "winget-cli",
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
            AICLI_LOG(Core, Error, << "Download request failed. Returned status: " << requestStatus);
            THROW_HR_MSG(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, requestStatus), "Download request status is not success.");
        }

        AICLI_LOG(Core, Verbose, << "Download request status success.");

        // Get content length. Don't fail the download if failed.
        LONGLONG contentLength = 0;
        DWORD cbContentLength = sizeof(contentLength);

        HttpQueryInfoA(
            urlFile.get(),
            HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER64,
            &contentLength,
            &cbContentLength,
            nullptr);
        AICLI_LOG(Core, Verbose, << "Download size: " << contentLength);

        // Setup hash engine
        SHA256 hashEngine;
        std::string contentHash;

        const int bufferSize = 1024 * 1024; // 1MB
        auto buffer = std::make_unique<BYTE[]>(bufferSize);

        BOOL readSuccess = true;
        DWORD bytesRead = 0;
        LONGLONG bytesDownloaded = 0;

        do
        {
            if (progress.IsCancelled())
            {
                AICLI_LOG(Core, Info, << "Download cancelled.");
                return {};
            }

            readSuccess = InternetReadFile(urlFile.get(), buffer.get(), bufferSize, &bytesRead);

            THROW_LAST_ERROR_IF_MSG(!readSuccess, "InternetReadFile() failed.");

            if (computeHash)
            {
                hashEngine.Add(buffer.get(), bytesRead);
            }

            dest.write((char*)buffer.get(), bytesRead);

            bytesDownloaded += bytesRead;

            if (bytesRead != 0)
            {
                progress.OnProgress(bytesDownloaded, contentLength, ProgressType::Bytes);
            }

        } while (bytesRead != 0);

        dest.flush();

        std::vector<BYTE> result;
        if (computeHash)
        {
            result = hashEngine.Get();
            AICLI_LOG(Core, Info, << "Download hash: " << SHA256::ConvertToString(result));
        }

        AICLI_LOG(Core, Info, << "Download completed.");

        return result;
    }

    std::optional<std::vector<BYTE>> Download(
        const std::string& url,
        const std::filesystem::path& dest,
        IProgressCallback& progress,
        bool computeHash)
    {
        THROW_HR_IF(E_INVALIDARG, url.empty());
        THROW_HR_IF(E_INVALIDARG, dest.empty());

        AICLI_LOG(Core, Info, << "Downloading to path: " << dest);

        std::filesystem::create_directories(dest.parent_path());

        std::ofstream emptyDestFile(dest);
        emptyDestFile.close();
        ApplyMotwIfApplicable(dest);

        // Use std::ofstream::app to append to previous empty file so that it will not
        // create a new file and clear motw.
        std::ofstream outfile(dest, std::ofstream::binary | std::ofstream::app);
        return DownloadToStream(url, outfile, progress, computeHash);
    }

    bool IsUrlRemote(std::string_view url)
    {
        using namespace std::string_view_literals;
        constexpr std::string_view s_http_start = "http://"sv;
        constexpr std::string_view s_https_start = "https://"sv;

        // Very simple choice right now: "does it start with http:// or https://"?
        if (CaseInsensitiveEquals(url.substr(0, s_http_start.length()), s_http_start) ||
            CaseInsensitiveEquals(url.substr(0, s_https_start.length()), s_https_start))
        {
            return true;
        }

        return false;
    }

    void ApplyMotwIfApplicable(const std::filesystem::path& filePath)
    {
        AICLI_LOG(Core, Info, << "Started applying motw to " << filePath);

        {
            // Check the file system the input file is on.
            wil::unique_hfile fileHandle{ CreateFileW(
                filePath.c_str(), /*lpFileName*/
                GENERIC_READ, /*dwDesiredAccess*/
                0, /*dwShareMode*/
                NULL, /*lpSecurityAttributes*/
                OPEN_EXISTING, /*dwCreationDisposition*/
                FILE_ATTRIBUTE_NORMAL, /*dwFlagsAndAttributes*/
                NULL /*hTemplateFile*/) };

            THROW_LAST_ERROR_IF(fileHandle.get() == INVALID_HANDLE_VALUE);

            wchar_t fileSystemName[MAX_PATH];
            THROW_LAST_ERROR_IF(!GetVolumeInformationByHandleW(
                fileHandle.get(), /*hFile*/
                NULL, /*lpVolumeNameBuffer*/
                0, /*nVolumeNameSize*/
                NULL, /*lpVolumeSerialNumber*/
                NULL, /*lpMaximumComponentLength*/
                NULL, /*lpFileSystemFlags*/
                fileSystemName, /*lpFileSystemNameBuffer*/
                MAX_PATH /*nFileSystemNameSize*/));

            if (_wcsicmp(fileSystemName, L"NTFS") != 0)
            {
                AICLI_LOG(Core, Info, << "File system is not NTFS. Skipped applying motw");
                return;
            }
        }

        // Zone Identifier stream name
        // https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/6e3f7352-d11c-4d76-8c39-2516a9df36e8
        std::filesystem::path motwPath(filePath);
        motwPath += L":Zone.Identifier:$DATA";

        // Apply mark of the web. ZoneId 3 means downloaded from internet.
        std::ofstream motwStream(motwPath);
        motwStream << "[ZoneTransfer]" << std::endl;
        motwStream << "ZoneId=3" << std::endl;

        AICLI_LOG(Core, Info, << "Finished applying motw");
    }
}
