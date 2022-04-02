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
            ManifestValidateOption validateOption);

    private:

        bool m_isMergedManifest = false;
        ManifestValidateOption m_validateOption;

        // Struct mapping a manifest field to its population logic
        struct FieldProcessInfo
        {
            FieldProcessInfo(std::string name, std::function<std::vector<ValidationError>(const YAML::Node&)> func, bool requireVerifiedPublisher = false) :
                Name(std::move(name)), ProcessFunc(func), RequireVerifiedPublisher(requireVerifiedPublisher) {}

            std::string Name;
            std::function<std::vector<ValidationError>(const YAML::Node&)> ProcessFunc;
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

        // These pointers are referenced in the processing functions in manifest field process info table.
        AppInstaller::Manifest::Manifest* m_p_manifest = nullptr;
        AppInstaller::Manifest::ManifestInstaller* m_p_installer = nullptr;
        std::map<InstallerSwitchType, Utility::NormalizedString>* m_p_switches = nullptr;
        AppInstaller::Manifest::ExpectedReturnCode* m_p_expectedReturnCode = nullptr;
        AppInstaller::Manifest::DependencyList* m_p_dependencyList = nullptr;
        AppInstaller::Manifest::Dependency* m_p_packageDependency = nullptr;
        AppInstaller::Manifest::ManifestLocalization* m_p_localization = nullptr;
        AppInstaller::Manifest::Agreement* m_p_agreement = nullptr;
        AppInstaller::Manifest::MarketsInfo* m_p_markets = nullptr;
        AppInstaller::Manifest::AppsAndFeaturesEntry* m_p_appsAndFeaturesEntry = nullptr;
        AppInstaller::Manifest::Documentation* m_p_documentation = nullptr;

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

        // This method takes YAML root node and list of manifest field info.
        // Yaml lib does not support case insensitive search and it allows duplicate keys. If duplicate keys exist,
        // the value is undefined. So in this method, we will iterate through the node map and process each individual
        // pair ourselves. This also helps with generating aggregated error rather than throwing on first failure.
        std::vector<ValidationError> ValidateAndProcessFields(
            const YAML::Node& rootNode,
            const std::vector<FieldProcessInfo>& fieldInfos);

        void ProcessDependenciesNode(DependencyType type, const YAML::Node& rootNode);
        std::vector<ValidationError> ProcessPackageDependenciesNode(const YAML::Node& rootNode);
        std::vector<ValidationError> ProcessAgreementsNode(const YAML::Node& agreementsNode);
        std::vector<ValidationError> ProcessMarketsNode(const YAML::Node& marketsNode);
        std::vector<ValidationError> ProcessAppsAndFeaturesEntriesNode(const YAML::Node& appsAndFeaturesEntriesNode);
        std::vector<ValidationError> ProcessExpectedReturnCodesNode(const YAML::Node& returnCodesNode);
        std::vector<ValidationError> ProcessDocumentationsNode(const YAML::Node& documentations);

        std::vector<ValidationError> PopulateManifestInternal(
            const YAML::Node& rootNode,
            Manifest& manifest,
            const ManifestVer& manifestVersion,
            ManifestValidateOption validateOption);
    };
}