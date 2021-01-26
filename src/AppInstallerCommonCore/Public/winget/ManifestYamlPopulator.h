// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Manifest.h>
#include <winget/ManifestValidation.h>
#include <winget/Yaml.h>

namespace AppInstaller::Manifest
{
    struct ManifestYamlPopulator
    {
        std::vector<ValidationError> PopulateManifest(const YAML::Node& rootNode, Manifest& manifest);

    private:

        struct FieldProcessInfo
        {
            FieldProcessInfo(std::string name, std::function<std::vector<ValidationError>(const YAML::Node&)> func) :
                Name(std::move(name)), ProcessFunc(func) {}

            std::string Name;
            std::function<std::vector<ValidationError>(const YAML::Node&)> ProcessFunc;
        };

        // Installers node need to be processed after other fields in package root for default installer values
        YAML::Node* m_p_installersNode;

        std::vector<FieldProcessInfo> RootFieldInfos;
        std::vector<FieldProcessInfo> InstallerFieldInfos;
        std::vector<FieldProcessInfo> SwitchesFieldInfos;
        std::vector<FieldProcessInfo> DependenciesFieldInfos;
        std::vector<FieldProcessInfo> PackageDependenciesFieldInfos;
        std::vector<FieldProcessInfo> LocalizationFieldInfos;

        AppInstaller::Manifest::Manifest* m_p_manifest = nullptr;
        AppInstaller::Manifest::ManifestInstaller* m_p_installer = nullptr;
        std::map<InstallerSwitchType, Utility::NormalizedString>* m_p_switches = nullptr;
        AppInstaller::Manifest::Dependency* m_p_dependency = nullptr;
        AppInstaller::Manifest::PackageDependency* m_p_packageDependency = nullptr;
        AppInstaller::Manifest::Localization* m_p_localization = nullptr;

        void GetRootFieldProcessInfo(const ManifestVer& manifestVersion);
        void GetInstallerFieldProcessInfo(const ManifestVer& manifestVersion);
        void GetSwitchesFieldProcessInfo(const ManifestVer& manifestVersion);
        void GetDependenciesFieldProcessInfo(const ManifestVer& manifestVersion);
        void GetPackageDependenciesFieldProcessInfo(const ManifestVer& manifestVersion);
        void GetLocalizationFieldProcessInfo(const ManifestVer& manifestVersion);
    };
}