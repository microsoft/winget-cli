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

        std::map<InstallerSwitchType, string_t> defaultKnownSwitches = GetDefaultKnownSwitches(this->InstallerType);

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
        std::map<InstallerSwitchType, string_t>& switches,
        const std::map<InstallerSwitchType, string_t>* manifestRootSwitches,
        const std::map<InstallerSwitchType, string_t>* defaultKnownSwitches)
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
        std::map<InstallerSwitchType, string_t>& switches,
        const std::map<InstallerSwitchType, string_t>* manifestRootSwitches,
        const std::map<InstallerSwitchType, string_t>* defaultKnownSwitches)
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

    std::map<ManifestInstaller::InstallerSwitchType, ManifestInstaller::string_t> ManifestInstaller::GetDefaultKnownSwitches(InstallerTypeEnum installerType)
    {
        switch (installerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Wix:
        case ManifestInstaller::InstallerTypeEnum::Msi:
            return
            {
                {InstallerSwitchType::Silent, string_t("/quiet")},
                {InstallerSwitchType::SilentWithProgress, string_t("/passive")},
                {InstallerSwitchType::Log, string_t("/log \"" + std::string(ARG_TOKEN_LOGPATH) + "\"")},
                {InstallerSwitchType::InstallLocation, string_t("TARGETDIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"")}
            };
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
            return
            {
                {InstallerSwitchType::Silent, string_t("/S")},
                {InstallerSwitchType::SilentWithProgress, string_t("/S")},
                {InstallerSwitchType::InstallLocation, string_t("/D=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"")}
            };
        case ManifestInstaller::InstallerTypeEnum::Inno:
            return
            {
                {InstallerSwitchType::Silent, string_t("/VERYSILENT")},
                {InstallerSwitchType::SilentWithProgress, string_t("/SILENT")},
                {InstallerSwitchType::Log, string_t("/LOG=\"" + std::string(ARG_TOKEN_LOGPATH) + "\"")},
                {InstallerSwitchType::InstallLocation, string_t("/DIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"")}
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

        return result;
    }

    std::string ManifestInstaller::InstallerTypeToString(ManifestInstaller::InstallerTypeEnum installerType)
    {
        std::string result = "Unknown";

        switch (installerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
            result = "Exe";
            break;
        case ManifestInstaller::InstallerTypeEnum::Inno:
            result = "Inno";
            break;
        case ManifestInstaller::InstallerTypeEnum::Msi:
            result = "Msi";
            break;
        case ManifestInstaller::InstallerTypeEnum::Msix:
            result = "Msix";
            break;
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
            result = "Nullsoft";
            break;
        case ManifestInstaller::InstallerTypeEnum::Wix:
            result = "Wix";
            break;
        case ManifestInstaller::InstallerTypeEnum::Zip:
            result = "Zip";
            break;
        case ManifestInstaller::InstallerTypeEnum::Burn:
            result = "Burn";
            break;
        }

        return result;
    }
}
