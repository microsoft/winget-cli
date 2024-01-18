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
        static std::vector<ValidationError> PopulateManifest(
            const YAML::Node& rootNode,
            Manifest& manifest,
            const ManifestVer& manifestVersion,
            ManifestValidateOption validateOption,
            const std::optional<YAML::Node>& shadowNode);

    private:

        bool m_isMergedManifest = false;
        ManifestValidateOption m_validateOption;

        // Struct mapping a manifest field to its population logic
        struct FieldProcessInfo
        {
            FieldProcessInfo(std::string name, std::function<std::vector<ValidationError>(const YAML::Node&, std::any&)> func, bool requireVerifiedPublisher = false) :
                Name(std::move(name)), ProcessFunc(func), RequireVerifiedPublisher(requireVerifiedPublisher) {}

            std::string Name;
            std::function<std::vector<ValidationError>(const YAML::Node&, std::any& any)> ProcessFunc;
            bool RequireVerifiedPublisher = false;
        };

        std::vector<FieldProcessInfo> RootFieldInfos;
        std::vector<FieldProcessInfo> InstallerFieldInfos;
        std::vector<FieldProcessInfo> SwitchesFieldInfos;
        std::vector<FieldProcessInfo> ExpectedReturnCodesFieldInfos;
        std::vector<FieldProcessInfo> DependenciesFieldInfos;
        std::vector<FieldProcessInfo> PackageDependenciesFieldInfos;
        std::vector<FieldProcessInfo> LocalizationFieldInfos;
        std::vector<FieldProcessInfo> AgreementFieldInfos;
        std::vector<FieldProcessInfo> MarketsFieldInfos;
        std::vector<FieldProcessInfo> AppsAndFeaturesEntryFieldInfos;
        std::vector<FieldProcessInfo> DocumentationFieldInfos;
        std::vector<FieldProcessInfo> IconFieldInfos;
        std::vector<FieldProcessInfo> NestedInstallerFileFieldInfos;
        std::vector<FieldProcessInfo> InstallationMetadataFieldInfos;
        std::vector<FieldProcessInfo> InstallationMetadataFilesFieldInfos;

        // Shadow manifest
        std::vector<FieldProcessInfo> ShadowIconFieldInfos;
        std::vector<FieldProcessInfo> ShadowLocalizationFieldInfos;

        // Cache of Installers node and Localization node
        YAML::Node const* m_p_installersNode = nullptr;
        YAML::Node const* m_p_localizationsNode = nullptr;

        std::vector<FieldProcessInfo> GetRootFieldProcessInfo(const ManifestVer& manifestVersion);
        std::vector<FieldProcessInfo> GetInstallerFieldProcessInfo(const ManifestVer& manifestVersion, bool forRootFields = false);
        std::vector<FieldProcessInfo> GetSwitchesFieldProcessInfo(const ManifestVer& manifestVersion);
        std::vector<FieldProcessInfo> GetExpectedReturnCodesFieldProcessInfo(const ManifestVer& manifestVersion);
        std::vector<FieldProcessInfo> GetDependenciesFieldProcessInfo(const ManifestVer& manifestVersion);
        std::vector<FieldProcessInfo> GetPackageDependenciesFieldProcessInfo(const ManifestVer& manifestVersion);
        std::vector<FieldProcessInfo> GetLocalizationFieldProcessInfo(const ManifestVer& manifestVersion, bool forRootFields = false);
        std::vector<FieldProcessInfo> GetAgreementFieldProcessInfo(const ManifestVer& manifestVersion);
        std::vector<FieldProcessInfo> GetMarketsFieldProcessInfo(const ManifestVer& manifestVersion);
        std::vector<FieldProcessInfo> GetAppsAndFeaturesEntryFieldProcessInfo(const ManifestVer& manifestVersion);
        std::vector<FieldProcessInfo> GetDocumentationFieldProcessInfo(const ManifestVer& manifestVersion);
        std::vector<FieldProcessInfo> GetIconFieldProcessInfo(const ManifestVer& manifestVersion);
        std::vector<FieldProcessInfo> GetNestedInstallerFileFieldProcessInfo(const ManifestVer& manifestVersion);
        std::vector<FieldProcessInfo> GetInstallationMetadataFieldProcessInfo(const ManifestVer& manifestVersion);
        std::vector<FieldProcessInfo> GetInstallationMetadataFilesFieldProcessInfo(const ManifestVer& manifestVersion);

        // This method takes YAML root node and list of manifest field info.
        // Yaml lib does not support case insensitive search and it allows duplicate keys. If duplicate keys exist,
        // the value is undefined. So in this method, we will iterate through the node map and process each individual
        // pair ourselves. This also helps with generating aggregated error rather than throwing on first failure.
        std::vector<ValidationError> ValidateAndProcessFields(
            const YAML::Node& rootNode,
            const std::vector<FieldProcessInfo>& fieldInfos,
            std::any& any);

        void ProcessDependenciesNode(DependencyType type, const YAML::Node& rootNode, DependencyList* dependencyList);
        std::vector<ValidationError> ProcessPackageDependenciesNode(const YAML::Node& rootNode, DependencyList* dependencyList);
        std::vector<ValidationError> ProcessAgreementsNode(const YAML::Node& agreementsNode, ManifestLocalization* localization);
        std::vector<ValidationError> ProcessMarketsNode(const YAML::Node& marketsNode, AppInstaller::Manifest::ManifestInstaller* installer);
        std::vector<ValidationError> ProcessAppsAndFeaturesEntriesNode(const YAML::Node& appsAndFeaturesEntriesNode, AppInstaller::Manifest::ManifestInstaller* installer);
        std::vector<ValidationError> ProcessExpectedReturnCodesNode(const YAML::Node& returnCodesNode, AppInstaller::Manifest::ManifestInstaller* installer);
        std::vector<ValidationError> ProcessDocumentationsNode(const YAML::Node& documentationsNode, ManifestLocalization* localization);
        std::vector<ValidationError> ProcessIconsNode(const YAML::Node& iconsNode, ManifestLocalization* localization);
        std::vector<ValidationError> ProcessNestedInstallerFilesNode(const YAML::Node& nestedInstallerFilesNode, AppInstaller::Manifest::ManifestInstaller* installer);
        std::vector<ValidationError> ProcessInstallationMetadataFilesNode(const YAML::Node& installedFilesNode, InstallationMetadataInfo* installationMetadata);

        std::vector<ValidationError> PopulateManifestInternal(
            const YAML::Node& rootNode,
            Manifest& manifest,
            const ManifestVer& manifestVersion,
            ManifestValidateOption validateOption,
            const std::optional<YAML::Node>& shadowNode);
    };
}