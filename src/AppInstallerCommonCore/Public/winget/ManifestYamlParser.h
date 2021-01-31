// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/ManifestValidation.h>
#include <winget/Manifest.h>

#include <filesystem>

namespace AppInstaller::Manifest::YamlParser
{
    struct YamlManifestInfo
    {
        YAML::Node Root;
        std::string FileName;
        ManifestTypeEnum ManifestType;
    };

    // fullValidation: Bool to set if manifest creation should perform extra validation that client does not need.
    //                 e.g. Channel should be null. Client code does not need this check to work properly.
    // throwOnWarning: Bool to indicate if an exception should be thrown with only warnings detected in the manifest.
    Manifest CreateFromPath(
        const std::filesystem::path& inputPath,
        bool fullValidation = false,
        bool throwOnWarning = false,
        bool isPartialManifest = false,
        PCWSTR resourceDll = nullptr,
        const std::filesystem::path& mergedManifestPath = {});

    Manifest Create(
        const std::string& input,
        bool fullValidation = false,
        bool throwOnWarning = false,
        bool isPartialManifest = false,
        PCWSTR resourceDll = nullptr,
        const std::filesystem::path& mergedManifestPath = {});

    Manifest CreateImpl(
        std::vector<YamlManifestInfo>& input,
        bool fullValidation,
        bool throwOnWarning,
        bool isPartialManifest,
        PCWSTR resourceDll,
        const std::filesystem::path& mergedManifestPath);
    

    
}