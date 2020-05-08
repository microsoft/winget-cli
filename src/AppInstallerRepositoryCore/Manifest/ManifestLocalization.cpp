// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ManifestLocalization.h"

namespace AppInstaller::Manifest
{
    std::vector<ValidationError> ManifestLocalization::PopulateLocalizationFields(
        const YAML::Node& localizationNode,
        const ManifestLocalization& defaultLocalization,
        bool fullValidation,
        ManifestVer manifestVersion)
    {
        // Populates default values first
        this->Description = defaultLocalization.Description;
        this->Homepage = defaultLocalization.Homepage;
        this->LicenseUrl = defaultLocalization.LicenseUrl;

        std::vector<ManifestFieldInfo> fieldInfos;

        if (manifestVersion >= PreviewManifestVersion)
        {
            std::vector<ManifestFieldInfo> previewFieldInfos =
            {
                { "Language", [this](const YAML::Node& value) { Language = value.as<std::string>(); }, true },
                { "Description", [this](const YAML::Node& value) { Description = value.as<std::string>(); } },
                { "Homepage", [this](const YAML::Node& value) { Homepage = value.as<std::string>(); } },
                { "LicenseUrl", [this](const YAML::Node& value) { LicenseUrl = value.as<std::string>(); } },
            };

            std::move(previewFieldInfos.begin(), previewFieldInfos.end(), std::inserter(fieldInfos, fieldInfos.end()));
        }

        return ValidateAndProcessFields(localizationNode, fieldInfos, fullValidation);
    }
}
