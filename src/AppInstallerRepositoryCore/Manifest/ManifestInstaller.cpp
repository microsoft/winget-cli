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
            defaultInstaller.InstallerType;

        std::map<InstallerSwitchType, std::string> defaultKnownSwitches = GetDefaultArgs(this->InstallerType);

        if (installerNode["Switches"])
        {
            YAML::Node switchesNode = installerNode["Switches"];
            PopulateSwitchesFields(&switchesNode, this->Switches, &(defaultInstaller.Switches), &defaultKnownSwitches);
        }
        else
        {
            PopulateSwitchesFields(nullptr, this->Switches, &(defaultInstaller.Switches), &defaultKnownSwitches);
        }
    }

    void ManifestInstaller::PopulateSwitchesFields(
        const YAML::Node* switchesNode,
        std::map<InstallerSwitchType, std::string>& switches,
        const std::map<InstallerSwitchType, std::string>* manifestRootSwitches,
        const std::map<InstallerSwitchType, std::string>* defaultKnownSwitches)
    {
        PopulateOneSwitchField(switchesNode, "Custom", InstallerSwitchType::Custom, switches, manifestRootSwitches, defaultKnownSwitches);
        PopulateOneSwitchField(switchesNode, "Silent", InstallerSwitchType::Silent, switches, manifestRootSwitches, defaultKnownSwitches);
        PopulateOneSwitchField(switchesNode, "SilentWithProgress", InstallerSwitchType::SilentWithProgress, switches, manifestRootSwitches, defaultKnownSwitches);
        PopulateOneSwitchField(switchesNode, "Interactive", InstallerSwitchType::Interactive, switches, manifestRootSwitches, defaultKnownSwitches);
        PopulateOneSwitchField(switchesNode, "Language", InstallerSwitchType::Language, switches, manifestRootSwitches, defaultKnownSwitches);
        PopulateOneSwitchField(switchesNode, "Log", InstallerSwitchType::Log, switches, manifestRootSwitches, defaultKnownSwitches);
        PopulateOneSwitchField(switchesNode, "InstallLocation", InstallerSwitchType::InstallLocation, switches, manifestRootSwitches, defaultKnownSwitches);
    }

    void ManifestInstaller::PopulateOneSwitchField(
        const YAML::Node* switchesNode,
        const std::string& switchName,
        InstallerSwitchType switchType,
        std::map<InstallerSwitchType, std::string>& switches,
        const std::map<InstallerSwitchType, std::string>* manifestRootSwitches,
        const std::map<InstallerSwitchType, std::string>* defaultKnownSwitches)
    {
        if (switchesNode && (*switchesNode)[switchName])
        {
            switches.emplace(switchType, (*switchesNode)[switchName].as<std::string>());
        }
        else if (manifestRootSwitches && manifestRootSwitches->find(switchType) != manifestRootSwitches->end())
        {
            switches.emplace(switchType, manifestRootSwitches->at(switchType));
        }
        else if (defaultKnownSwitches && defaultKnownSwitches->find(switchType) != defaultKnownSwitches->end())
        {
            switches.emplace(switchType, defaultKnownSwitches->at(switchType));
        }
    }

    std::map<ManifestInstaller::InstallerSwitchType, std::string> ManifestInstaller::GetDefaultArgs(InstallerTypeEnum installerType)
    {
        switch (installerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Wix:
        case ManifestInstaller::InstallerTypeEnum::Msi:
            return
            {
                {InstallerSwitchType::Silent, "/quiet"},
                {InstallerSwitchType::SilentWithProgress, "/passive"},
                {InstallerSwitchType::Log, "/log \"" + std::string(ARG_TOKEN_LOGPATH) + "\""},
                {InstallerSwitchType::InstallLocation, "TARGETDIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\""}
            };
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
            return
            {
                {InstallerSwitchType::Silent, "/S"},
                {InstallerSwitchType::SilentWithProgress, "/S"},
                {InstallerSwitchType::InstallLocation, "/D=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\""}
            };
        case ManifestInstaller::InstallerTypeEnum::Inno:
            return
            {
                {InstallerSwitchType::Silent, "/VERYSILENT"},
                {InstallerSwitchType::SilentWithProgress, "/SILENT"},
                {InstallerSwitchType::Log, "/LOG=\"" + std::string(ARG_TOKEN_LOGPATH) + "\""},
                {InstallerSwitchType::InstallLocation, "/DIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\""}
            };
        }
        return {};
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
        case ManifestInstaller::InstallerTypeEnum::Burn:
            out << "Burn";
            break;
        case ManifestInstaller::InstallerTypeEnum::InstallShield:
            out << "InstallShield";
            break;
        default:
            out << "Unknown";
        }

        return out;
    }
}
