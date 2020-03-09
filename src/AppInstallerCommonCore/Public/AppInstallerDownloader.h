// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerProgress.h>

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

    // Apply Mark of the web if the target file is on NTFS, otherwise does nothing.
    void ApplyMotwIfApplicable(const std::filesystem::path& filePath);
}
