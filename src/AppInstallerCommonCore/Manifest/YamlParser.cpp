// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerSHA256.h"
#include "winget/Yaml.h"
#include "winget/ManifestYamlParser.h"

namespace AppInstaller::Manifest
{
    namespace
    {
        // The maximum supported major version known about by this code.
        constexpr uint64_t s_MaxSupportedMajorVersion = 0;

        // The default manifest version assigned to manifests without a ManifestVersion field.
        constexpr std::string_view s_DefaultManifestVersion = "0.1.0"sv;

        // The manifest extension for the MS Store
        constexpr std::string_view s_MSStoreExtension = "msstore"sv;

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
    }

    void YamlParser::PrepareManifestFieldInfos(const ManifestVer& manifestVer)
    {
        // Initially supported fields
        RootFieldInfos =
        {
            { "ManifestVersion", [](const YAML::Node&) { /* ManifestVersion already processed */ }, false,
            // Regex here is to prevent leading 0s in the version, this also keeps consistent with other versions in the manifest
            "^(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])(\\.(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])){2}$" },
            { "Id", [this](const YAML::Node& value) { m_p_manifest->Id = value.as<std::string>(); Utility::Trim(m_p_manifest->Id); }, true, "^[\\S]+\\.[\\S]+$" },
            { "Name", [this](const YAML::Node& value) { m_p_manifest->Name = value.as<std::string>(); Utility::Trim(m_p_manifest->Name); }, true },
            { "Version", [this](const YAML::Node& value) { m_p_manifest->Version = value.as<std::string>(); Utility::Trim(m_p_manifest->Version); }, true,
            /* File name chars not allowed */ "^[^\\\\/:\\*\\?\"<>\\|\\x01-\\x1f]+$" },
            { "Publisher", [this](const YAML::Node& value) { m_p_manifest->Publisher = value.as<std::string>(); }, true },
            { "AppMoniker", [this](const YAML::Node& value) { m_p_manifest->AppMoniker = value.as<std::string>(); Utility::Trim(m_p_manifest->AppMoniker); } },
            { "Channel", [this](const YAML::Node& value) { m_p_manifest->Channel = value.as<std::string>(); Utility::Trim(m_p_manifest->Channel); } },
            { "Author", [this](const YAML::Node& value) { m_p_manifest->Author = value.as<std::string>(); } },
            { "License", [this](const YAML::Node& value) { m_p_manifest->License = value.as<std::string>(); } },
            { "MinOSVersion", [this](const YAML::Node& value) { m_p_manifest->MinOSVersion = value.as<std::string>(); Utility::Trim(m_p_manifest->MinOSVersion); }, false,
              "^(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])(\\.(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])){0,3}$" },
            { "Tags", [this](const YAML::Node& value) { m_p_manifest->Tags = SplitMultiValueField(value.as<std::string>()); } },
            { "Commands", [this](const YAML::Node& value) { m_p_manifest->Commands = SplitMultiValueField(value.as<std::string>()); } },
            { "Protocols", [this](const YAML::Node& value) { m_p_manifest->Protocols = SplitMultiValueField(value.as<std::string>()); } },
            { "FileExtensions", [this](const YAML::Node& value) { m_p_manifest->FileExtensions = SplitMultiValueField(value.as<std::string>()); } },
            { "InstallerType", [this](const YAML::Node& value) { m_p_manifest->InstallerType = ManifestInstaller::ConvertToInstallerTypeEnum(value.as<std::string>()); } },
            { "UpdateBehavior", [this](const YAML::Node& value) { m_p_manifest->UpdateBehavior = ManifestInstaller::ConvertToUpdateBehaviorEnum(value.as<std::string>()); } },
            { "PackageFamilyName", [this](const YAML::Node& value) { m_p_manifest->PackageFamilyName = value.as<std::string>(); }, false, "[-.A-Za-z0-9]+_[A-Za-z0-9]{13}" },
            { "ProductCode", [this](const YAML::Node& value) { m_p_manifest->ProductCode = value.as<std::string>(); } },
            { "Description", [this](const YAML::Node& value) { m_p_manifest->Description = value.as<std::string>(); } },
            { "Homepage", [this](const YAML::Node& value) { m_p_manifest->Homepage = value.as<std::string>(); } },
            { "LicenseUrl", [this](const YAML::Node& value) { m_p_manifest->LicenseUrl = value.as<std::string>(); } },
            { "Switches", [this](const YAML::Node& value) { *m_p_switchesNode = value; } },
            { "Installers", [this](const YAML::Node& value) { *m_p_installersNode = value; }, true },
            { "Localization", [this](const YAML::Node& value) { *m_p_localizationsNode = value; } },
        };

        InstallerFieldInfos =
        {
            { "Arch", [this](const YAML::Node& value) { m_p_installer->Arch = Utility::ConvertToArchitectureEnum(value.as<std::string>()); }, true },
            { "Url", [this](const YAML::Node& value) { m_p_installer->Url = value.as<std::string>(); } },
            { "Sha256", [this](const YAML::Node& value) { m_p_installer->Sha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>()); }, false, "^[A-Fa-f0-9]{64}$" },
            { "SignatureSha256", [this](const YAML::Node& value) { m_p_installer->SignatureSha256 = Utility::SHA256::ConvertToBytes(value.as<std::string>()); }, false, "^[A-Fa-f0-9]{64}$" },
            { "Language", [this](const YAML::Node& value) { m_p_installer->Language = value.as<std::string>(); } },
            { "Scope", [this](const YAML::Node& value) { m_p_installer->Scope = ManifestInstaller::ConvertToScopeEnum(value.as<std::string>()); } },
            { "InstallerType", [this](const YAML::Node& value) { m_p_installer->InstallerType = ManifestInstaller::ConvertToInstallerTypeEnum(value.as<std::string>()); } },
            { "UpdateBehavior", [this](const YAML::Node& value) { m_p_installer->UpdateBehavior = ManifestInstaller::ConvertToUpdateBehaviorEnum(value.as<std::string>()); } },
            { "PackageFamilyName", [this](const YAML::Node& value) { m_p_installer->PackageFamilyName = value.as<std::string>(); }, false, "[-.A-Za-z0-9]+_[A-Za-z0-9]{13}" },
            { "ProductCode", [this](const YAML::Node& value) { m_p_installer->ProductCode = value.as<std::string>(); } },
            { "Switches", [this](const YAML::Node& value) { *m_p_switchesNode = value; } },
        };

