// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include <string>
#include <vector>
#include <optional>
#include "ManifestInstaller.h"
#include "ManifestLocalization.h"

namespace AppInstaller::Manifest
{
    class Manifest
    {
    public:
        // Required
        std::string Id;

        // Required
        std::string Name;

        // Required
        std::string Version;

        // Name subject to change
        std::string ShortId;

        std::string CompanyName;

        // Comma separated Values
        std::string Authors;

        std::string Channel;

        std::string Author;

        std::string License;

        std::string MinOSVersion;

        // Comma separated values
        std::string Tags;

        // Comma separated values
        std::string Commands;

        // Comma separated values
        std::string Protocols;

        // Comma separated values
        std::string FileExtensions;

        std::vector<ManifestInstaller> Installers;

        std::vector<ManifestLocalization> Localization;

        std::string InstallerType;

        std::optional<InstallerSwitches> Switches;

        std::string Description;

        std::string Homepage;

        std::string LicenseUrl;

        void PopulateManifestFields(const YAML::Node& rootNode);

        static Manifest CreatePackageManifestFromFile(const std::string& inputFile);

        static Manifest CreatePackageManifest(const std::string& input);
    };
}