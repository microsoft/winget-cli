// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/ManifestValidation.h>
#include <winget/ManifestSchemaValidation.h>
#include <winget/Manifest.h>

#include <filesystem>

namespace AppInstaller::Manifest
{
    struct YamlParser
    {
        // fullValidation: Bool to set if manifest creation should perform extra validation that client does not need.
        //                 e.g. Channel should be null. Client code does not need this check to work properly.
        // throwOnWarning: Bool to indicate if an exception should be thrown with only warnings detected in the manifest.
        static Manifest CreateFromPath(const std::filesystem::path& inputPath, bool fullValidation = false, bool throwOnWarning = false);

        static Manifest Create(const std::string& input, bool fullValidation = false, bool throwOnWarning = false);

    private:

        

        static Manifest CreateInternal(const std::vector<YamlManifestInfo>& input, bool fullValidation, bool throwOnWarning);
        // These pointers are referenced in the processing functions in manifest field info table.
        YAML::Node* m_p_installersNode = nullptr;
        YAML::Node* m_p_switchesNode = nullptr;
        YAML::Node* m_p_localizationsNode = nullptr;
        AppInstaller::Manifest::Manifest* m_p_manifest = nullptr;
        AppInstaller::Manifest::ManifestInstaller* m_p_installer = nullptr;
        std::map<InstallerSwitchType, Utility::NormalizedString>* m_p_switches = nullptr;
        AppInstaller::Manifest::ManifestLocalization* m_p_localization = nullptr;

        // This struct contains individual manifest field population info
        struct ManifestFieldInfo
        {
            ManifestFieldInfo(std::string name, std::function<std::vector<ValidationError>(const YAML::Node&)> func) :
                Name(std::move(name)), ProcessFunc(func) {}

            std::string Name;
            std::function<std::vector<ValidationError>(const YAML::Node&)> ProcessFunc;
        };

        std::vector<ManifestFieldInfo> RootFieldInfos;
        std::vector<ManifestFieldInfo> InstallerFieldInfos;
        std::vector<ManifestFieldInfo> SwitchesFieldInfos;
        std::vector<ManifestFieldInfo> LocalizationFieldInfos;

        std::vector<ValidationError> ParseManifest(const std::vector<YamlManifestInfo>& input, Manifest& manifest, bool fullValidation, bool isPartialManifest = false);
        ManifestVer ValidateInput(const std::vector<YamlManifestInfo>& input, bool fullValidation, bool isPartialManifest);

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