        SwitchesFieldInfos =
        {
            { "Custom", [this](const YAML::Node& value) { (*m_p_switches)[ManifestInstaller::InstallerSwitchType::Custom] = value.as<std::string>(); } },
            { "Silent", [this](const YAML::Node& value) { (*m_p_switches)[ManifestInstaller::InstallerSwitchType::Silent] = value.as<std::string>(); } },
            { "SilentWithProgress", [this](const YAML::Node& value) { (*m_p_switches)[ManifestInstaller::InstallerSwitchType::SilentWithProgress] = value.as<std::string>(); } },
            { "Interactive", [this](const YAML::Node& value) { (*m_p_switches)[ManifestInstaller::InstallerSwitchType::Interactive] = value.as<std::string>(); } },
            { "Language", [this](const YAML::Node& value) { (*m_p_switches)[ManifestInstaller::InstallerSwitchType::Language] = value.as<std::string>(); } },
            { "Log", [this](const YAML::Node& value) { (*m_p_switches)[ManifestInstaller::InstallerSwitchType::Log] = value.as<std::string>(); } },
            { "InstallLocation", [this](const YAML::Node& value) { (*m_p_switches)[ManifestInstaller::InstallerSwitchType::InstallLocation] = value.as<std::string>(); } },
            { "Update", [this](const YAML::Node& value) { (*m_p_switches)[ManifestInstaller::InstallerSwitchType::Update] = value.as<std::string>(); } },
        };

