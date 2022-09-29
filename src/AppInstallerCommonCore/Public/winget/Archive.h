// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>

namespace AppInstaller::Archive
{
    HRESULT TryExtractArchive(const std::filesystem::path& archivePath, const std::filesystem::path& destPath);

    bool ScanZipFile(const std::filesystem::path& zipPath);
}