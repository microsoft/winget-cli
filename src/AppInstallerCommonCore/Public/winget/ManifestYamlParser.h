// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/ManifestValidation.h>
#include <winget/Manifest.h>
#include <winget/Yaml.h>

#include <filesystem>

namespace AppInstaller::Manifest::YamlParser
{
    struct YamlManifestInfo
    {
        // Root node of a yaml manifest file
        YAML::Node Root;

        // File name of the manifest file if applicable for error reporting
        std::string FileName;

        ManifestTypeEnum ManifestType = ManifestTypeEnum::Preview;
    };

    // fullValidation: Bool to set if manifest creation should perform extra validation that client does not need.
    //                 e.g. Channel should be null. Client code does not need this check to work properly.
    // throwOnWarning: Bool to indicate if an exception should be thrown with only warnings detected in the manifest.
    // mergedManifestPath: Output file for merged manifest after processing a multi file manifest
    // schemaValidationOnly: Bool to indicate if only schema validation should be performed
    Manifest CreateFromPath(
        const std::filesystem::path& inputPath,
        bool fullValidation = false,
        bool throwOnWarning = false,
        const std::filesystem::path& mergedManifestPath = {},
        bool schemaValidationOnly = false);

    Manifest Create(
        const std::string& input,
        bool fullValidation = false,
        bool throwOnWarning = false,
        const std::filesystem::path& mergedManifestPath = {},
        bool schemaValidationOnly = false);

    Manifest ParseManifest(
        std::vector<YamlManifestInfo>& input,
        bool fullValidation = false,
        bool throwOnWarning = false,
        const std::filesystem::path& mergedManifestPath = {},
        bool schemaValidationOnly = false);
}