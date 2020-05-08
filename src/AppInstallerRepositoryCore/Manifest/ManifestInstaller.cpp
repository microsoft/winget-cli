// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ManifestInstaller.h"

namespace AppInstaller::Manifest
{
    std::vector<ValidationError> ManifestInstaller::PopulateInstallerFields(
        const YAML::Node& installerNode,
        const ManifestInstaller& defaultInstaller,
        bool fullValidation,
        ManifestVer manifestVersion)
    {
        YAML::Node switchesNode;
        this->InstallerType = defaultInstaller.InstallerType;
        this->Scope = "user";

        std::vector<ManifestFieldInfo> fieldInfos;

        if (manifestVersion >= PreviewManifestVersion)
        {
            std::vector<ManifestFieldInfo> previewFieldInfos =
            {
                { "Arch", [this](const YAML::Node& value) { Arch = Utility::ConvertToArchitectureEnum(value.as<std::string>()); }, true },
                { "Url", [this](const YAML::Node& value) { Url = value.as<std::string>(); }, true },
                { "Sha256", [this](const YAML::Node& value) { Sha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>()); }, true, "^[A-Fa-f0-9]{64}$" },
                { "SignatureSha256", [this](const YAML::Node& value) { SignatureSha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>()); } },
                { "Language", [this](const YAML::Node& value) { Language = value.as<std::string>(); } },
                { "Scope", [this](const YAML::Node& value) { Scope = value.as<std::string>(); } },
                { "InstallerType", [this](const YAML::Node& value) { InstallerType = ConvertToInstallerTypeEnum(value.as<std::string>()); } },
                { "Switches", [&](const YAML::Node& value) { switchesNode = value; } },
            };

            std::move(previewFieldInfos.begin(), previewFieldInfos.end(), std::inserter(fieldInfos, fieldInfos.end()));
        }

        auto resultErrors = ValidateAndProcessFields(installerNode, fieldInfos, fullValidation);

        // Populate default known switches
        this->Switches = GetDefaultKnownSwitches(this->InstallerType);

        // Override with switches from manifest root if applicable
        for (auto const& keyValuePair : defaultInstaller.Switches)
        {
            this->Switches[keyValuePair.first] = keyValuePair.second;
        }

        // Override with switches from installer if applicable
        if (!switchesNode.IsNull())
        {
            auto errors = PopulateSwitchesFields(switchesNode, this->Switches, fullValidation, manifestVersion);
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
        }

        // Extra semantic validations after basic validation and field population
        if (fullValidation)
        {
            if (Arch == Utility::Architecture::Unknown)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "Arch");
            }

            if (InstallerType == InstallerTypeEnum::Unknown)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "InstallerType");
            }

            if (InstallerType == InstallerTypeEnum::Exe &&
                (Switches.find(InstallerSwitchType::SilentWithProgress) == Switches.end() ||
                 Switches.find(InstallerSwitchType::Silent) == Switches.end()))
            {
                resultErrors.emplace_back(ManifestError::ExeInstallerMissingSilentSwitches, ValidationError::Level::Warning);
            }

            // Check empty string before calling IsValidUrl to avoid duplicate error reporting.
            if (!Url.empty() && IsValidURL(NULL, Utility::ConvertToUTF16(Url).c_str(), 0) == S_FALSE)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "Url", Url);
            }
        }

        return resultErrors;
    }

    std::vector<ValidationError> ManifestInstaller::PopulateSwitchesFields(
        const YAML::Node& switchesNode,
        std::map<InstallerSwitchType, string_t>& switches,
        bool fullValidation,
        ManifestVer manifestVersion)
    {
        std::vector<ManifestFieldInfo> fieldInfos;

        if (manifestVersion >= PreviewManifestVersion)
        {
            std::vector<ManifestFieldInfo> previewFieldInfos =
            {
                { "Custom", [&](const YAML::Node& value) { switches[InstallerSwitchType::Custom] = value.as<std::string>(); } },
                { "Silent", [&](const YAML::Node& value) { switches[InstallerSwitchType::Silent] = value.as<std::string>(); } },
                { "SilentWithProgress", [&](const YAML::Node& value) { switches[InstallerSwitchType::SilentWithProgress] = value.as<std::string>(); } },
                { "Interactive", [&](const YAML::Node& value) { switches[InstallerSwitchType::Interactive] = value.as<std::string>(); } },
                { "Language", [&](const YAML::Node& value) { switches[InstallerSwitchType::Language] = value.as<std::string>(); } },
                { "Log", [&](const YAML::Node& value) { switches[InstallerSwitchType::Log] = value.as<std::string>(); } },
                { "InstallLocation", [&](const YAML::Node& value) { switches[InstallerSwitchType::InstallLocation] = value.as<std::string>(); } },
            };

            std::move(previewFieldInfos.begin(), previewFieldInfos.end(), std::inserter(fieldInfos, fieldInfos.end()));
        }

        return ValidateAndProcessFields(switchesNode, fieldInfos, fullValidation);
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
