// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "InstallerSwitches.h"
#include "Manifest.h"

namespace AppInstaller::Manifest
{
    void InstallerSwitches::PopulateSwitchesFields(const YAML::Node& switchesNode, const InstallerSwitches* defaultSwitches)
    {
        this->Default = switchesNode["Default"] ?
            switchesNode["Default"].as<std::string>() :
            defaultSwitches ? defaultSwitches->Default : "";

        this->Silent = switchesNode["Silent"] ?
            switchesNode["Silent"].as<std::string>() :
            defaultSwitches ? defaultSwitches->Silent : "";

        this->Verbose = switchesNode["Verbose"] ?
            switchesNode["Verbose"].as<std::string>() :
            defaultSwitches ? defaultSwitches->Verbose : "";
    }
}
