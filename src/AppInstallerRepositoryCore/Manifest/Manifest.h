// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ManifestInstaller.h"
#include "ManifestLocalization.h"
#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>
#include <yaml-cpp/yaml.h>

#include <wil/result.h>

#include <filesystem>
#include <string>
#include <optional>
#include <vector>

namespace AppInstaller::Manifest
{
    struct ManifestException : public wil::ResultException
    {
        ManifestException() : wil::ResultException(APPINSTALLER_CLI_ERROR_MANIFEST_FAILED) {}
    };

    // Our representation of the parsed manifest file.
    struct Manifest
    {
        using string_t = Utility::NormalizedString;

        // Required
        string_t Id;

        // Required
        string_t Name;

        // Required
        string_t Version;

        string_t AppMoniker;

        string_t Publisher;

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

        std::vector<ManifestInstaller> Installers;

        std::vector<ManifestLocalization> Localization;

        ManifestInstaller::InstallerTypeEnum InstallerType;

        std::map<ManifestInstaller::InstallerSwitchType, string_t> Switches;

        string_t Description;

        string_t Homepage;

        string_t LicenseUrl;

        void PopulateManifestFields(const YAML::Node& rootNode);

        static Manifest CreateFromPath(const std::filesystem::path& inputFile);

        static Manifest Create(const std::string& input);
    };
}