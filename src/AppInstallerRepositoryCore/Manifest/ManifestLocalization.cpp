// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ManifestLocalization.h"

namespace AppInstaller::Manifest
{
    void ManifestLocalization::PopulateLocalizationFields(const YAML::Node& localizationNode, const ManifestLocalization& defaultLocalization)
    {
        // Required
        this->Language = localizationNode["Language"].as<std::string>();

        // Optional
        this->Description = localizationNode["Description"] ?
            string_t(localizationNode["Description"].as<std::string>()) :
            defaultLocalization.Description;

        this->Homepage = localizationNode["Homepage"] ?
            string_t(localizationNode["Homepage"].as<std::string>()) :
            defaultLocalization.Homepage;

        this->LicenseUrl = localizationNode["LicenseUrl"] ?
            string_t(localizationNode["LicenseUrl"].as<std::string>()) :
            defaultLocalization.LicenseUrl;
    }
}
