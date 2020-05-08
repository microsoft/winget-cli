// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ManifestInstaller.h"
#include "ManifestLocalization.h"
#include "ManifestValidation.h"
#include <AppInstallerStrings.h>

#include <filesystem>
#include <string>
#include <vector>

namespace AppInstaller::Manifest
{
    // Representation of the parsed manifest file.
    struct Manifest
    {
        using string_t = Utility::NormalizedString;

        // Required
        string_t Id;

        // Required
        string_t Name;

        // Required
        string_t Version;

        // Required
        string_t Publisher;

        string_t AppMoniker;

        string_t Channel;

        string_t Author;

        string_t License;

        string_t MinOSVersion;

        // Comma separated values
        std::vector<string_t> Tags;

        // Comma separated values
        std::vector<string_t> Commands;

        // Comma separated values
        std::vector<string_t> Protocols;

        // Comma separated values
        std::vector<string_t> FileExtensions;

        ManifestInstaller::InstallerTypeEnum InstallerType = ManifestInstaller::InstallerTypeEnum::Unknown;

        string_t Description;

        string_t Homepage;

        string_t LicenseUrl;

        ManifestVer ManifestVersion;

        std::map<ManifestInstaller::InstallerSwitchType, string_t> Switches;

        std::vector<ManifestInstaller> Installers;

        std::vector<ManifestLocalization> Localization;

        std::vector<ValidationError> PopulateManifestFields(const YAML::Node& rootNode, bool fullValidation);

        // fullValidation: Bool to set if manifest creation should perform extra validation that client does not need.
        //                 e.g. Channel should be null. Client code does not need this check to work properly.
        // throwOnWarning: Bool to indicate if an exception should be thrown with only warnings detected in the manifest.
        static Manifest CreateFromPath(const std::filesystem::path& inputFile, bool fullValidation = false, bool throwOnWarning = false);

        static Manifest Create(const std::string& input, bool fullValidation = false, bool throwOnWarning = false);
    };
}