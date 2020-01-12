// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <optional>
#include "InstallerSwitches.h"
#include "..\AppInstallerCommonCore\Public\AppInstallerArchitecture.h"

using namespace AppInstaller::Utility;

namespace AppInstaller::Manifest
{
    class ManifestInstaller
    {
    public:
        // Required. Values: x86, x64, arm, arm64, all.
        Architecture Arch;

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

        void PopulateInstallerFields(const YAML::Node& installerNode);
    };
}