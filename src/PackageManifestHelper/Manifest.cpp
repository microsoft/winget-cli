// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "framework.h"
#include "Manifest.h"

namespace AppInstaller::Package::Manifest
{
    void Manifest::PopulateManifestFields(const YAML::Node& rootNode)
    {
        // Required fields
        this->Id = rootNode["Id"].as<std::string>();
        this->Name = rootNode["Name"].as<std::string>();
        this->Version = rootNode["Version"].as<std::string>();

        // Optional fields.
        this->ShortId = rootNode["ShortId"] ? rootNode["ShortId"].as<std::string>() : "";
        this->CompanyName = rootNode["CompanyName"] ? rootNode["CompanyName"].as<std::string>() : "";
        this->Authors = rootNode["Authors"] ? rootNode["Authors"].as<std::string>() : "";
        this->Channel = rootNode["Channel"] ? rootNode["Channel"].as<std::string>() : "";
        this->Author = rootNode["Author"] ? rootNode["Author"].as<std::string>() : "";
        this->License = rootNode["License"] ? rootNode["License"].as<std::string>() : "";
        this->MinOSVersion = rootNode["MinOSVersion"] ? rootNode["MinOSVersion"].as<std::string>() : "";
        this->Tags = rootNode["Tags"] ? rootNode["Tags"].as<std::string>() : "";
        this->Commands = rootNode["Commands"] ? rootNode["Commands"].as<std::string>() : "";
        this->Protocols = rootNode["Protocols"] ? rootNode["Protocols"].as<std::string>() : "";
        this->FileExtensions = rootNode["FileExtensions"] ? rootNode["FileExtensions"].as<std::string>() : "";
        this->InstallerType = rootNode["InstallerType"] ? rootNode["InstallerType"].as<std::string>() : "";
        this->Description = rootNode["Description"] ? rootNode["Description"].as<std::string>() : "";
        this->Homepage = rootNode["Homepage"] ? rootNode["Homepage"].as<std::string>() : "";
        this->LicenseUrl = rootNode["LicenseUrl"] ? rootNode["LicenseUrl"].as<std::string>() : "";

        YAML::Node installersNode = rootNode["Installers"];
        for (std::size_t i = 0; i < installersNode.size(); i++) {
            YAML::Node installerNode = installersNode[i];
            ManifestInstaller installer;
            installer.PopulateInstallerFields(installerNode);
            this->Installers.emplace_back(std::move(installer));
        }

        if (rootNode["Localization"])
        {
            YAML::Node localizationsNode = rootNode["Localization"];
            for (std::size_t i = 0; i < localizationsNode.size(); i++) {
                YAML::Node localizationNode = localizationsNode[i];
                ManifestLocalization localization;
                localization.PopulateLocalizationFields(localizationNode);
                this->Localization.emplace_back(std::move(localization));
            }
        }

        if (rootNode["Switches"])
        {
            YAML::Node switchesNode = rootNode["Switches"];
            InstallerSwitches switches;
            switches.PopulateSwitchesFields(switchesNode);
            this->Switches.emplace(std::move(switches));
        }
    }

    Manifest Manifest::CreatePackageManifest(const std::string& inputFile)
    {
        YAML::Node rootNode = YAML::LoadFile(inputFile);

        Manifest manifest;
        manifest.PopulateManifestFields(rootNode);

        return manifest;
    }

    Manifest Manifest::CreatePackageManifest(std::istream& inputStream)
    {
        YAML::Node rootNode = YAML::Load(inputStream);

        Manifest manifest;
        manifest.PopulateManifestFields(rootNode);

        return manifest;
    }


}

