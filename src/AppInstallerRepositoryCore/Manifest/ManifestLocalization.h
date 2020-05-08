// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>

#include "ManifestValidation.h"

namespace AppInstaller::Manifest
{
    class ManifestLocalization
    {
    public:
        using string_t = Utility::NormalizedString;

        // Required
        string_t Language;

        string_t Description;

        string_t Homepage;

        string_t LicenseUrl;

        // Populates ManifestLocalization
        // defaultLocalization: if an optional field is not found in the YAML node, the field will be populated with value from defaultLocalization.
        std::vector<ValidationError> PopulateLocalizationFields(
            const YAML::Node& localizationNode,
            const ManifestLocalization& defaultLocalization,
            bool fullValidation,
            ManifestVer manifestVersion);
    };
}