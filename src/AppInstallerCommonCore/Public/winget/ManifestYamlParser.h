// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/ManifestValidation.h>
#include <winget/Manifest.h>

#include <filesystem>

namespace AppInstaller::Manifest
{
    struct YamlParser
    {
        // fullValidation: Bool to set if manifest creation should perform extra validation that client does not need.
        //                 e.g. Channel should be null. Client code does not need this check to work properly.
        // throwOnWarning: Bool to indicate if an exception should be thrown with only warnings detected in the manifest.
        static Manifest CreateFromPath(const std::filesystem::path& inputFile, bool fullValidation = false, bool throwOnWarning = false);

        static Manifest Create(const std::string& input, bool fullValidation = false, bool throwOnWarning = false);

    private:
        // These pointers are referenced in the processing functions in manifest field info table.
        YAML::Node* m_p_installersNode = nullptr;
        YAML::Node* m_p_switchesNode = nullptr;
        YAML::Node* m_p_localizationsNode = nullptr;
        AppInstaller::Manifest::Manifest* m_p_manifest = nullptr;
        AppInstaller::Manifest::ManifestInstaller* m_p_installer = nullptr;
        std::map<ManifestInstaller::InstallerSwitchType, Utility::NormalizedString>* m_p_switches = nullptr;
        AppInstaller::Manifest::ManifestLocalization* m_p_localization = nullptr;

        // This struct contains individual app manifest field info
        struct ManifestFieldInfo
        {
            ManifestFieldInfo(std::string name, std::function<void(const YAML::Node&)> func, bool required = false, std::string regex = {}) :
                Name(std::move(name)), ProcessFunc(func), Required(required), RegEx(std::move(regex)) {}

            std::string Name;
            std::function<void(const YAML::Node&)> ProcessFunc;
            bool Required = false;
            std::string RegEx = {};
        };

        std::vector<ManifestFieldInfo> RootFieldInfos;
        std::vector<ManifestFieldInfo> InstallerFieldInfos;
        std::vector<ManifestFieldInfo> SwitchesFieldInfos;
        std::vector<ManifestFieldInfo> LocalizationFieldInfos;

        std::vector<ValidationError> ParseManifest(const YAML::Node& rootNode, Manifest& manifest, bool fullValidation);

        static std::map<ManifestInstaller::InstallerSwitchType, Utility::NormalizedString> GetDefaultKnownSwitches(
            ManifestInstaller::InstallerTypeEnum installerType);

        // This method takes YAML root node and list of manifest field info.
        // Yaml-cpp does not support case insensitive search and it allows duplicate keys. If duplicate keys exist,
        // the value is undefined. So in this method, we will iterate through the node map and process each individual
        // pair ourselves. This also helps with generating aggregated error rather than throwing on first failure.
        static std::vector<ValidationError> ValidateAndProcessFields(
            const YAML::Node& rootNode,
            const std::vector<ManifestFieldInfo>& fieldInfos,
            bool fullValidation);

        void PrepareManifestFieldInfos(const ManifestVer& manifestVer);
    };
}