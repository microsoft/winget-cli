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
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetRootFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        // Common fields across versions
        std::vector<FieldProcessInfo> result =
        {
            { "ManifestVersion", [](const YAML::Node&, std::any&)->ValidationErrors { /* ManifestVersion already populated. Field listed here for duplicate and PascalCase check */ return {}; } },
            { "Installers", [this](const YAML::Node& value, std::any&)->ValidationErrors { m_p_installersNode = &value; return {}; } },
            { "Localization", [this](const YAML::Node& value, std::any&)->ValidationErrors { m_p_localizationsNode = &value; return {}; } },
            {
                "Channel",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    Manifest* manifest = std::any_cast<Manifest*>(any);
                    manifest->Channel = Utility::Trim(value.as<std::string>());
                    return {};
                }
            },
        };

        // Additional version specific fields
        if (manifestVersion.Major() == 0)
        {
            std::vector<FieldProcessInfo> previewRootFields
            {
                {
                    "Id",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Manifest* manifest = std::any_cast<Manifest*>(any);
                        manifest->Id = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "Version",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Manifest* manifest = std::any_cast<Manifest*>(any);
                        manifest->Version = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "AppMoniker",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Manifest* manifest = std::any_cast<Manifest*>(any);
                        manifest->Moniker = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
            };

            std::move(previewRootFields.begin(), previewRootFields.end(), std::inserter(result, result.end()));
        }
        else if (manifestVersion.Major() == 1)
        {
            // Starting v1, we should be only adding new fields for each minor version increase
            if (manifestVersion >= ManifestVer{ s_ManifestVersionV1 })
            {
                std::vector<FieldProcessInfo> v1RootFields
                {
                    {
                        "PackageIdentifier",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            manifest->Id = Utility::Trim(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "PackageVersion",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            manifest->Version = Utility::Trim(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "Moniker",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            manifest->Moniker = Utility::Trim(value.as<std::string>());
                            return {};
                        }
                    },
                    { "ManifestType", [](const YAML::Node&, std::any&)->ValidationErrors { /* ManifestType already checked. Field listed here for duplicate and PascalCase check */ return {}; } },
                };

                std::move(v1RootFields.begin(), v1RootFields.end(), std::inserter(result, result.end()));
            }
        }

        // Root fields mapped as Installer and Localization values
        auto rootInstallerFields = GetInstallerFieldProcessInfo(manifestVersion, true);
        std::move(rootInstallerFields.begin(), rootInstallerFields.end(), std::inserter(result, result.end()));

        auto rootLocalizationFields = GetLocalizationFieldProcessInfo(manifestVersion, true);
        std::move(rootLocalizationFields.begin(), rootLocalizationFields.end(), std::inserter(result, result.end()));

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetInstallerFieldProcessInfo(const ManifestVer& manifestVersion, bool forRootFields)
    {
        // Common fields across versions
        std::vector<FieldProcessInfo> result =
        {
            {
                "InstallerType",
                [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    ManifestInstaller* installer;
                    if (forRootFields)
                    {
                        Manifest* manifest = std::any_cast<Manifest*>(any);
                        installer = &(manifest->DefaultInstallerInfo);
                    }
                    else
                    {
                        installer = std::any_cast<ManifestInstaller*>(any);
                    }

                    installer->BaseInstallerType = ConvertToInstallerTypeEnum(value.as<std::string>());
                    return {};
                }
            },
            {
                "PackageFamilyName",
                [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    ManifestInstaller* installer;
                    if (forRootFields)
                    {
                        Manifest* manifest = std::any_cast<Manifest*>(any);
                        installer = &(manifest->DefaultInstallerInfo);
                    }
                    else
                    {
                        installer = std::any_cast<ManifestInstaller*>(any);
                    }

                    installer->PackageFamilyName = value.as<std::string>();
                    return {};
                }
            },
            {
                "ProductCode",
                [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    ManifestInstaller* installer;
                    if (forRootFields)
                    {
                        Manifest* manifest = std::any_cast<Manifest*>(any);
                        installer = &(manifest->DefaultInstallerInfo);
                    }
                    else
                    {
                        installer = std::any_cast<ManifestInstaller*>(any);
                    }

                    installer->ProductCode = value.as<std::string>();
                    return {};
                }
            },
        };

        // Additional version specific fields
        if (manifestVersion.Major() == 0)
        {
            // Root level and Localization node level
            std::vector<FieldProcessInfo> previewCommonFields =
            {
                {
                    "UpdateBehavior",
                    [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        ManifestInstaller* installer;
                        if (forRootFields)
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            installer = &(manifest->DefaultInstallerInfo);
                        }
                        else
                        {
                            installer = std::any_cast<ManifestInstaller*>(any);
                        }

                        installer->UpdateBehavior = ConvertToUpdateBehaviorEnum(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "Switches",
                    [this, forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        ManifestInstaller* installer;
                        if (forRootFields)
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            installer = &(manifest->DefaultInstallerInfo);
                        }
                        else
                        {
                            installer = std::any_cast<ManifestInstaller*>(any);
                        }

                        std::any anySwitches = &(installer->Switches);
                        return ValidateAndProcessFields(value, SwitchesFieldInfos, anySwitches);
                    }
                },
            };

            std::move(previewCommonFields.begin(), previewCommonFields.end(), std::inserter(result, result.end()));

            if (!forRootFields)
            {
                // Installer node only
                std::vector<FieldProcessInfo> installerOnlyFields =
                {
                    {
                        "Arch",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer = std::any_cast<ManifestInstaller*>(any);
                            installer->Arch = Utility::ConvertToArchitectureEnum(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "Url",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer = std::any_cast<ManifestInstaller*>(any);
                            installer->Url = value.as<std::string>();
                            return {};
                        }
                    },
                    {
                        "Sha256",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer = std::any_cast<ManifestInstaller*>(any);
                            installer->Sha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "SignatureSha256",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer = std::any_cast<ManifestInstaller*>(any);
                            installer->SignatureSha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "Language",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer = std::any_cast<ManifestInstaller*>(any);
                            installer->Locale = value.as<std::string>();
                            return {};
                        }
                    },
                    {
                        "Scope",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer = std::any_cast<ManifestInstaller*>(any);
                            installer->Scope = ConvertToScopeEnum(value.as<std::string>());
                            return {};
                        }
                    },
                };

                if (manifestVersion.HasExtension(s_MSStoreExtension))
                {
                    installerOnlyFields.emplace_back(
                        "ProductId",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer = std::any_cast<ManifestInstaller*>(any);
                            installer->ProductId = value.as<std::string>();
                            return {};
                        });
                }

                std::move(installerOnlyFields.begin(), installerOnlyFields.end(), std::inserter(result, result.end()));
            }
            else
            {
                // Root node only
                std::vector<FieldProcessInfo> rootOnlyFields =
                {
                    {
                        "MinOSVersion",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            ManifestInstaller* installer = &(manifest->DefaultInstallerInfo);
                            installer->MinOSVersion = value.as<std::string>();
                            return {};
                        }
                    },
                    {
                        "Commands",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            ManifestInstaller* installer = &(manifest->DefaultInstallerInfo);
                            installer->Commands = SplitMultiValueField(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "Protocols",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            ManifestInstaller* installer = &(manifest->DefaultInstallerInfo);
                            installer->Protocols = SplitMultiValueField(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "FileExtensions",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            ManifestInstaller* installer = &(manifest->DefaultInstallerInfo);
                            installer->FileExtensions = SplitMultiValueField(value.as<std::string>());
                            return {};
                        }
                    },
                };

                std::move(rootOnlyFields.begin(), rootOnlyFields.end(), std::inserter(result, result.end()));
            }
        }
        else if (manifestVersion.Major() == 1)
        {
            // Starting v1, we should be only adding new fields for each minor version increase
            if (manifestVersion >= ManifestVer{ s_ManifestVersionV1 })
            {
                // Root level and Installer node level
                std::vector<FieldProcessInfo> v1CommonFields =
                {
                    {
                        "InstallerLocale",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->Locale = value.as<std::string>();
                            return {};
                        }
                    },
                    {
                        "Platform",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->Platform = ProcessPlatformSequenceNode(value);
                            return {};
                        }
                    },
                    {
                        "MinimumOSVersion",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->MinOSVersion = value.as<std::string>();
                            return {};
                        }
                    },
                    {
                        "Scope",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->Scope = ConvertToScopeEnum(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "InstallModes",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->InstallModes = ProcessInstallModeSequenceNode(value);
                            return {};
                        }
                    },
                    {
                        "InstallerSwitches",
                        [this, forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            std::any anySwitches = &(installer->Switches);
                            return ValidateAndProcessFields(value, SwitchesFieldInfos, anySwitches);
                        }
                    },
                    {
                        "InstallerSuccessCodes",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->InstallerSuccessCodes = ProcessInstallerSuccessCodeSequenceNode(value);
                            return {};
                        }
                    },
                    {
                        "UpgradeBehavior",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->UpdateBehavior = ConvertToUpdateBehaviorEnum(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "Commands",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->Commands = ProcessStringSequenceNode(value);
                            return {};
                        }
                    },
                    {
                        "Protocols",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->Protocols = ProcessStringSequenceNode(value);
                            return {};
                        }
                    },
                    {
                        "FileExtensions",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->FileExtensions = ProcessStringSequenceNode(value);
                            return {};
                        }
                    },
                    {
                        "Dependencies",
                        [this, forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            std::any anyDependencyList = &(installer->Dependencies);
                            return ValidateAndProcessFields(value, DependenciesFieldInfos, anyDependencyList);
                        }
                    },
                    {
                        "Capabilities",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->Capabilities = ProcessStringSequenceNode(value);
                            return {};
                        }
                    },
                    {
                        "RestrictedCapabilities",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->RestrictedCapabilities = ProcessStringSequenceNode(value);
                            return {};
                        }
                    },
                };

                std::move(v1CommonFields.begin(), v1CommonFields.end(), std::inserter(result, result.end()));

                if (!forRootFields)
                {
                    // Installer level only fields
                    std::vector<FieldProcessInfo> v1InstallerFields =
                    {
                        {
                            "Architecture",
                            [](const YAML::Node& value, std::any& any)->ValidationErrors
                            {
                                ManifestInstaller* installer = std::any_cast<ManifestInstaller*>(any);
                                installer->Arch = Utility::ConvertToArchitectureEnum(value.as<std::string>());
                                return {};
                            }
                        },
                        {
                            "InstallerUrl",
                            [](const YAML::Node& value, std::any& any)->ValidationErrors
                            {
                                ManifestInstaller* installer = std::any_cast<ManifestInstaller*>(any);
                                installer->Url = value.as<std::string>();
                                return {};
                            }
                        },
                        {
                            "InstallerSha256",
                            [](const YAML::Node& value, std::any& any)->ValidationErrors
                            {
                                ManifestInstaller* installer = std::any_cast<ManifestInstaller*>(any);
                                installer->Sha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>());
                                return {};
                            }
                        },
                        {
                            "SignatureSha256",
                            [](const YAML::Node& value, std::any& any)->ValidationErrors
                            {
                                ManifestInstaller* installer = std::any_cast<ManifestInstaller*>(any);
                                installer->SignatureSha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>());
                                return {};
                            }
                        },
                    };

                    std::move(v1InstallerFields.begin(), v1InstallerFields.end(), std::inserter(result, result.end()));
                }
            }

            if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_1 })
            {
                std::vector<FieldProcessInfo> fields_v1_1 =
                {
                    {
                        "InstallerAbortsTerminal",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->InstallerAbortsTerminal = value.as<bool>();
                            return {};
                        }
                    },
                    {
                        "InstallLocationRequired",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->InstallLocationRequired = value.as<bool>();
                            return {};
                        }
                    },
                    {
                        "RequireExplicitUpgrade",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->RequireExplicitUpgrade = value.as<bool>();
                            return {};
                        }
                    },
                    {
                        "ReleaseDate",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->ReleaseDate = Utility::Trim(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "UnsupportedOSArchitectures",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->UnsupportedOSArchitectures = ProcessArchitectureSequenceNode(value);
                            return {};
                        }
                    },
                    {
                        "ElevationRequirement",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->ElevationRequirement = ConvertToElevationRequirementEnum(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "Markets",
                        [this, forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            return ProcessMarketsNode(value, installer);
                        }
                    },
                    {
                        "AppsAndFeaturesEntries",
                        [this, forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            return ProcessAppsAndFeaturesEntriesNode(value, installer);
                        }
                    },
                    {
                        "ExpectedReturnCodes",
                        [this, forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            return ProcessExpectedReturnCodesNode(value, installer);
                        }
                    },
                };

                std::move(fields_v1_1.begin(), fields_v1_1.end(), std::inserter(result, result.end()));
            }

            if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_2 })
            {
                std::vector<FieldProcessInfo> fields_v1_2 =
                {
                    {
                        "UnsupportedArguments",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->UnsupportedArguments = ProcessUnsupportedArgumentsSequenceNode(value);
                            return {};
                        }
                    },
                    {
                        "DisplayInstallWarnings",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->DisplayInstallWarnings = value.as<bool>();
                            return {};
                        }
                    },
                };

                std::move(fields_v1_2.begin(), fields_v1_2.end(), std::inserter(result, result.end()));
            }

            if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_4 })
            {
                std::vector<FieldProcessInfo> fields_v1_4 =
                {
                    {
                        "NestedInstallerType",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->NestedInstallerType = ConvertToInstallerTypeEnum(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "NestedInstallerFiles",
                        [this, forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            return ProcessNestedInstallerFilesNode(value, installer);
                        }
                    },
                    {
                        "InstallationMetadata",
                        [this, forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            std::any anyInstallationMetadata = &(installer->InstallationMetadata);
                            return ValidateAndProcessFields(value, InstallationMetadataFieldInfos, anyInstallationMetadata);
                        }
                    },
                };

                std::move(fields_v1_4.begin(), fields_v1_4.end(), std::inserter(result, result.end()));
            }

            if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_6 })
            {
                std::vector<FieldProcessInfo> fields_v1_6 =
                {
                    {
                        "DownloadCommandProhibited",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestInstaller* installer;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                installer = &(manifest->DefaultInstallerInfo);
                            }
                            else
                            {
                                installer = std::any_cast<ManifestInstaller*>(any);
                            }

                            installer->DownloadCommandProhibited = value.as<bool>();
                            return {};
                        },
                        true },
                };

                std::move(fields_v1_6.begin(), fields_v1_6.end(), std::inserter(result, result.end()));
            }
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetSwitchesFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        // Common fields across versions
        std::vector<FieldProcessInfo> result =
        {
            {
                "Custom",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    auto switches = std::any_cast<std::map<InstallerSwitchType, Utility::NormalizedString>*>(any);
                    (*switches)[InstallerSwitchType::Custom] = value.as<std::string>();
                    return{};
                }
            },
            {
                "Silent",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    auto switches = std::any_cast<std::map<InstallerSwitchType, Utility::NormalizedString>*>(any);
                    (*switches)[InstallerSwitchType::Silent] = value.as<std::string>();
                    return{};
                }
            },
            {
                "SilentWithProgress",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    auto switches = std::any_cast<std::map<InstallerSwitchType, Utility::NormalizedString>*>(any);
                    (*switches)[InstallerSwitchType::SilentWithProgress] = value.as<std::string>();
                    return{};
                }
            },
            {
                "Interactive",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    auto switches = std::any_cast<std::map<InstallerSwitchType, Utility::NormalizedString>*>(any);
                    (*switches)[InstallerSwitchType::Interactive] = value.as<std::string>();
                    return{};
                }
            },
            {
                "Log",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    auto switches = std::any_cast<std::map<InstallerSwitchType, Utility::NormalizedString>*>(any);
                    (*switches)[InstallerSwitchType::Log] = value.as<std::string>();
                    return{};
                }
            },
            {
                "InstallLocation",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    auto switches = std::any_cast<std::map<InstallerSwitchType, Utility::NormalizedString>*>(any);
                    (*switches)[InstallerSwitchType::InstallLocation] = value.as<std::string>();
                    return{};
                }
            },
        };

        // Additional version specific fields
        if (manifestVersion.Major() == 0)
        {
            // Language only exists in preview manifests. Though we don't use it in our code yet, keep it here to be consistent with schema.
            result.emplace_back(
                "Language",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    auto switches = std::any_cast<std::map<InstallerSwitchType, Utility::NormalizedString>*>(any);
                    (*switches)[InstallerSwitchType::Language] = value.as<std::string>();
                    return{};
                });
            result.emplace_back(
                "Update",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    auto switches = std::any_cast<std::map<InstallerSwitchType, Utility::NormalizedString>*>(any);
                    (*switches)[InstallerSwitchType::Update] = value.as<std::string>();
                    return{};
                });
        }
        else if (manifestVersion.Major() == 1)
        {
            result.emplace_back(
                "Upgrade",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    auto switches = std::any_cast<std::map<InstallerSwitchType, Utility::NormalizedString>*>(any);
                    (*switches)[InstallerSwitchType::Update] = value.as<std::string>();
                    return{};
                });
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetExpectedReturnCodesFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        std::vector<FieldProcessInfo> result = {};

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_1 })
        {
            result.emplace_back(
                "InstallerReturnCode",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    ExpectedReturnCode* expectedReturnCode = std::any_cast<ExpectedReturnCode*>(any);
                    expectedReturnCode->InstallerReturnCode = static_cast<int>(value.as<int>());
                    return {};
                });
            result.emplace_back(
                "ReturnResponse",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    ExpectedReturnCode* expectedReturnCode = std::any_cast<ExpectedReturnCode*>(any);
                    expectedReturnCode->ReturnResponse = ConvertToExpectedReturnCodeEnum(value.as<std::string>());
                    return {};
                });
        }

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_2 })
        {
            result.emplace_back(
                "ReturnResponseUrl",
                [](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    ExpectedReturnCode* expectedReturnCode = std::any_cast<ExpectedReturnCode*>(any);
                    expectedReturnCode->ReturnResponseUrl = value.as<std::string>();
                    return {};
                });
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetLocalizationFieldProcessInfo(const ManifestVer& manifestVersion, bool forRootFields)
    {
        // Common fields across versions
        std::vector<FieldProcessInfo> result =
        {
            {
                "Description",
                [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    ManifestLocalization* localization;
                    if (forRootFields)
                    {
                        Manifest* manifest = std::any_cast<Manifest*>(any);
                        localization = &(manifest->DefaultLocalization);
                    }
                    else
                    {
                        localization = std::any_cast<ManifestLocalization*>(any);
                    }

                    localization->Add<Localization::Description>(Utility::Trim(value.as<std::string>()));
                    return {};
                }
            },
            {
                "LicenseUrl",
                [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    ManifestLocalization* localization;
                    if (forRootFields)
                    {
                        Manifest* manifest = std::any_cast<Manifest*>(any);
                        localization = &(manifest->DefaultLocalization);
                    }
                    else
                    {
                        localization = std::any_cast<ManifestLocalization*>(any);
                    }

                    localization->Add<Localization::LicenseUrl>(value.as<std::string>());
                    return {};
                }
            },
        };

        // Additional version specific fields
        if (manifestVersion.Major() == 0)
        {
            // Root level and Localization node level
            result.emplace_back(
                "Homepage",
                [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                {
                    ManifestLocalization* localization;
                    if (forRootFields)
                    {
                        Manifest* manifest = std::any_cast<Manifest*>(any);
                        localization = &(manifest->DefaultLocalization);
                    }
                    else
                    {
                        localization = std::any_cast<ManifestLocalization*>(any);
                    }

                    localization->Add<Localization::PackageUrl>(value.as<std::string>());
                    return {};
                });

            if (!forRootFields)
            {
                // Localization node only
                result.emplace_back(
                    "Language",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        ManifestLocalization* localization = std::any_cast<ManifestLocalization*>(any);
                        localization->Locale = value.as<std::string>();
                        return {};
                    });
            }
            else
            {
                // Root node only
                std::vector<FieldProcessInfo> rootOnlyFields =
                {
                    {
                        "Name",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            ManifestLocalization* localization = &(manifest->DefaultLocalization);
                            localization->Add<Localization::PackageName>(Utility::Trim(value.as<std::string>()));
                            return {};
                        }
                    },
                    {
                        "Publisher",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            ManifestLocalization* localization = &(manifest->DefaultLocalization);
                            localization->Add<Localization::Publisher>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "Author",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            ManifestLocalization* localization = &(manifest->DefaultLocalization);
                            localization->Add<Localization::Author>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "License",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            ManifestLocalization* localization = &(manifest->DefaultLocalization);
                            localization->Add<Localization::License>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "Tags",
                        [](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            Manifest* manifest = std::any_cast<Manifest*>(any);
                            ManifestLocalization* localization = &(manifest->DefaultLocalization);
                            localization->Add<Localization::Tags>(SplitMultiValueField(value.as<std::string>()));
                            return {};
                        }
                    },
                };

                std::move(rootOnlyFields.begin(), rootOnlyFields.end(), std::inserter(result, result.end()));
            }
        }
        else if (manifestVersion.Major() == 1)
        {
            // Starting v1, we should be only adding new fields for each minor version increase
            if (manifestVersion >= ManifestVer{ s_ManifestVersionV1 })
            {
                // Root level and Localization node level
                std::vector<FieldProcessInfo> v1CommonFields =
                {
                    {
                        "PackageLocale",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Locale = value.as<std::string>();
                            return {};
                        }
                    },
                    {
                        "Publisher",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::Publisher>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "PublisherUrl",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::PublisherUrl>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "PublisherSupportUrl",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::PublisherSupportUrl>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "PrivacyUrl",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::PrivacyUrl>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "Author",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::Author>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "PackageName",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::PackageName>(Utility::Trim(value.as<std::string>()));
                            return {};
                        }
                    },
                    {
                        "PackageUrl",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::PackageUrl>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "License",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::License>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "Copyright",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::Copyright>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "CopyrightUrl",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::CopyrightUrl>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "ShortDescription",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::ShortDescription>(Utility::Trim(value.as<std::string>()));
                            return {};
                        }
                    },
                    {
                        "Tags",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::Tags>(ProcessStringSequenceNode(value));
                            return {};
                        }
                    },
                };

                std::move(v1CommonFields.begin(), v1CommonFields.end(), std::inserter(result, result.end()));
            }

            if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_1 })
            {
                std::vector<FieldProcessInfo> fields_v1_1 =
                {
                    {
                        "Agreements",
                        [this, forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            return ProcessAgreementsNode(value, localization);
                        }
                    },
                    {
                        "ReleaseNotes",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::ReleaseNotes>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "ReleaseNotesUrl",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::ReleaseNotesUrl>(value.as<std::string>());
                            return {};
                        }
                    },
                };

                std::move(fields_v1_1.begin(), fields_v1_1.end(), std::inserter(result, result.end()));
            }

            if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_2 })
            {
                std::vector<FieldProcessInfo> fields_v1_2 =
                {
                    {
                        "PurchaseUrl",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::PurchaseUrl>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "InstallationNotes",
                        [forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            localization->Add<Localization::InstallationNotes>(value.as<std::string>());
                            return {};
                        }
                    },
                    {
                        "Documentations",
                        [this, forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            return ProcessDocumentationsNode(value, localization);
                        }
                    },
                };

                std::move(fields_v1_2.begin(), fields_v1_2.end(), std::inserter(result, result.end()));
            }

            if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_5 })
            {
                std::vector<FieldProcessInfo> fields_v1_5 =
                {
                    {
                        "Icons",
                        [this, forRootFields](const YAML::Node& value, std::any& any)->ValidationErrors
                        {
                            ManifestLocalization* localization;
                            if (forRootFields)
                            {
                                Manifest* manifest = std::any_cast<Manifest*>(any);
                                localization = &(manifest->DefaultLocalization);
                            }
                            else
                            {
                                localization = std::any_cast<ManifestLocalization*>(any);
                            }

                            return ProcessIconsNode(value, localization);
                        },
                        true
                    },
                };

                std::move(fields_v1_5.begin(), fields_v1_5.end(), std::inserter(result, result.end()));
            }
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetDependenciesFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        std::vector<FieldProcessInfo> result = {};

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1 })
        {
            result =
            {
                {
                    "WindowsFeatures",
                    [this](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        DependencyList* dependencyList = std::any_cast<DependencyList*>(any);
                        ProcessDependenciesNode(DependencyType::WindowsFeature, value, dependencyList);
                        return {};
                    }
                },
                {
                    "WindowsLibraries",
                    [this](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        DependencyList* dependencyList = std::any_cast<DependencyList*>(any);
                        ProcessDependenciesNode(DependencyType::WindowsLibrary, value, dependencyList);
                        return {};
                    }
                },
                {
                    "PackageDependencies",
                    [this](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        DependencyList* dependencyList = std::any_cast<DependencyList*>(any);
                        ProcessPackageDependenciesNode(value, dependencyList);
                        return {};
                    }
                },
                {
                    "ExternalDependencies",
                    [this](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        DependencyList* dependencyList = std::any_cast<DependencyList*>(any);
                        ProcessDependenciesNode(DependencyType::External, value, dependencyList);
                        return {};
                    }
                },
            };
        }

        return result;
    }

    void ManifestYamlPopulator::ProcessDependenciesNode(DependencyType type, const YAML::Node& node, DependencyList* dependencyList)
    {
        const auto& ids = ProcessStringSequenceNode(node);
        for (auto id : ids)
        {
            dependencyList->Add(Dependency(type, id));
        }
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetPackageDependenciesFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        std::vector<FieldProcessInfo> result = {};

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1 })
        {
            result =
            {
                {
                    "PackageIdentifier",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Dependency* packageDependency = std::any_cast<Dependency*>(any);
                        packageDependency->SetId(Utility::Trim(value.as<std::string>()));
                        return {};
                    }
                },
                {
                    "MinimumVersion",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Dependency* packageDependency = std::any_cast<Dependency*>(any);
                        packageDependency->MinVersion = Utility::Version(Utility::Trim(value.as<std::string>()));
                        return {};
                    }
                },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetAgreementFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        std::vector<FieldProcessInfo> result = {};

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_1 })
        {
            result =
            {
                {
                    "AgreementLabel",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Agreement* agreement = std::any_cast<Agreement*>(any);
                        agreement->Label = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "Agreement",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Agreement* agreement = std::any_cast<Agreement*>(any);
                        agreement->AgreementText = Utility::Trim(value.as<std::string>());
                        return {};
                    },
                    true },
                {
                    "AgreementUrl",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Agreement* agreement = std::any_cast<Agreement*>(any);
                        agreement->AgreementUrl = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetMarketsFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        std::vector<FieldProcessInfo> result = {};

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_1 })
        {
            result =
            {
                {
                    "AllowedMarkets",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        MarketsInfo* markets = std::any_cast<MarketsInfo*>(any);
                        markets->AllowedMarkets = ProcessStringSequenceNode(value);
                        return {};
                    }
                },
                {
                    "ExcludedMarkets",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        MarketsInfo* markets = std::any_cast<MarketsInfo*>(any);
                        markets->ExcludedMarkets = ProcessStringSequenceNode(value);
                        return {};
                    }
                },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetAppsAndFeaturesEntryFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        std::vector<FieldProcessInfo> result = {};

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_1 })
        {
            result =
            {
                {
                    "DisplayName",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        AppsAndFeaturesEntry* appsAndFeaturesEntry = std::any_cast<AppsAndFeaturesEntry*>(any);
                        appsAndFeaturesEntry->DisplayName = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "Publisher",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        AppsAndFeaturesEntry* appsAndFeaturesEntry = std::any_cast<AppsAndFeaturesEntry*>(any);
                        appsAndFeaturesEntry->Publisher = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "DisplayVersion",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        AppsAndFeaturesEntry* appsAndFeaturesEntry = std::any_cast<AppsAndFeaturesEntry*>(any);
                        appsAndFeaturesEntry->DisplayVersion = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "ProductCode",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        AppsAndFeaturesEntry* appsAndFeaturesEntry = std::any_cast<AppsAndFeaturesEntry*>(any);
                        appsAndFeaturesEntry->ProductCode = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "UpgradeCode",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        AppsAndFeaturesEntry* appsAndFeaturesEntry = std::any_cast<AppsAndFeaturesEntry*>(any);
                        appsAndFeaturesEntry->UpgradeCode = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "InstallerType",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        AppsAndFeaturesEntry* appsAndFeaturesEntry = std::any_cast<AppsAndFeaturesEntry*>(any);
                        appsAndFeaturesEntry->InstallerType = ConvertToInstallerTypeEnum(value.as<std::string>());
                        return {};
                    }
                },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetDocumentationFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        std::vector<FieldProcessInfo> result = {};

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_2 })
        {
            result =
            {
                {
                    "DocumentLabel",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Documentation* documentation = std::any_cast<Documentation*>(any);
                        documentation->DocumentLabel = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "DocumentUrl",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Documentation* documentation = std::any_cast<Documentation*>(any);
                        documentation->DocumentUrl = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetIconFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        std::vector<FieldProcessInfo> result = {};

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_5 })
        {
            result =
            {
                {
                    "IconUrl",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Icon* icon = std::any_cast<Icon*>(any);
                        icon->Url = Utility::Trim(value.as<std::string>()); return {};
                    }
                },
                {
                    "IconFileType",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Icon* icon = std::any_cast<Icon*>(any);
                        icon->FileType = ConvertToIconFileTypeEnum(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "IconResolution",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Icon* icon = std::any_cast<Icon*>(any);
                        icon->Resolution = ConvertToIconResolutionEnum(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "IconTheme",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Icon* icon = std::any_cast<Icon*>(any);
                        icon->Theme = ConvertToIconThemeEnum(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "IconSha256",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        Icon* icon = std::any_cast<Icon*>(any);
                        icon->Sha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>());
                        return {};
                    }
                },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetNestedInstallerFileFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        std::vector<FieldProcessInfo> result = {};

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_4 })
        {
            result =
            {
                {
                    "RelativeFilePath",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        NestedInstallerFile* nestedInstallerFile = std::any_cast<NestedInstallerFile*>(any);
                        nestedInstallerFile->RelativeFilePath = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "PortableCommandAlias",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        NestedInstallerFile* nestedInstallerFile = std::any_cast<NestedInstallerFile*>(any);
                        nestedInstallerFile->PortableCommandAlias = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetInstallationMetadataFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        std::vector<FieldProcessInfo> result = {};

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_4 })
        {
            result =
            {
                {
                    "DefaultInstallLocation",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        InstallationMetadataInfo* installationMetadata = std::any_cast<InstallationMetadataInfo*>(any);
                        installationMetadata->DefaultInstallLocation = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "Files",
                    [this](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        InstallationMetadataInfo* installationMetadata = std::any_cast<InstallationMetadataInfo*>(any);
                        return ProcessInstallationMetadataFilesNode(value, installationMetadata);
                    }
                },
            };
        }

        return result;
    }

    std::vector<ManifestYamlPopulator::FieldProcessInfo> ManifestYamlPopulator::GetInstallationMetadataFilesFieldProcessInfo(const ManifestVer& manifestVersion)
    {
        std::vector<FieldProcessInfo> result = {};

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_4 })
        {
            result =
            {
                {
                    "RelativeFilePath",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        InstalledFile* installedFile = std::any_cast<InstalledFile*>(any);
                        installedFile->RelativeFilePath = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "FileSha256",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        InstalledFile* installedFile = std::any_cast<InstalledFile*>(any);
                        installedFile->FileSha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "FileType",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        InstalledFile* installedFile = std::any_cast<InstalledFile*>(any);
                        installedFile->FileType = ConvertToInstalledFileTypeEnum(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "InvocationParameter",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        InstalledFile* installedFile = std::any_cast<InstalledFile*>(any);
                        installedFile->InvocationParameter = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
                {
                    "DisplayName",
                    [](const YAML::Node& value, std::any& any)->ValidationErrors
                    {
                        InstalledFile* installedFile = std::any_cast<InstalledFile*>(any);
                        installedFile->DisplayName = Utility::Trim(value.as<std::string>());
                        return {};
                    }
                },
            };
        }

        return result;
    }

    ValidationErrors ManifestYamlPopulator::ValidateAndProcessFields(
        const YAML::Node& rootNode,
        const std::vector<FieldProcessInfo>& fieldInfos,
        std::any& any)
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

            // We'll do case insensitive search first and validate correct case later.
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
                        auto errors = fieldInfo.ProcessFunc(valueNode, any);
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
            std::any any = &packageDependency;
            auto errors = ValidateAndProcessFields(entry, PackageDependenciesFieldInfos, any);
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
            std::any any = &agreement;
            auto errors = ValidateAndProcessFields(entry, AgreementFieldInfos, any);
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
        std::any any = &markets;
        auto errors = ValidateAndProcessFields(marketsNode, MarketsFieldInfos, any);
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
            std::any any = &appsAndFeaturesEntry;
            auto errors = ValidateAndProcessFields(entry, AppsAndFeaturesEntryFieldInfos, any);
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
            std::any any = &returnCode;
            auto errors = ValidateAndProcessFields(entry, ExpectedReturnCodesFieldInfos, any);
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
            std::any any = &documentation;
            auto errors = ValidateAndProcessFields(entry, DocumentationFieldInfos, any);
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
            std::any any = &icon;
            auto errors = ValidateAndProcessFields(entry, IconFieldInfos, any);
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
            std::any any = &nestedInstallerFile;
            auto errors = ValidateAndProcessFields(entry, NestedInstallerFileFieldInfos, any);
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
            std::any any = &installedFile;
            auto errors = ValidateAndProcessFields(entry, InstallationMetadataFilesFieldInfos, any);
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            installedFiles.emplace_back(std::move(installedFile));
        }

        if (!installedFiles.empty())
        {
            installationMetadata->Files = installedFiles;
        }

        return resultErrors;
    }

    ValidationErrors ManifestYamlPopulator::PopulateManifestInternal(
        const YAML::Node& rootNode,
        Manifest& manifest,
        const ManifestVer& manifestVersion,
        ManifestValidateOption validateOption)
    {
        m_validateOption = validateOption;
        m_isMergedManifest = !rootNode["ManifestType"sv].IsNull() && rootNode["ManifestType"sv].as<std::string>() == "merged";

        ValidationErrors resultErrors;
        manifest.ManifestVersion = manifestVersion;

        // Prepare field infos
        RootFieldInfos = GetRootFieldProcessInfo(manifestVersion);
        InstallerFieldInfos = GetInstallerFieldProcessInfo(manifestVersion);
        SwitchesFieldInfos = GetSwitchesFieldProcessInfo(manifestVersion);
        ExpectedReturnCodesFieldInfos = GetExpectedReturnCodesFieldProcessInfo(manifestVersion);
        DependenciesFieldInfos = GetDependenciesFieldProcessInfo(manifestVersion);
        PackageDependenciesFieldInfos = GetPackageDependenciesFieldProcessInfo(manifestVersion);
        LocalizationFieldInfos = GetLocalizationFieldProcessInfo(manifestVersion);
        AgreementFieldInfos = GetAgreementFieldProcessInfo(manifestVersion);
        MarketsFieldInfos = GetMarketsFieldProcessInfo(manifestVersion);
        AppsAndFeaturesEntryFieldInfos = GetAppsAndFeaturesEntryFieldProcessInfo(manifestVersion);
        DocumentationFieldInfos = GetDocumentationFieldProcessInfo(manifestVersion);
        IconFieldInfos = GetIconFieldProcessInfo(manifestVersion);
        NestedInstallerFileFieldInfos = GetNestedInstallerFileFieldProcessInfo(manifestVersion);
        InstallationMetadataFieldInfos = GetInstallationMetadataFieldProcessInfo(manifestVersion);
        InstallationMetadataFilesFieldInfos = GetInstallationMetadataFilesFieldProcessInfo(manifestVersion);

        std::any anyManifest = &manifest;
        resultErrors = ValidateAndProcessFields(rootNode, RootFieldInfos, anyManifest);

        if (!m_p_installersNode)
        {
            return resultErrors;
        }

        // Populate installers
        for (auto const& entry : m_p_installersNode->Sequence())
        {
            ManifestInstaller installer = manifest.DefaultInstallerInfo;

            // Clear these defaults as PackageFamilyName, ProductCode, AppsAndFeaturesEntries need to be copied based on InstallerType
            installer.PackageFamilyName.clear();
            installer.ProductCode.clear();
            installer.AppsAndFeaturesEntries.clear();
            // Clear dependencies as installer overrides root dependencies
            installer.Dependencies.Clear();
            // Clear nested installers as it should only be copied for zip installerType.
            installer.NestedInstallerType = InstallerTypeEnum::Unknown;
            installer.NestedInstallerFiles.clear();

            std::any anyInstaller = &installer;
            auto errors = ValidateAndProcessFields(entry, InstallerFieldInfos, anyInstaller);
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));

            // Copy in system reference strings from the root if not set in the installer and appropriate
            if (installer.PackageFamilyName.empty() && DoesInstallerTypeUsePackageFamilyName(installer.EffectiveInstallerType()))
            {
                installer.PackageFamilyName = manifest.DefaultInstallerInfo.PackageFamilyName;
            }

            if (installer.ProductCode.empty() && DoesInstallerTypeUseProductCode(installer.EffectiveInstallerType()))
            {
                installer.ProductCode = manifest.DefaultInstallerInfo.ProductCode;
            }

            if (installer.AppsAndFeaturesEntries.empty() && DoesInstallerTypeWriteAppsAndFeaturesEntry(installer.EffectiveInstallerType()))
            {
                installer.AppsAndFeaturesEntries = manifest.DefaultInstallerInfo.AppsAndFeaturesEntries;
            }

            if (IsArchiveType(installer.BaseInstallerType))
            {
                if (installer.NestedInstallerFiles.empty())
                {
                    installer.NestedInstallerFiles = manifest.DefaultInstallerInfo.NestedInstallerFiles;
                }

                if (installer.NestedInstallerType == InstallerTypeEnum::Unknown)
                {
                    installer.NestedInstallerType = manifest.DefaultInstallerInfo.NestedInstallerType;
                }
            }

            // If there are no dependencies on installer use default ones
            if (!installer.Dependencies.HasAny())
            {
                installer.Dependencies = manifest.DefaultInstallerInfo.Dependencies;
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

            manifest.Installers.emplace_back(std::move(installer));
        }

        // Populate additional localizations
        if (m_p_localizationsNode && m_p_localizationsNode->IsSequence())
        {
            for (auto const& entry : m_p_localizationsNode->Sequence())
            {
                ManifestLocalization localization;
                std::any any = &localization;
                auto errors = ValidateAndProcessFields(entry, LocalizationFieldInfos, any);
                std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
                manifest.Localizations.emplace_back(std::move(std::move(localization)));
            }
        }

        return resultErrors;
    }

    ValidationErrors ManifestYamlPopulator::PopulateManifest(
        const YAML::Node& rootNode,
        Manifest& manifest,
        const ManifestVer& manifestVersion,
        ManifestValidateOption validateOption)
    {
        ManifestYamlPopulator manifestPopulator;
        return manifestPopulator.PopulateManifestInternal(rootNode, manifest, manifestVersion, validateOption);
    }
}