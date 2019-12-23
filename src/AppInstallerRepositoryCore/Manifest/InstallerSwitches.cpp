// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "InstallerSwitches.h"

namespace AppInstaller::Manifest
{
    void InstallerSwitches::PopulateSwitchesFields(YAML::Node switchesNode)
    {
        this->Default = switchesNode["Default"] ? switchesNode["Default"].as<std::string>() : "";
        this->Silent = switchesNode["Silent"] ? switchesNode["Silent"].as<std::string>() : "";
        this->Verbose = switchesNode["Verbose"] ? switchesNode["Verbose"].as<std::string>() : "";
    }
}
