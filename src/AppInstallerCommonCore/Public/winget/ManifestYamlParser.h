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

        ManifestTypeEnum ManifestType;
    };

    // fullValidation: Bool to set if manifest creation should perform extra validation that client does not need.
    //                 e.g. Channel should be null. Client code does not need this check to work properly.
    // throwOnWarning: Bool to indicate if an exception should be thrown with only warnings detected in the manifest.
    // resourceDll:    Binary where schemas are embedded, or nullptr if they are embedded in the binary that created the process
    // mergedManifestPath: Output file for merged manifest after processing a multi file manifest
    // isPartialManifest:  Bool to indicate if the input only consists of partial of a multi file manifest. In this
    //                     case, only schema validation will be performed..
    Manifest CreateFromPath(
        const std::filesystem::path& inputPath,
        bool fullValidation = false,
        bool throwOnWarning = false,
        PCWSTR resourceDll = nullptr,
        const std::filesystem::path& mergedManifestPath = {},
        bool isPartialManifest = false);

    Manifest Create(
        const std::string& input,
        bool fullValidation = false,
        bool throwOnWarning = false,
        PCWSTR resourceDll = nullptr,
        const std::filesystem::path& mergedManifestPath = {},
        bool isPartialManifest = false);

    Manifest ParseManifest(
        std::vector<YamlManifestInfo>& input,
        bool fullValidation = false,
        bool throwOnWarning = false,
        PCWSTR resourceDll = nullptr,
        const std::filesystem::path& mergedManifestPath = {},
        bool isPartialManifest = false);
}