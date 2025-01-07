// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Manifest.h>
#include <winget/ManifestValidation.h>
#include <winget/Yaml.h>

namespace AppInstaller::Manifest
{
    // Add here new manifest pointer types.
    using VariantManifestPtr = std::variant<Agreement*, AppsAndFeaturesEntry*, Dependency*, DependencyList*, Documentation*, ExpectedReturnCode*, Icon*, InstallationMetadataInfo*, InstalledFile*, Manifest*, ManifestInstaller*, ManifestLocalization*, MarketsInfo*, NestedInstallerFile*, std::map<InstallerSwitchType, Utility::NormalizedString>*, AppInstaller::Authentication::AuthenticationInfo*, AppInstaller::Authentication::MicrosoftEntraIdAuthenticationInfo*>;

    struct ManifestYamlPopulator
    {
        static std::vector<ValidationError> PopulateManifest(
            YAML::Node& rootNode,
            Manifest& manifest,
            const ManifestVer& manifestVersion,
            ManifestValidateOption validateOption,
            const std::optional<YAML::Node>& shadowNode);

    private:

        ManifestYamlPopulator(YAML::Node& rootNode, Manifest& manifest, const ManifestVer& manifestVersion, ManifestValidateOption validateOption);

        std::reference_wrapper<YAML::Node> m_rootNode;
        std::reference_wrapper<Manifest> m_manifest;
        std::reference_wrapper<const ManifestVer> m_manifestVersion;
        bool m_isMergedManifest = false;
        ManifestValidateOption m_validateOption;

        // Struct mapping a manifest field to its population logic
        struct FieldProcessInfo
        {
            FieldProcessInfo(std::string name, std::function<std::vector<ValidationError>(const YAML::Node&, const VariantManifestPtr& v)> func, bool requireVerifiedPublisher = false) :
                Name(std::move(name)), ProcessFunc(func), RequireVerifiedPublisher(requireVerifiedPublisher) {}

            std::string Name;
            std::function<std::vector<ValidationError>(const YAML::Node&, const VariantManifestPtr& v)> ProcessFunc;
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
        std::vector<FieldProcessInfo> AuthenticationFieldInfos;
        std::vector<FieldProcessInfo> MicrosoftEntraIdAuthenticationInfoFieldInfos;

        // Cache of Installers node and Localization node
        YAML::Node const* m_p_installersNode = nullptr;
        YAML::Node const* m_p_localizationsNode = nullptr;

        std::vector<FieldProcessInfo> GetRootFieldProcessInfo();
        std::vector<FieldProcessInfo> GetInstallerFieldProcessInfo(bool forRootFields = false);
        std::vector<FieldProcessInfo> GetSwitchesFieldProcessInfo();
        std::vector<FieldProcessInfo> GetExpectedReturnCodesFieldProcessInfo();
        std::vector<FieldProcessInfo> GetDependenciesFieldProcessInfo();
        std::vector<FieldProcessInfo> GetPackageDependenciesFieldProcessInfo();
        std::vector<FieldProcessInfo> GetLocalizationFieldProcessInfo(bool forRootFields = false);
        std::vector<FieldProcessInfo> GetAgreementFieldProcessInfo();
        std::vector<FieldProcessInfo> GetMarketsFieldProcessInfo();
        std::vector<FieldProcessInfo> GetAppsAndFeaturesEntryFieldProcessInfo();
        std::vector<FieldProcessInfo> GetDocumentationFieldProcessInfo();
        std::vector<FieldProcessInfo> GetIconFieldProcessInfo();
        std::vector<FieldProcessInfo> GetNestedInstallerFileFieldProcessInfo();
        std::vector<FieldProcessInfo> GetInstallationMetadataFieldProcessInfo();
        std::vector<FieldProcessInfo> GetInstallationMetadataFilesFieldProcessInfo();
        std::vector<FieldProcessInfo> GetAuthenticationFieldInfos();
        std::vector<FieldProcessInfo> GetMicrosoftEntraIdAuthenticationInfoFieldInfos();

        // Shadow
        std::vector<FieldProcessInfo> GetShadowRootFieldProcessInfo();
        std::vector<FieldProcessInfo> GetShadowLocalizationFieldProcessInfo();

        // This method takes YAML root node and list of manifest field info.
        // Yaml lib does not support case-insensitive search and it allows duplicate keys. If duplicate keys exist,
        // the value is undefined. So in this method, we will iterate through the node map and process each individual
        // pair ourselves. This also helps with generating aggregated error rather than throwing on first failure.
        std::vector<ValidationError> ValidateAndProcessFields(
            const YAML::Node& rootNode,
            const std::vector<FieldProcessInfo>& fieldInfos,
            const VariantManifestPtr& v);

        std::vector<ValidationError> ProcessPackageDependenciesNode(const YAML::Node& rootNode, DependencyList* dependencyList);
        std::vector<ValidationError> ProcessAgreementsNode(const YAML::Node& agreementsNode, ManifestLocalization* localization);
        std::vector<ValidationError> ProcessMarketsNode(const YAML::Node& marketsNode, AppInstaller::Manifest::ManifestInstaller* installer);
        std::vector<ValidationError> ProcessAppsAndFeaturesEntriesNode(const YAML::Node& appsAndFeaturesEntriesNode, AppInstaller::Manifest::ManifestInstaller* installer);
        std::vector<ValidationError> ProcessExpectedReturnCodesNode(const YAML::Node& returnCodesNode, AppInstaller::Manifest::ManifestInstaller* installer);
        std::vector<ValidationError> ProcessDocumentationsNode(const YAML::Node& documentationsNode, ManifestLocalization* localization);
        std::vector<ValidationError> ProcessIconsNode(const YAML::Node& iconsNode, ManifestLocalization* localization);
        std::vector<ValidationError> ProcessNestedInstallerFilesNode(const YAML::Node& nestedInstallerFilesNode, AppInstaller::Manifest::ManifestInstaller* installer);
        std::vector<ValidationError> ProcessInstallationMetadataFilesNode(const YAML::Node& installedFilesNode, InstallationMetadataInfo* installationMetadata);
        std::vector<ValidationError> ProcessShadowLocalizationNode(const YAML::Node& localizationNode, Manifest* manifest);

        std::vector<ValidationError> PopulateManifestInternal();
        std::vector<ValidationError> InsertShadow(const YAML::Node& shadowNode);
    };
}
