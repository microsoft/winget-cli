// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "AppInstallerCLICore.h"
#include "ManifestInstaller.h"
#include "ManifestLocalization.h"

#include <filesystem>
#include <string>
#include <optional>
#include <vector>

namespace AppInstaller::Manifest
{
    struct ManifestException : public wil::ResultException
    {
        ManifestException() : wil::ResultException(CLICORE_ERROR_INSTALLFLOW_FAILED) {}
    };

    // Our representation of the parsed manifest file.
    struct Manifest
    {
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

        static Manifest CreateFromPath(const std::filesystem::path& inputFile);

        static Manifest Create(const std::string& input);
    };
}