// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <optional>
#include <AppInstallerArchitecture.h>
#include "InstallerSwitches.h"

namespace AppInstaller::Manifest
{
    class ManifestInstaller
    {
    public:
        // Required. Values: x86, x64, arm, arm64, all.
        AppInstaller::Utility::Architecture Arch;

        // Required
        std::string Url;

        // Required
        std::vector<BYTE> Sha256;

        // Empty means default
        std::string Language;

        // Name TBD
        std::string Scope;

        // If present, has more presedence than root
        std::string InstallerType;

        // If present, has more presedence than root
        std::optional<InstallerSwitches> Switches;

        // Populates ManifestInstaller
        // defaultInstaller: if an optional field is not found in the YAML node, the field will be populated with value from defaultInstaller.
        void PopulateInstallerFields(const YAML::Node& installerNode, const ManifestInstaller& defaultInstaller);
    };
}