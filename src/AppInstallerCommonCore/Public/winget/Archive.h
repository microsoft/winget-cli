// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>

namespace AppInstaller::Archive
{
    enum class ExtractionMethod
    {
        // Default archive extraction method is ShellApi.
        ShellApi,
        Tar,
    };

    HRESULT TryExtractArchive(const std::filesystem::path& archivePath, const std::filesystem::path& destPath);

    bool ScanZipFile(const std::filesystem::path& zipPath);
}
