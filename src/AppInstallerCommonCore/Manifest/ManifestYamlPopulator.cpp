// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerSHA256.h"
#include "winget/ManifestYamlPopulator.h"

namespace AppInstaller::Manifest
{
    using ValidationErrors = std::vector<ValidationError>;
    using ExpectedReturnCodeInfo = AppInstaller::Manifest::ManifestInstaller::ExpectedReturnCodeInfo;

    namespace
    {
        template <typename Ptr>
        Ptr* variant_ptr(const VariantManifestPtr& v) { return std::get<Ptr*>(v); }

        ManifestInstaller* GetManifestInstallerPtrFromManifest(const VariantManifestPtr& v) { return &(variant_ptr<Manifest>(v)->DefaultInstallerInfo); }

        ManifestLocalization* GetManifestLocalizationPtrFromManifest(const VariantManifestPtr& v) { return &(variant_ptr<Manifest>(v)->DefaultLocalization); }

        ManifestInstaller* GetManifestInstallerPtr(const VariantManifestPtr& v)
        {
            if (auto installer = std::get_if<ManifestInstaller*>(&v))
            {
                return *installer;
            }

            return GetManifestInstallerPtrFromManifest(v);
        }

        ManifestLocalization* GetManifestLocalizationPtr(const VariantManifestPtr& v)
        {
            if (auto localization = std::get_if<ManifestLocalization*>(&v))
            {
                return *localization;
            }

            return GetManifestLocalizationPtrFromManifest(v);
        }

        // Only used in preview manifest
        std::vector<Manifest::string_t> SplitMultiValueField(const std::string& input)
        {
            if (input.empty())
            {
                return {};
            }

            std::vector<Manifest::string_t> result;
            size_t currentPos = 0;
            while (currentPos < input.size())
            {
                size_t splitPos = input.find(',', currentPos);
                if (splitPos == std::string::npos)
                {
                    splitPos = input.size();
                }

                std::string splitVal = input.substr(currentPos, splitPos - currentPos);
                Utility::Trim(splitVal);
                if (!splitVal.empty())
                {
                    result.emplace_back(std::move(splitVal));
                }
                currentPos = splitPos + 1;
            }

            return result;
        }

        std::vector<Manifest::string_t> ProcessStringSequenceNode(const YAML::Node& node, bool trim = true)
        {
            THROW_HR_IF(E_INVALIDARG, !node.IsSequence());

            std::vector<Manifest::string_t> result;

            for (auto const& entry : node.Sequence())
            {
                std::string value = entry.as<std::string>();
                if (trim)
                {
                    Utility::Trim(value);
                }

                result.emplace_back(std::move(value));
            }

            return result;
        }

        std::vector<DWORD> ProcessInstallerSuccessCodeSequenceNode(const YAML::Node& node)
        {
            THROW_HR_IF(E_INVALIDARG, !node.IsSequence());

            std::vector<DWORD> result;

            for (auto const& entry : node.Sequence())
            {
                result.emplace_back(static_cast<DWORD>(entry.as<int>()));
            }

            return result;
        }

        std::vector<PlatformEnum> ProcessPlatformSequenceNode(const YAML::Node& node)
        {
            THROW_HR_IF(E_INVALIDARG, !node.IsSequence());

            std::vector<PlatformEnum> result;

            for (auto const& entry : node.Sequence())
            {
                result.emplace_back(ConvertToPlatformEnum(entry.as<std::string>()));
            }

            return result;
        }

        std::vector<InstallModeEnum> ProcessInstallModeSequenceNode(const YAML::Node& node)
        {
            THROW_HR_IF(E_INVALIDARG, !node.IsSequence());

            std::vector<InstallModeEnum> result;

            for (auto const& entry : node.Sequence())
            {
                result.emplace_back(ConvertToInstallModeEnum(entry.as<std::string>()));
            }

            return result;
        }

        std::vector<AppInstaller::Utility::Architecture> ProcessArchitectureSequenceNode(const YAML::Node& node)
        {
            THROW_HR_IF(E_INVALIDARG, !node.IsSequence());

            std::vector<AppInstaller::Utility::Architecture> result;

            for (auto const& entry : node.Sequence())
            {
                result.emplace_back(Utility::ConvertToArchitectureEnum(entry.as<std::string>()));
            }

            return result;
        }

        std::vector<UnsupportedArgumentEnum> ProcessUnsupportedArgumentsSequenceNode(const YAML::Node& node)
        {
            THROW_HR_IF(E_INVALIDARG, !node.IsSequence());

            std::vector<UnsupportedArgumentEnum> result;

            for (auto const& entry : node.Sequence())
            {
                result.emplace_back(ConvertToUnsupportedArgumentEnum(entry.as<std::string>()));
            }

            return result;
        }

        void ProcessDependenciesNode(DependencyType type, const YAML::Node& node, DependencyList* dependencyList)
        {
            const auto& ids = ProcessStringSequenceNode(node);
            for (auto id : ids)
            {
                dependencyList->Add(Dependency(type, id));
            }
        }
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetRootFieldProcessInfo()
    {
        // Common fields across versions
        std::vector<FieldProcessInfo> result =
        {
            { "ManifestVersion", [](const YAML::Node&, const VariantManifestPtr&)->ValidationErrors { /* ManifestVersion already populated. Field listed here for duplicate and PascalCase check */ return {}; } },
            { "Installers", [this](const YAML::Node& value, const VariantManifestPtr&)->ValidationErrors { m_p_installersNode = &value; return {}; } },
            { "Localization", [this](const YAML::Node& value, const VariantManifestPtr&)->ValidationErrors { m_p_localizationsNode = &value; return {}; } },
            { "Channel", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Manifest>(v)->Channel = Utility::Trim(value.as<std::string>()); return {}; } },
        };

