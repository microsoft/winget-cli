// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>

namespace YAML { class Node; }

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

        // Populates ManifestLocalization
        // defaultLocalization: if an optional field is not found in the YAML node, the field will be populated with value from defaultLocalization.
        void PopulateLocalizationFields(const YAML::Node& localizationNode, const ManifestLocalization& defaultLocalization);
    };
}