// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include <string>
namespace AppInstaller::Package::Manifest
{
    class InstallerSwitches
    {
    public:
        std::string Default;

        std::string Silent;

        std::string Verbose;

        void PopulateSwitchesFields(YAML::Node switchesNode);
    };
}