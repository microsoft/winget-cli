// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include <string>

namespace AppInstaller::Manifest
{
    class ManifestLocalization
    {
    public:
        // Required
        std::string Language;

        std::string Description;

        std::string Homepage;

        std::string LicenseUrl;

        void PopulateLocalizationFields(YAML::Node localizationNode);
    };
}