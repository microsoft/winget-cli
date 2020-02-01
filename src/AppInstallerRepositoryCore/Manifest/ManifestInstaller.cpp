// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ManifestInstaller.h"

namespace AppInstaller::Manifest
{
    void ManifestInstaller::PopulateInstallerFields(const YAML::Node& installerNode, const ManifestInstaller& defaultInstaller)
    {
        // Required fields
        this->Arch = Utility::ConvertToArchitectureEnum(installerNode["Arch"].as<std::string>());
        this->Url = installerNode["Url"].as<std::string>();
        this->Sha256 = Utility::SHA256::ConvertToBytes(installerNode["Sha256"].as<std::string>());

        if (installerNode["SignatureSha256"])
        {
            this->SignatureSha256 = Utility::SHA256::ConvertToBytes(installerNode["SignatureSha256"].as<std::string>());
        }

        // Optional fields.
        this->Language = installerNode["Language"] ? installerNode["Language"].as<std::string>() : "";
        this->Scope = installerNode["Scope"] ? installerNode["Scope"].as<std::string>() : "";

        this->InstallerType = installerNode["InstallerType"] ?
            ConvertToInstallerTypeEnum(installerNode["InstallerType"].as<std::string>()) :
            InstallerTypeEnum::Unknown;

        if (installerNode["Switches"])
        {
            YAML::Node switchesNode = installerNode["Switches"];
            InstallerSwitches switches;
            switches.PopulateSwitchesFields(switchesNode,
                defaultInstaller.Switches.has_value() ? &(defaultInstaller.Switches.value()) : nullptr);
            this->Switches.emplace(std::move(switches));
        }
        else if (defaultInstaller.Switches.has_value())
        {
            this->Switches.emplace(defaultInstaller.Switches.value());
        }
    }

    ManifestInstaller::InstallerTypeEnum ManifestInstaller::ConvertToInstallerTypeEnum(const std::string& in)
    {
        static std::map<std::string, InstallerTypeEnum> InstallerTypeEnumMap = {
            { "inno", InstallerTypeEnum::Inno },
            { "wix", InstallerTypeEnum::Wix },
            { "msi", InstallerTypeEnum::Msi },
            { "nullsoft", InstallerTypeEnum::Nullsoft },
            { "zip", InstallerTypeEnum::Zip },
            { "appx", InstallerTypeEnum::Msix },
            { "msix", InstallerTypeEnum::Msix },
            { "exe", InstallerTypeEnum::Exe },
        };

        std::string inStrLower = Utility::ToLower(in);
        auto iter = InstallerTypeEnumMap.find(inStrLower);

        if (iter != InstallerTypeEnumMap.end())
        {
            return iter->second;
        }

        return InstallerTypeEnum::Unknown;
    }
}
