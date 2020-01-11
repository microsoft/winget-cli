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

        void PopulateLocalizationFields(const YAML::Node& localizationNode);
    };
}