// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "InstallerSwitches.h"
#include "Manifest.h"

namespace AppInstaller::Manifest
{
    void InstallerSwitches::PopulateSwitchesFields(const YAML::Node& switchesNode, const InstallerSwitches* defaultSwitches)
    {
        this->Custom = switchesNode["Custom"] ?
            switchesNode["Custom"].as<std::string>() :
            defaultSwitches ? defaultSwitches->Custom : "";

        this->Silent = switchesNode["Silent"] ?
            switchesNode["Silent"].as<std::string>() :
            defaultSwitches ? defaultSwitches->Silent : "";

        this->SilentWithProgress = switchesNode["SilentWithProgress"] ?
            switchesNode["SilentWithProgress"].as<std::string>() :
            defaultSwitches ? defaultSwitches->SilentWithProgress : "";

        this->Interactive = switchesNode["Interactive"] ?
            switchesNode["Interactive"].as<std::string>() :
            defaultSwitches ? defaultSwitches->Interactive : "";

        this->Language = switchesNode["Language"] ?
            switchesNode["Language"].as<std::string>() :
            defaultSwitches ? defaultSwitches->Language : "";

        this->Log = switchesNode["Log"] ?
            switchesNode["Log"].as<std::string>() :
            defaultSwitches ? defaultSwitches->Log : "";

        this->InstallLocation = switchesNode["InstallLocation"] ?
            switchesNode["InstallLocation"].as<std::string>() :
            defaultSwitches ? defaultSwitches->InstallLocation : "";
    }
}
