// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ManifestInstaller.h"

namespace AppInstaller::Manifest
{
    void ManifestInstaller::PopulateInstallerFields(const YAML::Node& installerNode)
    {
        // Required fields
        this->Arch = installerNode["Arch"].as<std::string>();
        this->Url = installerNode["Url"].as<std::string>();
        this->Sha256 = installerNode["Sha256"].as<std::string>();

        // Optional fields.
        this->Language = installerNode["Language"] ? installerNode["Language"].as<std::string>() : "";
        this->Scope = installerNode["Scope"] ? installerNode["Scope"].as<std::string>() : "";
        this->InstallerType = installerNode["InstallerType"] ? installerNode["InstallerType"].as<std::string>() : "";

        if (installerNode["Switches"])
        {
            YAML::Node switchesNode = installerNode["Switches"];
            InstallerSwitches switches;
            switches.PopulateSwitchesFields(switchesNode);
            this->Switches.emplace(std::move(switches));
        }
    }
}
