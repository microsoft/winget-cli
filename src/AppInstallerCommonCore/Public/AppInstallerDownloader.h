// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerProgress.h>

#include <urlmon.h>

#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace AppInstaller::Utility
{
    // Downloads a file from the given URL and places it in the given location.
    //   url: The url to be downloaded from. http->https redirection is allowed.
    //   dest: The stream to be downloaded to.
    //   computeHash: Optional. Indicates if SHA256 hash should be calculated when downloading.
    std::optional<std::vector<BYTE>> DownloadToStream(
        const std::string& url,
        std::ostream& dest,
        IProgressCallback& progress,
        bool computeHash = false);

    // Downloads a file from the given URL and places it in the given location.
    //   url: The url to be downloaded from. http->https redirection is allowed.
    //   dest: The path to local file to be downloaded to.
    //   computeHash: Optional. Indicates if SHA256 hash should be calculated when downloading.
    std::optional<std::vector<BYTE>> Download(
        const std::string& url,
        const std::filesystem::path& dest,
        IProgressCallback& progress,
        bool computeHash = false);

    // Determines if the given url is a remote location.
    bool IsUrlRemote(std::string_view url);

    // Determines if the given url is secured.
    bool IsUrlSecure(std::string_view url);

    // Apply Mark of the web if the target file is on NTFS, otherwise does nothing.
    void ApplyMotwIfApplicable(const std::filesystem::path& filePath, URLZONE zone);

    // Apply Mark of the web using IAttachmentExecute::Save if the target file is on NTFS, otherwise does nothing.
    // This method only does a best effort since Attachment Execution Service may be disabled.
    // If IAttachmentExecute::Save is successfully invoked and the scan failed, the failure HRESULT is returned.
    HRESULT ApplyMotwUsingIAttachmentExecuteIfApplicable(const std::filesystem::path& filePath, const std::string& source);
}
