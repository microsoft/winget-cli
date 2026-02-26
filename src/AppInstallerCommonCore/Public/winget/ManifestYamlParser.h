// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/ManifestValidation.h>
#include <winget/Manifest.h>
#include <winget/Yaml.h>
#include <AppInstallerSHA256.h>
#include <filesystem>

namespace AppInstaller::Manifest::YamlParser
{
    struct YamlManifestInfo
    {
        // Root node of a yaml manifest file
        YAML::Node Root;

        // File name of the manifest file if applicable for error reporting
        std::string FileName;

        // Schema header string found in the manifest file
        YAML::DocumentSchemaHeader DocumentSchemaHeader;

        // The SHA256 hash of the stream
        Utility::SHA256::HashBuffer StreamSha256;

        ManifestTypeEnum ManifestType = ManifestTypeEnum::Preview;
    };

    Manifest CreateFromPath(
        const std::filesystem::path& inputPath,
        ManifestValidateOption validateOption = {},
        const std::filesystem::path& mergedManifestPath = {});

    Manifest Create(
        const std::string& input,
        ManifestValidateOption validateOption = {},
        const std::filesystem::path& mergedManifestPath = {});

    Manifest ParseManifest(
        std::vector<YamlManifestInfo>& input,
        ManifestValidateOption validateOption = {},
        const std::filesystem::path& mergedManifestPath = {});
}
