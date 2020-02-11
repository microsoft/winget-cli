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
        std::string inStrLower = Utility::ToLower(in);
        InstallerTypeEnum result = InstallerTypeEnum::Unknown;

        if (inStrLower == "inno")
        {
            result = InstallerTypeEnum::Inno;
        }
        else if (inStrLower == "wix")
        {
            result = InstallerTypeEnum::Wix;
        }
        else if (inStrLower == "msi")
        {
            result = InstallerTypeEnum::Msi;
        }
        else if (inStrLower == "nullsoft")
        {
            result = InstallerTypeEnum::Nullsoft;
        }
        else if (inStrLower == "zip")
        {
            result = InstallerTypeEnum::Zip;
        }
        else if (inStrLower == "appx" || inStrLower == "msix")
        {
            result = InstallerTypeEnum::Msix;
        }
        else if (inStrLower == "exe")
        {
            result = InstallerTypeEnum::Exe;
        }
        else if (inStrLower == "burn")
        {
            result = InstallerTypeEnum::Burn;
        }
        else if (inStrLower == "installshield")
        {
            result = InstallerTypeEnum::InstallShield;
        }

        return result;
    }

    std::ostream& operator<<(std::ostream& out, const ManifestInstaller::InstallerTypeEnum& installerType)
    {
        switch (installerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
            out << "Exe";
            break;
        case ManifestInstaller::InstallerTypeEnum::Inno:
            out << "Inno";
            break;
        case ManifestInstaller::InstallerTypeEnum::Msi:
            out << "Msi";
            break;
        case ManifestInstaller::InstallerTypeEnum::Msix:
            out << "Msix";
            break;
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
            out << "Nullsoft";
            break;
        case ManifestInstaller::InstallerTypeEnum::Wix:
            out << "Wix";
            break;
        case ManifestInstaller::InstallerTypeEnum::Zip:
            out << "Zip";
            break;
        default:
            out << "Unknown";
        }

        return out;
    }
}
