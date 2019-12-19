// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "yaml-cpp\yaml.h"
#include "ManifestLocalization.h"

namespace AppInstaller::Package::Manifest
{
    void ManifestLocalization::PopulateLocalizationFields(YAML::Node localizationNode)
    {
        // Required
        this->Language = localizationNode["Language"].as<std::string>();

        // Optional
        this->Description = localizationNode["Description"] ? localizationNode["Description"].as<std::string>() : "";
        this->Homepage = localizationNode["Homepage"] ? localizationNode["Homepage"].as<std::string>() : "";
        this->LicenseUrl = localizationNode["LicenseUrl"] ? localizationNode["LicenseUrl"].as<std::string>() : "";
    }
}
