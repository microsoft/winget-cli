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
        std::string Default;

        std::string Silent;

        std::string Verbose;

        void PopulateSwitchesFields(const YAML::Node& switchesNode);
    };
}