        LocalizationFieldInfos =
        {
            { "Language", [this](const YAML::Node& value) { m_p_localization->Language = value.as<std::string>(); }, true },
            { "Description", [this](const YAML::Node& value) { m_p_localization->Description = value.as<std::string>(); } },
            { "Homepage", [this](const YAML::Node& value) { m_p_localization->Homepage = value.as<std::string>(); } },
            { "LicenseUrl", [this](const YAML::Node& value) { m_p_localization->LicenseUrl = value.as<std::string>(); } },
        };

        // Store extension
        if (manifestVer.HasExtension(s_MSStoreExtension))
        {
            InstallerFieldInfos.emplace_back("ProductId", [this](const YAML::Node& value) { m_p_installer->ProductId = value.as<std::string>(); });
        }
    }

    Manifest YamlParser::CreateFromPath(const std::filesystem::path& inputFile, bool fullValidation, bool throwOnWarning)
    {
        Manifest manifest;
        std::vector<ValidationError> errors;

        try
        {
            YAML::Node rootNode = YAML::Load(inputFile);
            YamlParser parser;
            errors = parser.ParseManifest(rootNode, manifest, fullValidation);
        }
        catch (const ManifestException&)
        {
            // Prevent ManifestException from being wrapped in another ManifestException
            throw;
        }
        catch (const std::exception& e)
        {
            THROW_EXCEPTION_MSG(ManifestException(), e.what());
        }

        if (!errors.empty())
        {
            ManifestException ex{ std::move(errors) };

            if (throwOnWarning || !ex.IsWarningOnly())
            {
                THROW_EXCEPTION(ex);
            }
        }

        return manifest;
    }

    Manifest YamlParser::Create(const std::string& input, bool fullValidation, bool throwOnWarning)
    {
        Manifest manifest;
        std::vector<ValidationError> errors;

        try
        {
            YAML::Node rootNode = YAML::Load(input);
            YamlParser parser;
            errors = parser.ParseManifest(rootNode, manifest, fullValidation);
        }
        catch (const ManifestException&)
        {
            // Prevent ManifestException from being wrapped in another ManifestException
            throw;
        }
        catch (const std::exception& e)
        {
            THROW_EXCEPTION_MSG(ManifestException(), e.what());
        }

        if (!errors.empty())
        {
            ManifestException ex{ std::move(errors) };

            if (throwOnWarning || !ex.IsWarningOnly())
            {
                THROW_EXCEPTION(ex);
            }
        }

        return manifest;
    }

    std::vector<ValidationError> YamlParser::ParseManifest(const YAML::Node& rootNode, Manifest& manifest, bool fullValidation)
    {
        // Detects empty files with a better error.
        if (!rootNode.IsMap())
        {
            THROW_EXCEPTION_MSG(ManifestException(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST), "The manifest does not contain a valid root.");
        }

        // Detect manifest version first to determine expected fields
        // Use index to access ManifestVersion directly. If there're duplicates or other general errors, it'll be detected in later
        // processing of iterating the whole manifest.
        if (rootNode["ManifestVersion"sv])
        {
            auto manifestVersionValue = rootNode["ManifestVersion"sv].as<std::string>();
            manifest.ManifestVersion = ManifestVer(manifestVersionValue);
        }
        else
        {
            manifest.ManifestVersion = ManifestVer(s_DefaultManifestVersion);
        }

        // Check manifest version is supported
        if (manifest.ManifestVersion.Major() > s_MaxSupportedMajorVersion)
        {
            THROW_EXCEPTION_MSG(ManifestException(APPINSTALLER_CLI_ERROR_UNSUPPORTED_MANIFESTVERSION), "Unsupported ManifestVersion: %S", manifest.ManifestVersion.ToString().c_str());
        }

        PrepareManifestFieldInfos(manifest.ManifestVersion);

        // Populate root fields
        YAML::Node switchesNode;
        YAML::Node installersNode;
        YAML::Node localizationsNode;
        m_p_switchesNode = &switchesNode;
        m_p_installersNode = &installersNode;
        m_p_localizationsNode = &localizationsNode;
        m_p_manifest = &manifest;
        auto resultErrors = ValidateAndProcessFields(rootNode, RootFieldInfos, fullValidation);

        // Populate root switches
        if (!switchesNode.IsNull())
        {
            m_p_switches = &manifest.Switches;
            auto errors = ValidateAndProcessFields(switchesNode, SwitchesFieldInfos, fullValidation);
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
        }

        // Populate installers
        for (std::size_t i = 0; i < installersNode.size(); i++)
        {
            YAML::Node installerNode = installersNode[i];
            ManifestInstaller installer;
            YAML::Node installerSwitchesNode;

            // Populate defaults
            installer.InstallerType = manifest.InstallerType;
            installer.UpdateBehavior = manifest.UpdateBehavior;
            installer.Scope = ManifestInstaller::ScopeEnum::User;

            m_p_installer = &installer;
            m_p_switchesNode = &installerSwitchesNode;
            auto errors = ValidateAndProcessFields(installerNode, InstallerFieldInfos, fullValidation);
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));

            // Copy in system reference strings from the root if not set in the installer and appropriate
            if (installer.PackageFamilyName.empty() && ManifestInstaller::DoesInstallerTypeUsePackageFamilyName(installer.InstallerType))
            {
                installer.PackageFamilyName = manifest.PackageFamilyName;
            }

            if (installer.ProductCode.empty() && ManifestInstaller::DoesInstallerTypeUseProductCode(installer.InstallerType))
            {
                installer.ProductCode = manifest.ProductCode;
            }

            // Populate default known switches
            installer.Switches = GetDefaultKnownSwitches(installer.InstallerType);

            // Override with switches from manifest root if applicable
            for (auto const& keyValuePair : manifest.Switches)
            {
                installer.Switches[keyValuePair.first] = keyValuePair.second;
            }

            // Override with switches from installer declaration if applicable
            if (!installerSwitchesNode.IsNull())
            {
                m_p_switches = &installer.Switches;
                auto switchesErrors = ValidateAndProcessFields(installerSwitchesNode, SwitchesFieldInfos, fullValidation);
                std::move(switchesErrors.begin(), switchesErrors.end(), std::inserter(resultErrors, resultErrors.end()));
            }

            manifest.Installers.emplace_back(std::move(installer));
        }

        // Populate localization fields
        if (!localizationsNode.IsNull())
        {
            for (std::size_t i = 0; i < localizationsNode.size(); i++)
            {
                YAML::Node localizationNode = localizationsNode[i];
                ManifestLocalization localization;

                // Populates default values from root first
                localization.Description = manifest.Description;
                localization.Homepage = manifest.Homepage;
                localization.LicenseUrl = manifest.LicenseUrl;

                m_p_localization = &localization;
                auto errors = ValidateAndProcessFields(localizationNode, LocalizationFieldInfos, fullValidation);
                std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
                manifest.Localization.emplace_back(std::move(localization));
            }
        }

        // Extra semantic validations after basic validation and field population
        if (fullValidation)
        {
            auto errors = ValidateManifest(manifest);
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
        }

        return resultErrors;
    }

    std::vector<ValidationError> YamlParser::ValidateAndProcessFields(
        const YAML::Node& rootNode,
        const std::vector<ManifestFieldInfo>& fieldInfos,
        bool fullValidation)
    {
        std::vector<ValidationError> errors;

        if (rootNode.size() == 0 || !rootNode.IsMap())
        {
            errors.emplace_back(ManifestError::InvalidRootNode, "", "", rootNode.Mark().line, rootNode.Mark().column);
            return errors;
        }

        // Keeps track of already processed fields. Used to check duplicate fields or missing required fields.
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
                const ManifestFieldInfo& fieldInfo = *fieldIter;

                // Make sure the found key is in Pascal Case
                if (key != fieldInfo.Name)
                {
                    errors.emplace_back(ManifestError::FieldIsNotPascalCase, key, "", keyValuePair.first.Mark().line, keyValuePair.first.Mark().column);
                }

                // Make sure it's not a duplicate key
                if (!processedFields.insert(fieldInfo.Name).second)
                {
                    errors.emplace_back(ManifestError::FieldDuplicate, fieldInfo.Name, "", keyValuePair.first.Mark().line, keyValuePair.first.Mark().column);
                }

                // Validate non empty value is provided for required fields
                if (fieldInfo.Required)
                {
                    if (!valueNode.IsDefined() || valueNode.IsNull() ||  // Should be defined and not null
                        (valueNode.IsScalar() && valueNode.as<std::string>().empty()) ||  // Scalar type should have content
                        ((valueNode.IsMap() || valueNode.IsSequence()) && valueNode.size() == 0))  // Map or sequence type should have size greater than 0
                    {
                        errors.emplace_back(ManifestError::RequiredFieldEmpty, fieldInfo.Name, "", valueNode.Mark().line, valueNode.Mark().column);
                    }
                }

                // Validate value against regex if applicable
                if (fullValidation && !fieldInfo.RegEx.empty())
                {
                    std::string value = valueNode.as<std::string>();
                    std::regex pattern{ fieldInfo.RegEx };
                    if (!std::regex_match(value, pattern))
                    {
                        errors.emplace_back(ManifestError::InvalidFieldValue, fieldInfo.Name, value, valueNode.Mark().line, valueNode.Mark().column);
                        continue;
                    }
                }

                if (!valueNode.IsNull())
                {
                    fieldInfo.ProcessFunc(valueNode);
                }
            }
            else
            {
                // For full validation, also reports unrecognized fields as warning
                if (fullValidation)
                {
                    errors.emplace_back(ManifestError::FieldUnknown, key, "", keyValuePair.first.Mark().line, keyValuePair.first.Mark().column, ValidationError::Level::Warning);
                }
            }
        }

        // Make sure required fields are provided
        for (auto const& fieldInfo : fieldInfos)
        {
            if (fieldInfo.Required && processedFields.find(fieldInfo.Name) == processedFields.end())
            {
                errors.emplace_back(ManifestError::RequiredFieldMissing, fieldInfo.Name);
            }
        }

        return errors;
    }

    std::map<ManifestInstaller::InstallerSwitchType, ManifestInstaller::string_t> YamlParser::GetDefaultKnownSwitches(
        ManifestInstaller::InstallerTypeEnum installerType)
    {
        switch (installerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Wix:
        case ManifestInstaller::InstallerTypeEnum::Msi:
            return
            {
                {ManifestInstaller::InstallerSwitchType::Silent, ManifestInstaller::string_t("/quiet")},
                {ManifestInstaller::InstallerSwitchType::SilentWithProgress, ManifestInstaller::string_t("/passive")},
                {ManifestInstaller::InstallerSwitchType::Log, ManifestInstaller::string_t("/log \"" + std::string(ARG_TOKEN_LOGPATH) + "\"")},
                {ManifestInstaller::InstallerSwitchType::InstallLocation, ManifestInstaller::string_t("TARGETDIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"")},
                {ManifestInstaller::InstallerSwitchType::Update, ManifestInstaller::string_t("REINSTALL=ALL REINSTALLMODE=vamus")}
            };
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
            return
            {
                {ManifestInstaller::InstallerSwitchType::Silent, ManifestInstaller::string_t("/S")},
                {ManifestInstaller::InstallerSwitchType::SilentWithProgress, ManifestInstaller::string_t("/S")},
                {ManifestInstaller::InstallerSwitchType::InstallLocation, ManifestInstaller::string_t("/D=" + std::string(ARG_TOKEN_INSTALLPATH))}
            };
        case ManifestInstaller::InstallerTypeEnum::Inno:
            return
            {
                {ManifestInstaller::InstallerSwitchType::Silent, ManifestInstaller::string_t("/VERYSILENT")},
                {ManifestInstaller::InstallerSwitchType::SilentWithProgress, ManifestInstaller::string_t("/SILENT")},
                {ManifestInstaller::InstallerSwitchType::Log, ManifestInstaller::string_t("/LOG=\"" + std::string(ARG_TOKEN_LOGPATH) + "\"")},
                {ManifestInstaller::InstallerSwitchType::InstallLocation, ManifestInstaller::string_t("/DIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"")}
            };
        default:
            return {};
        }
    }
}