// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Manifest.h"

namespace AppInstaller::Manifest
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

        if (rootNode["Switches"])
        {
            YAML::Node switchesNode = rootNode["Switches"];
            InstallerSwitches switches;
            switches.PopulateSwitchesFields(switchesNode);
            this->Switches.emplace(std::move(switches));
        }

        // Create default ManifestInstaller to be used to populate default value when optional fields are not found.
        ManifestInstaller defaultInstaller;
        defaultInstaller.InstallerType = this->InstallerType;
        if (this->Switches.has_value())
        {
            defaultInstaller.Switches.emplace(this->Switches.value());
        }

        YAML::Node installersNode = rootNode["Installers"];
        for (std::size_t i = 0; i < installersNode.size(); i++) {
            YAML::Node installerNode = installersNode[i];
            ManifestInstaller installer;
            installer.PopulateInstallerFields(installerNode, defaultInstaller);
            this->Installers.emplace_back(std::move(installer));
        }

        // Create default ManifestLocalization to be used to populate default value when optional fields are not found.
        ManifestLocalization defaultLocalization;
        defaultLocalization.Description = this->Description;
        defaultLocalization.Homepage = this->Homepage;
        defaultLocalization.LicenseUrl = this->LicenseUrl;

        if (rootNode["Localization"])
        {
            YAML::Node localizationsNode = rootNode["Localization"];
            for (std::size_t i = 0; i < localizationsNode.size(); i++) {
                YAML::Node localizationNode = localizationsNode[i];
                ManifestLocalization localization;
                localization.PopulateLocalizationFields(localizationNode, defaultLocalization);
                this->Localization.emplace_back(std::move(localization));
            }
        }
    }

    Manifest Manifest::CreateFromPath(const std::filesystem::path& inputFile)
    {
        Manifest manifest;

        try
        {
            YAML::Node rootNode = YAML::LoadFile(inputFile.u8string());
            manifest.PopulateManifestFields(rootNode);
        }
        catch (std::runtime_error & e)
        {
            AICLI_LOG(YAML, Error, << "Failed to create manifest from file: " << inputFile.u8string());
            THROW_EXCEPTION_MSG(ManifestException(), e.what());
        }

        return manifest;
    }

    Manifest Manifest::Create(const std::string& input)
    {
        Manifest manifest;

        try
        {
            YAML::Node rootNode = YAML::Load(input);
            manifest.PopulateManifestFields(rootNode);
        }
        catch (std::runtime_error& e)
        {
            AICLI_LOG(YAML, Error, << "Failed to create manifest: " << input);
            THROW_EXCEPTION_MSG(ManifestException(), e.what());
        }

        return manifest;
    }
}
