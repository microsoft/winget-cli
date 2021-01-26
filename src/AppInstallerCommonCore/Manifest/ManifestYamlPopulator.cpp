// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Manifest.h"
#include "winget/ManifestValidation.h"
#include "winget/Yaml.h"

namespace AppInstaller::Manifest
{
    using ValidationErrors = std::vector<ValidationError>;

    struct FieldProcessInfo
    {
        FieldProcessInfo(std::string name, std::function<ValidationErrors(const YAML::Node&)> func) :
            Name(std::move(name)), ProcessFunc(func) {}

        std::string Name;
        std::function<ValidationErrors(const YAML::Node&)> ProcessFunc;
    };

    std::vector<FieldProcessInfo> GetRootFieldProcessInfo(const ManifestVer& manifestVersion, Manifest** ppManifest)
    {
        std::vector<FieldProcessInfo> result =
        {
            { "ManifestVersion", [](const YAML::Node&)->ValidationErrors { /* ManifestVersion already processed. Field listed here for duplicate and PascalCase check */ return {}; } },
            { "Installers", [=](const YAML::Node& value)->ValidationErrors { *m_p_installersNode = value; }, true },
            { "Localization", [=](const YAML::Node& value)->ValidationErrors { *m_p_localizationsNode = value; } },
        };

        if (manifestVersion.Major() == 0)
        {
            return
            {
                { "Id", [=](const YAML::Node& value)->ValidationErrors { auto s = value.as<std::string>(); (*ppManifest)->Id = Utility::Trim(s); return {}; } },
                { "Version", [=](const YAML::Node& value)->ValidationErrors { auto s = value.as<std::string>(); (*ppManifest)->Version = Utility::Trim(s); return {}; } },
                { "Name", [=](const YAML::Node& value)->ValidationErrors { auto s = value.as<std::string>(); (*ppManifest)->DefaultLocalization.Add<Localization::PackageName>(Utility::Trim(s)); return {}; } },
                { "Publisher", [=](const YAML::Node& value)->ValidationErrors { (*ppManifest)->DefaultLocalization.Add<Localization::Publisher>(value.as<std::string>()); return {}; } },
                { "Author", [=](const YAML::Node& value)->ValidationErrors { (*ppManifest)->DefaultLocalization.Add<Localization::Author>(value.as<std::string>()); return {}; } },
                { "License", [=](const YAML::Node& value)->ValidationErrors { (*ppManifest)->DefaultLocalization.Add<Localization::License>(value.as<std::string>()); return {}; } },
                { "LicenseUrl", [=](const YAML::Node& value)->ValidationErrors { (*ppManifest)->DefaultLocalization.Add<Localization::LicenseUrl>(value.as<std::string>()); return {}; } },
                { "AppMoniker", [=](const YAML::Node& value)->ValidationErrors { auto s = value.as<std::string>(); (*ppManifest)->DefaultLocalization.Add<Localization::Moniker>(Utility::Trim(s)); return {}; } },
                { "Channel", [=](const YAML::Node& value)->ValidationErrors { auto s = value.as<std::string>(); (*ppManifest)->DefaultInstallerInfo.Channel = Utility::Trim(s); return {}; } },
                { "MinOSVersion", [=](const YAML::Node& value)->ValidationErrors { (*ppManifest)->DefaultInstallerInfo.MinOSVersion = value.as<std::string>(); } },
                { "Tags", [=](const YAML::Node& value)->ValidationErrors { m_p_manifest->Tags = SplitMultiValueField(value.as<std::string>()); } },
                { "Commands", [=](const YAML::Node& value)->ValidationErrors { m_p_manifest->Commands = SplitMultiValueField(value.as<std::string>()); } },
                { "Protocols", [=](const YAML::Node& value)->ValidationErrors { m_p_manifest->Protocols = SplitMultiValueField(value.as<std::string>()); } },
                { "FileExtensions", [=](const YAML::Node& value)->ValidationErrors { m_p_manifest->FileExtensions = SplitMultiValueField(value.as<std::string>()); } },
                { "UpdateBehavior", [=](const YAML::Node& value)->ValidationErrors { m_p_manifest->UpdateBehavior = ManifestInstaller::ConvertToUpdateBehaviorEnum(value.as<std::string>()); } },
                
                { "Homepage", [=](const YAML::Node& value)->ValidationErrors { m_p_manifest->Homepage = value.as<std::string>(); } },
                
                { "InstallerType", [=](const YAML::Node& value)->ValidationErrors { m_p_manifest->InstallerType = ManifestInstaller::ConvertToInstallerTypeEnum(value.as<std::string>()); } },
                { "PackageFamilyName", [=](const YAML::Node& value)->ValidationErrors { m_p_manifest->PackageFamilyName = value.as<std::string>(); }, false, "[-.A-Za-z0-9]+_[A-Za-z0-9]{13}" },
                { "ProductCode", [=](const YAML::Node& value)->ValidationErrors { m_p_manifest->ProductCode = value.as<std::string>(); } },
                { "Description", [=](const YAML::Node& value)->ValidationErrors { m_p_manifest->Description = value.as<std::string>(); } },
                { "Switches", [=](const YAML::Node& value)->ValidationErrors { *m_p_switchesNode = value; } },
                
            };
        }
        else
        {
            // Starting v1, we should be only adding new fields for each minor version increase
            if (manifestVersion >= ManifestVer{ s_ManifestVersionV1 })
            {

            }

        }
    }

    std::vector<FieldProcessInfo> GetInstallerFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        return {};
    }

    std::vector<FieldProcessInfo> GetLocalizationFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        return {};
    }

    std::vector<FieldProcessInfo> GetDependenciesFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        return {};
    }

    std::vector<FieldProcessInfo> GetPackageDependenciesFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        return {};
    }

    std::vector<ValidationError> PopulateManifest(const YAML::Node& rootNode, Manifest& manifest, const ManifestVer& manifestVersion)
    {
        

    }
}