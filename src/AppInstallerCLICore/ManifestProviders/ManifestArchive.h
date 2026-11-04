// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Manifest.h>
#include <filesystem>

namespace AppInstaller::CLI::Workflow
{
    Manifest::Manifest LoadManifestArchive(const std::filesystem::path& archivePath);
}