        // Additional version specific fields
        if (m_manifestVersion.get().Major() == 0)
        {
            std::vector<FieldProcessInfo> previewRootFields
            {
                { "Id", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Manifest>(v)->Id = Utility::Trim(value.as<std::string>()); return {}; } },
                { "Version", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Manifest>(v)->Version = Utility::Trim(value.as<std::string>()); return {}; } },
                { "AppMoniker", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Manifest>(v)->Moniker = Utility::Trim(value.as<std::string>()); return {}; } },
            };


            std::move(previewRootFields.begin(), previewRootFields.end(), std::inserter(result, result.end()));
        }
        else if (m_manifestVersion.get().Major() == 1)
        {
            // Starting v1, we should be only adding new fields for each minor version increase
            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1 })
            {
                std::vector<FieldProcessInfo> v1RootFields
                {
                    { "PackageIdentifier", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Manifest>(v)->Id = Utility::Trim(value.as<std::string>()); return {}; } },
                    { "PackageVersion", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Manifest>(v)->Version = Utility::Trim(value.as<std::string>()); return {}; } },
                    { "Moniker", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Manifest>(v)->Moniker = Utility::Trim(value.as<std::string>()); return {}; } },
                    { "ManifestType", [](const YAML::Node&, const VariantManifestPtr&)->ValidationErrors { /* ManifestType already checked. Field listed here for duplicate and PascalCase check */ return {}; } },
                };

                std::move(v1RootFields.begin(), v1RootFields.end(), std::inserter(result, result.end()));
            }
        }

        // Root fields mapped as Installer and Localization values
        auto rootInstallerFields = GetInstallerFieldProcessInfo(true);
        std::move(rootInstallerFields.begin(), rootInstallerFields.end(), std::inserter(result, result.end()));

        auto rootLocalizationFields = GetLocalizationFieldProcessInfo(true);
        std::move(rootLocalizationFields.begin(), rootLocalizationFields.end(), std::inserter(result, result.end()));

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetInstallerFieldProcessInfo(bool forRootFields)
    {
        // Common fields across versions
        std::vector<FieldProcessInfo> result =
        {
            { "InstallerType", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->BaseInstallerType = ConvertToInstallerTypeEnum(value.as<std::string>()); return {}; } },
            { "PackageFamilyName", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->PackageFamilyName = value.as<std::string>(); return {}; } },
            { "ProductCode", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->ProductCode = value.as<std::string>(); return {}; } },
        };

        // Additional version specific fields
        if (m_manifestVersion.get().Major() == 0)
        {
            // Root level and Localization node level
            std::vector<FieldProcessInfo> previewCommonFields =
            {
                { "UpdateBehavior", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->UpdateBehavior = ConvertToUpdateBehaviorEnum(value.as<std::string>()); return {}; } },
                { "Switches", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ValidateAndProcessFields(value, SwitchesFieldInfos, VariantManifestPtr(&(GetManifestInstallerPtr(v)->Switches))); }},
            };

            std::move(previewCommonFields.begin(), previewCommonFields.end(), std::inserter(result, result.end()));

            if (!forRootFields)
            {
                // Installer node only
                std::vector<FieldProcessInfo> installerOnlyFields =
                {
                    { "Arch", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestInstaller>(v)->Arch = Utility::ConvertToArchitectureEnum(value.as<std::string>()); return {}; } },
                    { "Url", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestInstaller>(v)->Url = value.as<std::string>(); return {}; } },
                    { "Sha256", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestInstaller>(v)->Sha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>()); return {}; } },
                    { "SignatureSha256", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestInstaller>(v)->SignatureSha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>()); return {}; } },
                    { "Language", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestInstaller>(v)->Locale = value.as<std::string>(); return {}; } },
                    { "Scope", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestInstaller>(v)->Scope = ConvertToScopeEnum(value.as<std::string>()); return {}; } },
                };

                if (m_manifestVersion.get().HasExtension(s_MSStoreExtension))
                {
                    installerOnlyFields.emplace_back("ProductId", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestInstaller>(v)->ProductId = value.as<std::string>(); return {}; });
                }

                std::move(installerOnlyFields.begin(), installerOnlyFields.end(), std::inserter(result, result.end()));
            }
            else
            {
                // Root node only
                std::vector<FieldProcessInfo> rootOnlyFields =
                {
                    { "MinOSVersion", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtrFromManifest(v)->MinOSVersion = value.as<std::string>(); return {}; } },
                    { "Commands", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtrFromManifest(v)->Commands = SplitMultiValueField(value.as<std::string>()); return {}; } },
                    { "Protocols", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtrFromManifest(v)->Protocols = SplitMultiValueField(value.as<std::string>()); return {}; } },
                    { "FileExtensions", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtrFromManifest(v)->FileExtensions = SplitMultiValueField(value.as<std::string>()); return {}; } },
                };

                std::move(rootOnlyFields.begin(), rootOnlyFields.end(), std::inserter(result, result.end()));
            }
        }
        else if (m_manifestVersion.get().Major() == 1)
        {
            // Starting v1, we should be only adding new fields for each minor version increase
            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1 })
            {
                // Root level and Installer node level
                std::vector<FieldProcessInfo> v1CommonFields =
                {
                    { "InstallerLocale", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->Locale = value.as<std::string>(); return {}; } },
                    { "Platform", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->Platform = ProcessPlatformSequenceNode(value); return {}; } },
                    { "MinimumOSVersion", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->MinOSVersion = value.as<std::string>(); return {}; } },
                    { "Scope", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->Scope = ConvertToScopeEnum(value.as<std::string>()); return {}; } },
                    { "InstallModes", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->InstallModes = ProcessInstallModeSequenceNode(value); return {}; } },
                    { "InstallerSwitches", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ValidateAndProcessFields(value, SwitchesFieldInfos, VariantManifestPtr(&(GetManifestInstallerPtr(v)->Switches))); }},
                    { "InstallerSuccessCodes", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->InstallerSuccessCodes = ProcessInstallerSuccessCodeSequenceNode(value); return {}; } },
                    { "UpgradeBehavior", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->UpdateBehavior = ConvertToUpdateBehaviorEnum(value.as<std::string>()); return {}; } },
                    { "Commands", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->Commands = ProcessStringSequenceNode(value); return {}; } },
                    { "Protocols", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->Protocols = ProcessStringSequenceNode(value); return {}; } },
                    { "FileExtensions", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->FileExtensions = ProcessStringSequenceNode(value); return {}; } },
                    { "Dependencies", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ValidateAndProcessFields(value, DependenciesFieldInfos, VariantManifestPtr(&(GetManifestInstallerPtr(v)->Dependencies))); }},
                    { "Capabilities", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->Capabilities = ProcessStringSequenceNode(value); return {}; } },
                    { "RestrictedCapabilities", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->RestrictedCapabilities = ProcessStringSequenceNode(value); return {}; } },
                };

                std::move(v1CommonFields.begin(), v1CommonFields.end(), std::inserter(result, result.end()));

                if (!forRootFields)
                {
                    // Installer level only fields
                    std::vector<FieldProcessInfo> v1InstallerFields =
                    {
                        { "Architecture", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestInstaller>(v)->Arch = Utility::ConvertToArchitectureEnum(value.as<std::string>()); return {}; } },
                        { "InstallerUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestInstaller>(v)->Url = value.as<std::string>(); return {}; } },
                        { "InstallerSha256", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestInstaller>(v)->Sha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>()); return {}; } },
                        { "SignatureSha256", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestInstaller>(v)->SignatureSha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>()); return {}; } },
                        // No custom validation needed at field populating time since we have semantic validation later to block msstore and productId from community repo.
                        { "MSStoreProductIdentifier", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestInstaller>(v)->ProductId = value.as<std::string>(); return {}; } },
                    };

                    std::move(v1InstallerFields.begin(), v1InstallerFields.end(), std::inserter(result, result.end()));
                }
            }

            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_1 })
            {
                std::vector<FieldProcessInfo> fields_v1_1 =
                {
                    { "InstallerAbortsTerminal", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->InstallerAbortsTerminal = value.as<bool>(); return {}; } },
                    { "InstallLocationRequired", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->InstallLocationRequired = value.as<bool>(); return {}; } },
                    { "RequireExplicitUpgrade", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->RequireExplicitUpgrade = value.as<bool>(); return {}; } },
                    { "ReleaseDate", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->ReleaseDate = Utility::Trim(value.as<std::string>()); return {}; } },
                    { "UnsupportedOSArchitectures", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->UnsupportedOSArchitectures = ProcessArchitectureSequenceNode(value); return {}; } },
                    { "ElevationRequirement", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->ElevationRequirement = ConvertToElevationRequirementEnum(value.as<std::string>()); return {}; } },
                    { "Markets", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ProcessMarketsNode(value, GetManifestInstallerPtr(v)); } },
                    { "AppsAndFeaturesEntries", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ProcessAppsAndFeaturesEntriesNode(value, GetManifestInstallerPtr(v)); } },
                    { "ExpectedReturnCodes", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ProcessExpectedReturnCodesNode(value, GetManifestInstallerPtr(v)); } },
                };

                std::move(fields_v1_1.begin(), fields_v1_1.end(), std::inserter(result, result.end()));
            }

            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_2 })
            {
                std::vector<FieldProcessInfo> fields_v1_2 =
                {
                    { "UnsupportedArguments", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->UnsupportedArguments = ProcessUnsupportedArgumentsSequenceNode(value); return {}; } },
                    { "DisplayInstallWarnings", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->DisplayInstallWarnings = value.as<bool>(); return {}; } },
                };

                std::move(fields_v1_2.begin(), fields_v1_2.end(), std::inserter(result, result.end()));
            }

            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_4 })
            {
                std::vector<FieldProcessInfo> fields_v1_4 =
                {
                    { "NestedInstallerType", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->NestedInstallerType = ConvertToInstallerTypeEnum(value.as<std::string>()); return {}; } },
                    { "NestedInstallerFiles", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ProcessNestedInstallerFilesNode(value, GetManifestInstallerPtr(v)); } },
                    { "InstallationMetadata", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ValidateAndProcessFields(value, InstallationMetadataFieldInfos, VariantManifestPtr(&(GetManifestInstallerPtr(v)->InstallationMetadata))); }},
                };

                std::move(fields_v1_4.begin(), fields_v1_4.end(), std::inserter(result, result.end()));
            }

            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_6 })
            {
                std::vector<FieldProcessInfo> fields_v1_6 =
                {
                    { "DownloadCommandProhibited", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->DownloadCommandProhibited = value.as<bool>(); return {}; }, true },
                };

                std::move(fields_v1_6.begin(), fields_v1_6.end(), std::inserter(result, result.end()));
            }

            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_7 })
            {
                std::vector<FieldProcessInfo> fields_v1_7 =
                {
                    { "RepairBehavior", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->RepairBehavior = ConvertToRepairBehaviorEnum(value.as<std::string>()); return {}; } },
                };

                std::move(fields_v1_7.begin(), fields_v1_7.end(), std::inserter(result, result.end()));
            }

            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_9 })
            {
                std::vector<FieldProcessInfo> fields_v1_9 =
                {
                    { "ArchiveBinariesDependOnPath", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->ArchiveBinariesDependOnPath = value.as<bool>(); return {}; } },
                };

                std::move(fields_v1_9.begin(), fields_v1_9.end(), std::inserter(result, result.end()));
            }

            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_10 })
            {
                std::vector<FieldProcessInfo> fields_v1_10 =
                {
                    { "Authentication", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestInstallerPtr(v)->AuthInfo = {}; auto errors = ValidateAndProcessFields(value, AuthenticationFieldInfos, VariantManifestPtr(&(GetManifestInstallerPtr(v)->AuthInfo))); GetManifestInstallerPtr(v)->AuthInfo.UpdateRequiredFieldsIfNecessary(); return errors; }, true},
                };

                std::move(fields_v1_10.begin(), fields_v1_10.end(), std::inserter(result, result.end()));
            }
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetSwitchesFieldProcessInfo()
    {
        // Common fields across versions
        std::vector<FieldProcessInfo> result =
        {
            { "Custom", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { (*variant_ptr<std::map<InstallerSwitchType, Utility::NormalizedString>>(v))[InstallerSwitchType::Custom] = value.as<std::string>(); return{}; } },
            { "Silent", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { (*variant_ptr<std::map<InstallerSwitchType, Utility::NormalizedString>>(v))[InstallerSwitchType::Silent] = value.as<std::string>(); return{}; } },
            { "SilentWithProgress", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { (*variant_ptr<std::map<InstallerSwitchType, Utility::NormalizedString>>(v))[InstallerSwitchType::SilentWithProgress] = value.as<std::string>(); return{}; } },
            { "Interactive", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { (*variant_ptr<std::map<InstallerSwitchType, Utility::NormalizedString>>(v))[InstallerSwitchType::Interactive] = value.as<std::string>(); return{}; } },
            { "Log", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { (*variant_ptr<std::map<InstallerSwitchType, Utility::NormalizedString>>(v))[InstallerSwitchType::Log] = value.as<std::string>(); return{}; } },
            { "InstallLocation", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { (*variant_ptr<std::map<InstallerSwitchType, Utility::NormalizedString>>(v))[InstallerSwitchType::InstallLocation] = value.as<std::string>(); return{}; } },
        };

        // Additional version specific fields
        if (m_manifestVersion.get().Major() == 0)
        {
            // Language only exists in preview manifests. Though we don't use it in our code yet, keep it here to be consistent with schema.
            result.emplace_back("Language", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { (*variant_ptr<std::map<InstallerSwitchType, Utility::NormalizedString>>(v))[InstallerSwitchType::Language] = value.as<std::string>(); return{}; });
            result.emplace_back("Update", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { (*variant_ptr<std::map<InstallerSwitchType, Utility::NormalizedString>>(v))[InstallerSwitchType::Update] = value.as<std::string>(); return{}; });
        }
        else if (m_manifestVersion.get().Major() == 1)
        {
            result.emplace_back("Upgrade", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { (*variant_ptr<std::map<InstallerSwitchType, Utility::NormalizedString>>(v))[InstallerSwitchType::Update] = value.as<std::string>(); return{}; });

            if (m_manifestVersion.get() >= ManifestVer{s_ManifestVersionV1_7})
            {
                result.emplace_back("Repair", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { (*variant_ptr<std::map<InstallerSwitchType, Utility::NormalizedString>>(v))[InstallerSwitchType::Repair] = value.as<std::string>(); return{}; });
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetExpectedReturnCodesFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_1 })
        {
            result.emplace_back("InstallerReturnCode", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ExpectedReturnCode>(v)->InstallerReturnCode = static_cast<int>(value.as<int>()); return {}; });
            result.emplace_back("ReturnResponse", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ExpectedReturnCode>(v)->ReturnResponse = ConvertToExpectedReturnCodeEnum(value.as<std::string>()); return {}; });
        }

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_2 })
        {
            result.emplace_back("ReturnResponseUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ExpectedReturnCode>(v)->ReturnResponseUrl = value.as<std::string>(); return {}; });
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetLocalizationFieldProcessInfo(bool forRootFields)
    {
        // Common fields across versions
        std::vector<FieldProcessInfo> result =
        {
            { "Description", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::Description>(Utility::Trim(value.as<std::string>())); return {}; } },
            { "LicenseUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::LicenseUrl>(value.as<std::string>()); return {}; } },
        };

        // Additional version specific fields
        if (m_manifestVersion.get().Major() == 0)
        {
            // Root level and Localization node level
            result.emplace_back("Homepage", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::PackageUrl>(value.as<std::string>()); return {}; });

            if (!forRootFields)
            {
                // Localization node only
                result.emplace_back("Language", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<ManifestLocalization>(v)->Locale = value.as<std::string>(); return {}; });
            }
            else
            {
                // Root node only
                std::vector<FieldProcessInfo> rootOnlyFields =
                {
                    { "Name", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtrFromManifest(v)->Add<Localization::PackageName>(Utility::Trim(value.as<std::string>())); return {}; } },
                    { "Publisher", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtrFromManifest(v)->Add<Localization::Publisher>(value.as<std::string>()); return {}; } },
                    { "Author", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtrFromManifest(v)->Add<Localization::Author>(value.as<std::string>()); return {}; } },
                    { "License", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtrFromManifest(v)->Add<Localization::License>(value.as<std::string>()); return {}; } },
                    { "Tags", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtrFromManifest(v)->Add<Localization::Tags>(SplitMultiValueField(value.as<std::string>())); return {}; } },
                };

                std::move(rootOnlyFields.begin(), rootOnlyFields.end(), std::inserter(result, result.end()));
            }
        }
        else if (m_manifestVersion.get().Major() == 1)
        {
            // Starting v1, we should be only adding new fields for each minor version increase
            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1 })
            {
                // Root level and Localization node level
                std::vector<FieldProcessInfo> v1CommonFields =
                {
                    { "PackageLocale", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Locale = value.as<std::string>(); return {}; } },
                    { "Publisher", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::Publisher>(value.as<std::string>()); return {}; } },
                    { "PublisherUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::PublisherUrl>(value.as<std::string>()); return {}; } },
                    { "PublisherSupportUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::PublisherSupportUrl>(value.as<std::string>()); return {}; } },
                    { "PrivacyUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::PrivacyUrl>(value.as<std::string>()); return {}; } },
                    { "Author", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::Author>(value.as<std::string>()); return {}; } },
                    { "PackageName", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::PackageName>(Utility::Trim(value.as<std::string>())); return {}; } },
                    { "PackageUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::PackageUrl>(value.as<std::string>()); return {}; } },
                    { "License", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::License>(value.as<std::string>()); return {}; } },
                    { "Copyright", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::Copyright>(value.as<std::string>()); return {}; } },
                    { "CopyrightUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::CopyrightUrl>(value.as<std::string>()); return {}; } },
                    { "ShortDescription", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::ShortDescription>(Utility::Trim(value.as<std::string>())); return {}; } },
                    { "Tags", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::Tags>(ProcessStringSequenceNode(value)); return {}; } },
                };

                std::move(v1CommonFields.begin(), v1CommonFields.end(), std::inserter(result, result.end()));
            }

            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_1 })
            {
                std::vector<FieldProcessInfo> fields_v1_1 =
                {
                    { "Agreements", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ProcessAgreementsNode(value, GetManifestLocalizationPtr(v)); } },
                    { "ReleaseNotes", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::ReleaseNotes>(value.as<std::string>()); return {}; } },
                    { "ReleaseNotesUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::ReleaseNotesUrl>(value.as<std::string>()); return {}; } },
                };

                std::move(fields_v1_1.begin(), fields_v1_1.end(), std::inserter(result, result.end()));
            }

            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_2 })
            {
                std::vector<FieldProcessInfo> fields_v1_2 =
                {
                    { "PurchaseUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::PurchaseUrl>(value.as<std::string>()); return {}; } },
                    { "InstallationNotes", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Add<Localization::InstallationNotes>(value.as<std::string>()); return {}; } },
                    { "Documentations", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ProcessDocumentationsNode(value, GetManifestLocalizationPtr(v)); } },
                };

                std::move(fields_v1_2.begin(), fields_v1_2.end(), std::inserter(result, result.end()));
            }

            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_5 })
            {
                std::vector<FieldProcessInfo> fields_v1_5 =
                {
                    { "Icons", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ProcessIconsNode(value, GetManifestLocalizationPtr(v)); }, true },
                };

                std::move(fields_v1_5.begin(), fields_v1_5.end(), std::inserter(result, result.end()));
            }
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetDependenciesFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1 })
        {
            result =
            {
                { "WindowsFeatures", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { ProcessDependenciesNode(DependencyType::WindowsFeature, value, variant_ptr<DependencyList>(v)); return {}; } },
                { "WindowsLibraries", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { ProcessDependenciesNode(DependencyType::WindowsLibrary, value, variant_ptr<DependencyList>(v)); return {}; } },
                { "PackageDependencies", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { ProcessPackageDependenciesNode(value, variant_ptr<DependencyList>(v)); return {}; } },
                { "ExternalDependencies", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { ProcessDependenciesNode(DependencyType::External, value, variant_ptr<DependencyList>(v)); return {}; } },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetPackageDependenciesFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1 })
        {
            result =
            {
                { "PackageIdentifier", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Dependency>(v)->SetId(Utility::Trim(value.as<std::string>())); return {}; } },
                { "MinimumVersion", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Dependency>(v)->MinVersion = Utility::Version(Utility::Trim(value.as<std::string>())); return {}; } },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetAgreementFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_1 })
        {
            result =
            {
                { "AgreementLabel", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Agreement>(v)->Label = Utility::Trim(value.as<std::string>()); return {}; } },
                { "Agreement", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Agreement>(v)->AgreementText = Utility::Trim(value.as<std::string>()); return {}; }, true },
                { "AgreementUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Agreement>(v)->AgreementUrl = Utility::Trim(value.as<std::string>()); return {}; } },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetMarketsFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_1 })
        {
            result =
            {
                { "AllowedMarkets", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<MarketsInfo>(v)->AllowedMarkets = ProcessStringSequenceNode(value); return {}; } },
                { "ExcludedMarkets", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<MarketsInfo>(v)->ExcludedMarkets = ProcessStringSequenceNode(value); return {}; } },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetAppsAndFeaturesEntryFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_1 })
        {
            result =
            {
                { "DisplayName", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<AppsAndFeaturesEntry>(v)->DisplayName = Utility::Trim(value.as<std::string>()); return {}; } },
                { "Publisher", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<AppsAndFeaturesEntry>(v)->Publisher = Utility::Trim(value.as<std::string>()); return {}; } },
                { "DisplayVersion", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<AppsAndFeaturesEntry>(v)->DisplayVersion = Utility::Trim(value.as<std::string>()); return {}; } },
                { "ProductCode", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<AppsAndFeaturesEntry>(v)->ProductCode = Utility::Trim(value.as<std::string>()); return {}; } },
                { "UpgradeCode", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<AppsAndFeaturesEntry>(v)->UpgradeCode = Utility::Trim(value.as<std::string>()); return {}; } },
                { "InstallerType", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<AppsAndFeaturesEntry>(v)->InstallerType = ConvertToInstallerTypeEnum(value.as<std::string>()); return {}; } },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetDocumentationFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_2 })
        {
            result =
            {
                { "DocumentLabel", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Documentation>(v)->DocumentLabel = Utility::Trim(value.as<std::string>()); return {}; } },
                { "DocumentUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Documentation>(v)->DocumentUrl = Utility::Trim(value.as<std::string>()); return {}; } },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetIconFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_5 })
        {
            result =
            {
                { "IconUrl", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Icon>(v)->Url = Utility::Trim(value.as<std::string>()); return {}; } },
                { "IconFileType", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Icon>(v)->FileType = ConvertToIconFileTypeEnum(value.as<std::string>()); return {}; } },
                { "IconResolution", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Icon>(v)->Resolution = ConvertToIconResolutionEnum(value.as<std::string>()); return {}; } },
                { "IconTheme", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Icon>(v)->Theme = ConvertToIconThemeEnum(value.as<std::string>()); return {}; } },
                { "IconSha256", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Icon>(v)->Sha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>()); return {}; } },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetNestedInstallerFileFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_4 })
        {
            result =
            {
                { "RelativeFilePath", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<NestedInstallerFile>(v)->RelativeFilePath = Utility::Trim(value.as<std::string>()); return {}; } },
                { "PortableCommandAlias", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<NestedInstallerFile>(v)->PortableCommandAlias = Utility::Trim(value.as<std::string>()); return {}; } },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetInstallationMetadataFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_4 })
        {
            result =
            {
                { "DefaultInstallLocation", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<InstallationMetadataInfo>(v)->DefaultInstallLocation = Utility::Trim(value.as<std::string>()); return {}; } },
                { "Files", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ProcessInstallationMetadataFilesNode(value, variant_ptr<InstallationMetadataInfo>(v)); } },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetInstallationMetadataFilesFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_4 })
        {
            result =
            {
                { "RelativeFilePath", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<InstalledFile>(v)->RelativeFilePath = Utility::Trim(value.as<std::string>()); return {}; } },
                { "FileSha256", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<InstalledFile>(v)->FileSha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>()); return {}; } },
                { "FileType", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<InstalledFile>(v)->FileType = ConvertToInstalledFileTypeEnum(value.as<std::string>()); return {}; } },
                { "InvocationParameter", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<InstalledFile>(v)->InvocationParameter = Utility::Trim(value.as<std::string>()); return {}; } },
                { "DisplayName", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<InstalledFile>(v)->DisplayName = Utility::Trim(value.as<std::string>()); return {}; } },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetAuthenticationFieldInfos()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_10 })
        {
            result =
            {
                { "AuthenticationType", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Authentication::AuthenticationInfo>(v)->Type = Authentication::ConvertToAuthenticationType(value.as<std::string>()); return {}; } },
                { "MicrosoftEntraIdAuthenticationInfo", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Authentication::AuthenticationInfo>(v)->MicrosoftEntraIdInfo.emplace(); return ValidateAndProcessFields(value, MicrosoftEntraIdAuthenticationInfoFieldInfos, VariantManifestPtr(&(variant_ptr<Authentication::AuthenticationInfo>(v)->MicrosoftEntraIdInfo.value()))); }},
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetMicrosoftEntraIdAuthenticationInfoFieldInfos()
    {
        std::vector<FieldProcessInfo> result = {};

        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_10 })
        {
            result =
            {
                { "Resource", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Authentication::MicrosoftEntraIdAuthenticationInfo>(v)->Resource = Utility::Trim(value.as<std::string>()); return {}; } },
                { "Scope", [](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { variant_ptr<Authentication::MicrosoftEntraIdAuthenticationInfo>(v)->Scope = Utility::Trim(value.as<std::string>()); return {}; } },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetShadowRootFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result;

        if (m_manifestVersion.get().Major() == 1)
        {
            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_5 })
            {
                std::vector<FieldProcessInfo> fields_v1_5 =
                {
                    {
                        { "Localization", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ProcessShadowLocalizationNode(value, variant_ptr<Manifest>(v)); } },
                        { "ManifestType", [](const YAML::Node&, const VariantManifestPtr&)->ValidationErrors { return {}; } },
                        { "PackageIdentifier", [](const YAML::Node&, const VariantManifestPtr&)->ValidationErrors { return {}; } },
                        { "PackageVersion", [](const YAML::Node&, const VariantManifestPtr&)->ValidationErrors { return {}; } },
                        { "ManifestVersion", [](const YAML::Node&, const VariantManifestPtr&)->ValidationErrors { return {}; } },
                    },
                };

                std::move(fields_v1_5.begin(), fields_v1_5.end(), std::inserter(result, result.end()));
            }
        }

        auto rootLocalizationFields = GetShadowLocalizationFieldProcessInfo();
        std::move(rootLocalizationFields.begin(), rootLocalizationFields.end(), std::inserter(result, result.end()));

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetShadowLocalizationFieldProcessInfo()
    {
        std::vector<FieldProcessInfo> result;

        if (m_manifestVersion.get().Major() == 1)
        {
            if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_5 })
            {
                std::vector<FieldProcessInfo> fields_v1_5 =
                {
                    { "PackageLocale", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { GetManifestLocalizationPtr(v)->Locale = value.as<std::string>(); return {}; } },
                    { "Icons", [this](const YAML::Node& value, const VariantManifestPtr& v)->ValidationErrors { return ProcessIconsNode(value, GetManifestLocalizationPtr(v)); } },
                };

                std::move(fields_v1_5.begin(), fields_v1_5.end(), std::inserter(result, result.end()));
            }
        }

        return result;
    }

    ValidationErrors ManifestYamlPopulator::ValidateAndProcessFields(
        const YAML::Node& rootNode,
        const std::vector<FieldProcessInfo>& fieldInfos,
        const VariantManifestPtr& v)
    {
        ValidationErrors resultErrors;

        if (!rootNode.IsMap() || rootNode.size() == 0)
        {
            resultErrors.emplace_back(ManifestError::InvalidRootNode, "", "", m_isMergedManifest ? 0 : rootNode.Mark().line, m_isMergedManifest ? 0 : rootNode.Mark().column);
            return resultErrors;
        }

        // Keeps track of already processed fields. Used to check duplicate fields.
        std::set<std::string> processedFields;

        for (auto const& keyValuePair : rootNode.Mapping())
        {
            std::string key = keyValuePair.first.as<std::string>();
            const YAML::Node& valueNode = keyValuePair.second;

            // We'll do case-insensitive search first and validate correct case later.
            auto fieldIter = std::find_if(fieldInfos.begin(), fieldInfos.end(),
                [&](auto const& s)
                {
                    return Utility::CaseInsensitiveEquals(s.Name, key);
                });

            if (fieldIter != fieldInfos.end())
            {
                const FieldProcessInfo& fieldInfo = *fieldIter;

                // Make sure the found key is in Pascal Case
                if (key != fieldInfo.Name)
                {
                    resultErrors.emplace_back(ManifestError::FieldIsNotPascalCase, key, "", m_isMergedManifest ? 0 : keyValuePair.first.Mark().line, m_isMergedManifest ? 0 : keyValuePair.first.Mark().column);
                }

                // Make sure it's not a duplicate key
                if (!processedFields.insert(fieldInfo.Name).second)
                {
                    resultErrors.emplace_back(ManifestError::FieldDuplicate, fieldInfo.Name, "", m_isMergedManifest ? 0 : keyValuePair.first.Mark().line, m_isMergedManifest ? 0 : keyValuePair.first.Mark().column);
                }

                if (fieldInfo.RequireVerifiedPublisher)
                {
                    resultErrors.emplace_back(ManifestError::FieldRequireVerifiedPublisher, fieldInfo.Name, "",
                        m_isMergedManifest ? 0 : keyValuePair.first.Mark().line, m_isMergedManifest ? 0 : keyValuePair.first.Mark().column,
                        m_validateOption.ErrorOnVerifiedPublisherFields ? ValidationError::Level::Error : ValidationError::Level::Warning);
                }

                if (!valueNode.IsNull())
                {
                    try
                    {
                        auto errors = fieldInfo.ProcessFunc(valueNode, v);
                        std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
                    }
                    catch (const std::exception&)
                    {
                        resultErrors.emplace_back(ManifestError::FieldFailedToProcess, fieldInfo.Name);
                    }
                }
            }
            else
            {
                // For full validation, also reports unrecognized fields as warning
                if (m_validateOption.FullValidation)
                {
                    resultErrors.emplace_back(ManifestError::FieldUnknown, key, "", m_isMergedManifest ? 0 : keyValuePair.first.Mark().line, m_isMergedManifest ? 0 : keyValuePair.first.Mark().column, ValidationError::Level::Warning);
                }
            }
        }

        return resultErrors;
    }

    ValidationErrors ManifestYamlPopulator::ProcessPackageDependenciesNode(const YAML::Node& rootNode, DependencyList* dependencyList)
    {
        ValidationErrors resultErrors;

        for (auto const& entry : rootNode.Sequence())
        {
            Dependency packageDependency = Dependency(DependencyType::Package);
            auto errors = ValidateAndProcessFields(entry, PackageDependenciesFieldInfos, VariantManifestPtr(&packageDependency));
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            dependencyList->Add(std::move(packageDependency));
        }

        return resultErrors;
    }

    ValidationErrors ManifestYamlPopulator::ProcessAgreementsNode(const YAML::Node& agreementsNode, ManifestLocalization* localization)
    {
        THROW_HR_IF(E_INVALIDARG, !agreementsNode.IsSequence());

        ValidationErrors resultErrors;
        std::vector<Agreement> agreements;

        for (auto const& entry : agreementsNode.Sequence())
        {
            Agreement agreement;
            auto errors = ValidateAndProcessFields(entry, AgreementFieldInfos, VariantManifestPtr(&agreement));
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            agreements.emplace_back(std::move(agreement));
        }

        if (!agreements.empty())
        {
            localization->Add<Localization::Agreements>(std::move(agreements));
        }

        return resultErrors;
    }

    std::vector<ValidationError> ManifestYamlPopulator::ProcessMarketsNode(const YAML::Node& marketsNode, ManifestInstaller* installer)
    {
        MarketsInfo markets;
        auto errors = ValidateAndProcessFields(marketsNode, MarketsFieldInfos, VariantManifestPtr(&markets));
        installer->Markets = markets;
        return errors;
    }

    std::vector<ValidationError> ManifestYamlPopulator::ProcessAppsAndFeaturesEntriesNode(const YAML::Node& appsAndFeaturesEntriesNode, ManifestInstaller* installer)
    {
        THROW_HR_IF(E_INVALIDARG, !appsAndFeaturesEntriesNode.IsSequence());

        ValidationErrors resultErrors;
        std::vector<AppsAndFeaturesEntry> appsAndFeaturesEntries;

        for (auto const& entry : appsAndFeaturesEntriesNode.Sequence())
        {
            AppsAndFeaturesEntry appsAndFeaturesEntry;
            auto errors = ValidateAndProcessFields(entry, AppsAndFeaturesEntryFieldInfos, VariantManifestPtr(&appsAndFeaturesEntry));
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            appsAndFeaturesEntries.emplace_back(std::move(appsAndFeaturesEntry));
        }

        installer->AppsAndFeaturesEntries = appsAndFeaturesEntries;

        return resultErrors;
    }

    ValidationErrors ManifestYamlPopulator::ProcessExpectedReturnCodesNode(const YAML::Node& returnCodesNode, ManifestInstaller* installer)
    {
        THROW_HR_IF(E_INVALIDARG, !returnCodesNode.IsSequence());

        ValidationErrors resultErrors;
        std::map<DWORD, ExpectedReturnCodeInfo> returnCodes;

        for (auto const& entry : returnCodesNode.Sequence())
        {
            ExpectedReturnCode returnCode;
            auto errors = ValidateAndProcessFields(entry, ExpectedReturnCodesFieldInfos, VariantManifestPtr(&returnCode));
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            if (!returnCodes.insert({ returnCode.InstallerReturnCode, {returnCode.ReturnResponse, returnCode.ReturnResponseUrl} }).second)
            {
                resultErrors.emplace_back(ManifestError::DuplicateReturnCodeEntry);
            }
        }

        installer->ExpectedReturnCodes = returnCodes;

        return resultErrors;
    }

    ValidationErrors ManifestYamlPopulator::ProcessDocumentationsNode(const YAML::Node& documentationsNode, ManifestLocalization* localization)
    {
        THROW_HR_IF(E_INVALIDARG, !documentationsNode.IsSequence());

        ValidationErrors resultErrors;
        std::vector<Documentation> documentations;

        for (auto const& entry : documentationsNode.Sequence())
        {
            Documentation documentation;
            auto errors = ValidateAndProcessFields(entry, DocumentationFieldInfos, VariantManifestPtr(&documentation));
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            documentations.emplace_back(std::move(documentation));
        }

        if (!documentations.empty())
        {
            localization->Add<Localization::Documentations>(std::move(documentations));
        }

        return resultErrors;
    }

    std::vector<ValidationError> ManifestYamlPopulator::ProcessIconsNode(const YAML::Node& iconsNode, ManifestLocalization* localization)
    {
        THROW_HR_IF(E_INVALIDARG, !iconsNode.IsSequence());

        ValidationErrors resultErrors;
        std::vector<Icon> icons;

        for (auto const& entry : iconsNode.Sequence())
        {
            Icon icon;
            auto errors = ValidateAndProcessFields(entry, IconFieldInfos, VariantManifestPtr(&icon));
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            icons.emplace_back(std::move(icon));
        }

        if (!icons.empty())
        {
            localization->Add<Localization::Icons>(std::move(icons));
        }

        return resultErrors;
    }

    ValidationErrors ManifestYamlPopulator::ProcessNestedInstallerFilesNode(const YAML::Node& nestedInstallerFilesNode, ManifestInstaller* installer)
    {
        THROW_HR_IF(E_INVALIDARG, !nestedInstallerFilesNode.IsSequence());

        ValidationErrors resultErrors;
        std::vector<NestedInstallerFile> nestedInstallerFiles;

        for (auto const& entry : nestedInstallerFilesNode.Sequence())
        {
            NestedInstallerFile nestedInstallerFile;
            auto errors = ValidateAndProcessFields(entry, NestedInstallerFileFieldInfos, VariantManifestPtr(&nestedInstallerFile));
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            nestedInstallerFiles.emplace_back(std::move(nestedInstallerFile));
        }

        if (!nestedInstallerFiles.empty())
        {
            installer->NestedInstallerFiles = nestedInstallerFiles;
        }

        return resultErrors;
    }

    std::vector<ValidationError> ManifestYamlPopulator::ProcessInstallationMetadataFilesNode(const YAML::Node& installedFilesNode, InstallationMetadataInfo* installationMetadata)
    {
        THROW_HR_IF(E_INVALIDARG, !installedFilesNode.IsSequence());

        ValidationErrors resultErrors;
        std::vector<InstalledFile> installedFiles;

        for (auto const& entry : installedFilesNode.Sequence())
        {
            InstalledFile installedFile;
            auto errors = ValidateAndProcessFields(entry, InstallationMetadataFilesFieldInfos, VariantManifestPtr(&installedFile));
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            installedFiles.emplace_back(std::move(installedFile));
        }

        if (!installedFiles.empty())
        {
            installationMetadata->Files = installedFiles;
        }

        return resultErrors;
    }

    std::vector<ValidationError> ManifestYamlPopulator::ProcessShadowLocalizationNode(const YAML::Node& localizationNode, Manifest* manifest)
    {
        THROW_HR_IF(E_INVALIDARG, !localizationNode.IsSequence());

        ValidationErrors resultErrors;
        auto shadowLocalizationFields = GetShadowLocalizationFieldProcessInfo();

        for (auto const& entry : localizationNode.Sequence())
        {
            ManifestLocalization localization;
            auto errors = ValidateAndProcessFields(entry, shadowLocalizationFields, VariantManifestPtr(&localization));
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            manifest->Localizations.emplace_back(std::move(std::move(localization)));
        }

        return resultErrors;
    }

    ManifestYamlPopulator::ManifestYamlPopulator(YAML::Node& rootNode, Manifest& manifest, const ManifestVer& manifestVersion, ManifestValidateOption validateOption) :
        m_rootNode(rootNode), m_manifest(manifest), m_manifestVersion(manifestVersion), m_validateOption(validateOption)
    {
        m_isMergedManifest = !m_rootNode.get()["ManifestType"sv].IsNull() && m_rootNode.get()["ManifestType"sv].as<std::string>() == "merged";
        m_manifest.get().ManifestVersion = m_manifestVersion;
    }

    ValidationErrors ManifestYamlPopulator::PopulateManifestInternal()
    {
        const YAML::Node& rootNode = m_rootNode;
        ValidationErrors resultErrors;

        // Prepare field infos
        RootFieldInfos = GetRootFieldProcessInfo();
        InstallerFieldInfos = GetInstallerFieldProcessInfo();
        SwitchesFieldInfos = GetSwitchesFieldProcessInfo();
        ExpectedReturnCodesFieldInfos = GetExpectedReturnCodesFieldProcessInfo();
        DependenciesFieldInfos = GetDependenciesFieldProcessInfo();
        PackageDependenciesFieldInfos = GetPackageDependenciesFieldProcessInfo();
        LocalizationFieldInfos = GetLocalizationFieldProcessInfo();
        AgreementFieldInfos = GetAgreementFieldProcessInfo();
        MarketsFieldInfos = GetMarketsFieldProcessInfo();
        AppsAndFeaturesEntryFieldInfos = GetAppsAndFeaturesEntryFieldProcessInfo();
        DocumentationFieldInfos = GetDocumentationFieldProcessInfo();
        IconFieldInfos = GetIconFieldProcessInfo();
        NestedInstallerFileFieldInfos = GetNestedInstallerFileFieldProcessInfo();
        InstallationMetadataFieldInfos = GetInstallationMetadataFieldProcessInfo();
        InstallationMetadataFilesFieldInfos = GetInstallationMetadataFilesFieldProcessInfo();
        AuthenticationFieldInfos = GetAuthenticationFieldInfos();
        MicrosoftEntraIdAuthenticationInfoFieldInfos = GetMicrosoftEntraIdAuthenticationInfoFieldInfos();

        resultErrors = ValidateAndProcessFields(rootNode, RootFieldInfos, VariantManifestPtr(&(m_manifest.get())));

        if (!m_p_installersNode)
        {
            return resultErrors;
        }

        // Populate installers
        for (auto const& entry : m_p_installersNode->Sequence())
        {
            ManifestInstaller installer = m_manifest.get().DefaultInstallerInfo;

            // Clear these defaults as PackageFamilyName, ProductCode, AppsAndFeaturesEntries need to be copied based on InstallerType
            installer.PackageFamilyName.clear();
            installer.ProductCode.clear();
            installer.AppsAndFeaturesEntries.clear();
            // Clear dependencies as installer overrides root dependencies
            installer.Dependencies.Clear();
            // Clear nested installers as it should only be copied for zip installerType.
            installer.NestedInstallerType = InstallerTypeEnum::Unknown;
            installer.NestedInstallerFiles.clear();

            auto errors = ValidateAndProcessFields(entry, InstallerFieldInfos, VariantManifestPtr(&installer));
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));

            // Copy in system reference strings from the root if not set in the installer and appropriate
            if (installer.PackageFamilyName.empty() && DoesInstallerTypeUsePackageFamilyName(installer.EffectiveInstallerType()))
            {
                installer.PackageFamilyName = m_manifest.get().DefaultInstallerInfo.PackageFamilyName;
            }

            if (installer.ProductCode.empty() && DoesInstallerTypeUseProductCode(installer.EffectiveInstallerType()))
            {
                installer.ProductCode = m_manifest.get().DefaultInstallerInfo.ProductCode;
            }

            if (installer.AppsAndFeaturesEntries.empty() && DoesInstallerTypeWriteAppsAndFeaturesEntry(installer.EffectiveInstallerType()))
            {
                installer.AppsAndFeaturesEntries = m_manifest.get().DefaultInstallerInfo.AppsAndFeaturesEntries;
            }

            if (IsArchiveType(installer.BaseInstallerType))
            {
                if (installer.NestedInstallerFiles.empty())
                {
                    installer.NestedInstallerFiles = m_manifest.get().DefaultInstallerInfo.NestedInstallerFiles;
                }

                if (installer.NestedInstallerType == InstallerTypeEnum::Unknown)
                {
                    installer.NestedInstallerType = m_manifest.get().DefaultInstallerInfo.NestedInstallerType;
                }
            }

            // If there are no dependencies on installer use default ones
            if (!installer.Dependencies.HasAny())
            {
                installer.Dependencies = m_manifest.get().DefaultInstallerInfo.Dependencies;
            }

            // Populate installer default switches if not exists
            auto defaultSwitches = GetDefaultKnownSwitches(installer.EffectiveInstallerType());
            for (auto const& defaultSwitch : defaultSwitches)
            {
                if (installer.Switches.find(defaultSwitch.first) == installer.Switches.end())
                {
                    installer.Switches[defaultSwitch.first] = defaultSwitch.second;
                }
            }

            // Populate installer default return codes if not present in ExpectedReturnCodes and InstallerSuccessCodes
            auto defaultReturnCodes = GetDefaultKnownReturnCodes(installer.EffectiveInstallerType());
            for (auto const& defaultReturnCode : defaultReturnCodes)
            {
                if (installer.ExpectedReturnCodes.find(defaultReturnCode.first) == installer.ExpectedReturnCodes.end() &&
                    std::find(installer.InstallerSuccessCodes.begin(), installer.InstallerSuccessCodes.end(), defaultReturnCode.first) == installer.InstallerSuccessCodes.end())
                {
                    installer.ExpectedReturnCodes[defaultReturnCode.first].ReturnResponseEnum = defaultReturnCode.second;
                }
            }

            m_manifest.get().Installers.emplace_back(std::move(installer));
        }

        // Populate additional localizations
        if (m_p_localizationsNode && m_p_localizationsNode->IsSequence())
        {
            for (auto const& entry : m_p_localizationsNode->Sequence())
            {
                ManifestLocalization localization;
                auto errors = ValidateAndProcessFields(entry, LocalizationFieldInfos, VariantManifestPtr(&localization));
                std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
                m_manifest.get().Localizations.emplace_back(std::move(std::move(localization)));
            }
        }

        return resultErrors;
    }

    ValidationErrors ManifestYamlPopulator::InsertShadow(const YAML::Node& shadowNode)
    {
        Manifest shadowManifest;

        // Process shadow node.
        auto resultErrors = ValidateAndProcessFields(shadowNode, GetShadowRootFieldProcessInfo(), VariantManifestPtr(&shadowManifest));

        // Merge.
        if (m_manifestVersion.get() >= ManifestVer{ s_ManifestVersionV1_5 })
        {
            // Default localization
            if (Utility::ICUCaseInsensitiveEquals(m_manifest.get().DefaultLocalization.Locale, shadowManifest.DefaultLocalization.Locale))
            {
                // Icons
                if (!m_manifest.get().DefaultLocalization.Contains(Localization::Icons) &&
                    shadowManifest.DefaultLocalization.Contains(Localization::Icons))
                {
                    m_manifest.get().DefaultLocalization.Add<Localization::Icons>(std::move(shadowManifest.DefaultLocalization.Get<Localization::Icons>()));

                    YAML::Node key{ YAML::Node::Type::Scalar, "", YAML::Mark() };
                    key.SetScalar("Icons");
                    YAML::Node value = shadowNode.GetChildNode("Icons");
                    m_rootNode.get().AddMappingNode(std::move(key), std::move(value));
                }
            }

            // Localizations
            if (!shadowManifest.Localizations.empty())
            {
                // Merge manifest object
                for (auto const& shadowLocalization : shadowManifest.Localizations)
                {
                    // Manifest
                    if (auto iter = std::find_if(m_manifest.get().Localizations.begin(), m_manifest.get().Localizations.end(), [&](auto const& l) { return Utility::ICUCaseInsensitiveEquals(l.Locale, shadowLocalization.Locale); }); iter != m_manifest.get().Localizations.end())
                    {
                        if (!(*iter).Contains(Localization::Icons) &&
                            shadowLocalization.Contains(Localization::Icons))
                        {
                            (*iter).Add<Localization::Icons>(std::move(shadowLocalization.Get<Localization::Icons>()));
                        }
                    }
                    else
                    {
                        ManifestLocalization localization = shadowLocalization;
                        m_manifest.get().Localizations.emplace_back(std::move(std::move(localization)));
                    }
                }

                // Merge yaml
                auto shadowLocalizationsNode = shadowNode.GetChildNode("Localization");
                if (m_p_localizationsNode)
                {
                    m_rootNode.get().GetChildNode("Localization").MergeSequenceNode(shadowLocalizationsNode, "PackageLocale", true);
                }
                else
                {
                    YAML::Node key{ YAML::Node::Type::Scalar, "", YAML::Mark() };
                    key.SetScalar("Localization");
                    m_rootNode.get().AddMappingNode(std::move(key), std::move(shadowLocalizationsNode));
                }
            }
        }

        return resultErrors;
    }

    ValidationErrors ManifestYamlPopulator::PopulateManifest(
        YAML::Node& rootNode,
        Manifest& manifest,
        const ManifestVer& manifestVersion,
        ManifestValidateOption validateOption,
        const std::optional<YAML::Node>& shadowNode)
    {
        ManifestYamlPopulator manifestPopulator(rootNode, manifest, manifestVersion, validateOption);
        auto errors = manifestPopulator.PopulateManifestInternal();

        if (shadowNode.has_value())
        {
            auto shadowErrors = manifestPopulator.InsertShadow(shadowNode.value());
            std::move(shadowErrors.begin(), shadowErrors.end(), std::inserter(errors, errors.end()));
        }

        return errors;
    }
}
