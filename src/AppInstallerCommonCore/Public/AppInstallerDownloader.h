// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerErrors.h>
#include <AppInstallerProgress.h>

#include <chrono>
#include <filesystem>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

using namespace std::chrono_literals;

namespace AppInstaller::Utility
{
    // The type of data being downloaded; determines what code should
    // be used when downloading.
    enum class DownloadType
    {
        Index,
        Manifest,
        WinGetUtil,
        Installer,
        InstallerMetadataCollectionInput,
        ConfigurationFile,
    };

    struct DownloadRequestHeader
    {
        std::string Name;
        std::string Value;
        bool IsAuth = false;
    };

    // Extra metadata about a download for use by certain downloaders (Delivery Optimization for instance).
    // Extra download request headers.
    struct DownloadInfo
    {
        std::string DisplayName;
        std::string ContentId;
        std::vector<DownloadRequestHeader> RequestHeaders;
    };

    // Properties about the downloaded file.
    struct DownloadResult
    {
        std::vector<BYTE> Sha256Hash;
        uint64_t SizeInBytes = 0;
        std::optional<std::string> ContentType;
    };

    // An exception that indicates that a remote service is too busy/unavailable and may contain data on when to try again.
    struct ServiceUnavailableException : public wil::ResultException
    {
        ServiceUnavailableException(std::chrono::seconds retryAfter = 0s) : wil::ResultException(APPINSTALLER_CLI_ERROR_SERVICE_UNAVAILABLE), m_retryAfter(retryAfter) {}

        std::chrono::seconds RetryAfter() const { return m_retryAfter; }

    private:
        std::chrono::seconds m_retryAfter;
    };

    // Downloads a file from the given URL and places it in the given location.
    //   url: The url to be downloaded from. http->https redirection is allowed.
    //   dest: The stream to be downloaded to.
    //   computeHash: Optional. Indicates if SHA256 hash should be calculated when downloading.
    //   downloadInfo: Optional. Currently only used by DO to identify the download.
    DownloadResult DownloadToStream(
        const std::string& url,
        std::ostream& dest,
        DownloadType type,
        IProgressCallback& progress,
        std::optional<DownloadInfo> downloadInfo = {});

    // Downloads a file from the given URL and places it in the given location.
    //   url: The url to be downloaded from. http->https redirection is allowed.
    //   dest: The path to local file to be downloaded to.
    //   computeHash: Optional. Indicates if SHA256 hash should be calculated when downloading.
    //   downloadInfo: Optional. Currently only used by DO to identify the download.
    DownloadResult Download(
        const std::string& url,
        const std::filesystem::path& dest,
        DownloadType type,
        IProgressCallback& progress,
        std::optional<DownloadInfo> downloadInfo = {});

    // Gets the headers for the given URL.
    std::map<std::string, std::string> GetHeaders(std::string_view url);

    // Determines if the given url is a remote location.
    bool IsUrlRemote(std::string_view url);

    // Determines if the given url is secured.
    bool IsUrlSecure(std::string_view url);

    // Apply Mark of the web if the target file is on NTFS, otherwise does nothing.
    void ApplyMotwIfApplicable(const std::filesystem::path& filePath, URLZONE zone);

    // Remove Mark of the web if the target file is on NTFS, otherwise does nothing.
    void RemoveMotwIfApplicable(const std::filesystem::path& filePath);

    // Apply Mark of the web using IAttachmentExecute::Save if the target file is on NTFS, otherwise does nothing.
    // This method only does a best effort since Attachment Execution Service may be disabled.
    // If IAttachmentExecute::Save is successfully invoked and the scan failed, the failure HRESULT is returned.
    // zoneIfScanFailure: URLZONE to apply if IAttachmentExecute::Save scan failed.
    HRESULT ApplyMotwUsingIAttachmentExecuteIfApplicable(const std::filesystem::path& filePath, const std::string& source, URLZONE zoneIfScanFailure);

    // Function to read-only create a stream from a uri string (url address or file system path)
    ::Microsoft::WRL::ComPtr<IStream> GetReadOnlyStreamFromURI(std::string_view uriStr);

    // Gets the retry after value in terms of a delay in seconds.
    std::chrono::seconds GetRetryAfter(const std::wstring& retryAfter);

    // Gets the retry after value in terms of a delay in seconds.
    std::chrono::seconds GetRetryAfter(const winrt::Windows::Web::Http::HttpResponseMessage& response);
}
