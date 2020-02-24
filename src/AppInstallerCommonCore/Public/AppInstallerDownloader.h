// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerFuture.h>

namespace AppInstaller::Utility
{
    // Downloads a file from the given URL and places it in the given location.
    //   url: The url to be downloaded from. http->https redirection is allowed.
    //   dest: The path to local file to be downloaded to.
    //   computeHash: Optional. Indicates if SHA256 hash should be calculated when downloading.
    Future<std::vector<BYTE>> DownloadAsync(
        const std::string& url,
        const std::filesystem::path& dest,
        bool computeHash = false);
}