// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include <string>
#include <optional>
#include "InstallerSwitches.h"

namespace AppInstaller::Package::Manifest
{
    class ManifestInstaller
    {
    public:
        // Required. Values: x86, x64, arm, arm64, all.
        std::string Arch;

        // Required
        std::string Url;

        // Required
        std::string Sha256;

        // Empty means default
        std::string Language;

        // Name TBD
        std::string Scope;

        // If present, has more presedence than root
        std::string InstallerType;

        // If present, has more presedence than root
        std::optional<InstallerSwitches> Switches;

        void PopulateInstallerFields(YAML::Node installerNode);
    };
}