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

        // The SHA256 hash of the stream
        Utility::SHA256::HashBuffer StreamSha256;

        ManifestTypeEnum ManifestType = ManifestTypeEnum::Preview;
    };

    enum class ManifestValidateOption : int
    {
        Default = 0,
        SchemaValidationOnly = 0x1,
        ErrorOnVerifiedPublisherFields = 0x2,

        // Options not exposed in winget util
        FullValidation = 0x10000000,
        ThrowOnWarning = 0x20000000,
    };

    DEFINE_ENUM_FLAG_OPERATORS(ManifestValidateOption);

    Manifest CreateFromPath(
        const std::filesystem::path& inputPath,
        ManifestValidateOption validateOption = ManifestValidateOption::Default,
        const std::filesystem::path& mergedManifestPath = {});

    Manifest Create(
        const std::string& input,
        ManifestValidateOption validateOption = ManifestValidateOption::Default,
        const std::filesystem::path& mergedManifestPath = {});

    Manifest ParseManifest(
        std::vector<YamlManifestInfo>& input,
        ManifestValidateOption validateOption = ManifestValidateOption::Default,
        const std::filesystem::path& mergedManifestPath = {});
}