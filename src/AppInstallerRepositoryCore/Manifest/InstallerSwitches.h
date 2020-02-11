// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>

namespace YAML { class Node; }

namespace AppInstaller::Manifest
{
    class InstallerSwitches
    {
    public:
        std::string Custom;

        std::string Silent;

        std::string SilentWithProgress;

        std::string Interactive;

        std::string Language;

        std::string Log;

        std::string InstallLocation;

        // Populates InstallerSwitches
        // defaultSwitches: if an optional field is not found in the YAML node, the field will be populated with value from defaultSwitches.
        void PopulateSwitchesFields(const YAML::Node& switchesNode, const InstallerSwitches* defaultSwitches = nullptr);
    };
}