// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ManifestLocalization.h"

namespace AppInstaller::Manifest
{
    std::vector<ValidationError> ManifestLocalization::PopulateLocalizationFields(const YAML::Node& localizationNode, const ManifestLocalization& defaultLocalization)
    {
        // Populates default values first
        this->Description = defaultLocalization.Description;
        this->Homepage = defaultLocalization.Homepage;
        this->LicenseUrl = defaultLocalization.LicenseUrl;

        const std::vector<ManifestFieldInfo> FieldInfos =
        {
            { "Language", [this](const YAML::Node& value) { Language = value.as<std::string>(); }, true },
            { "Description", [this](const YAML::Node& value) { Description = value.as<std::string>(); } },
            { "Homepage", [this](const YAML::Node& value) { Homepage = value.as<std::string>(); } },
            { "LicenseUrl", [this](const YAML::Node& value) { LicenseUrl = value.as<std::string>(); } },
        };

        return ValidateAndProcessFields(localizationNode, FieldInfos);
    }
}
