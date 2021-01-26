// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerSHA256.h"
#include "winget/Yaml.h"
#include "winget/ManifestYamlParser.h"
#include "winget/ManifestSchemaValidation.h"

namespace AppInstaller::Manifest
{
    namespace
    {
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

    Manifest YamlParser::CreateFromPath(const std::filesystem::path& inputPath, bool fullValidation, bool throwOnWarning)
    {
        std::vector<YamlManifestInfo> docList;

        try
        {
            if (std::filesystem::is_directory(inputPath))
            {
                for (const auto& file : std::filesystem::directory_iterator(inputPath))
                {
                    THROW_HR_IF_MSG(HRESULT_FROM_WIN32(ERROR_DIRECTORY_NOT_SUPPORTED), std::filesystem::is_directory(file.path()), "Subdirectory not supported in manifest path");

                    YamlManifestInfo doc;
                    doc.Root = YAML::Load(file.path());
                    doc.FileName = file.path().filename().u8string();
                    docList.emplace_back(std::move(doc));
                }
            }
            else
            {
                YamlManifestInfo doc;
                doc.Root = YAML::Load(inputPath);
                doc.FileName = inputPath.filename().u8string();
                docList.emplace_back(std::move(doc));
            }
        }
        catch (const std::exception& e)
        {
            THROW_EXCEPTION_MSG(ManifestException(), e.what());
        }

        return CreateInternal(docList, fullValidation, throwOnWarning);
    }

    Manifest YamlParser::Create(const std::string& input, bool fullValidation, bool throwOnWarning)
    {
        std::vector<YamlManifestInfo> docList;

        try
        {
            YamlManifestInfo doc;
            doc.Root = YAML::Load(input);
            docList.emplace_back(std::move(doc));
        }
        catch (const std::exception& e)
        {
            THROW_EXCEPTION_MSG(ManifestException(), e.what());
        }

        return CreateInternal(docList, fullValidation, throwOnWarning);
    }

    Manifest YamlParser::CreateInternal(const std::vector<YamlManifestInfo>& input, bool fullValidation, bool throwOnWarning)
    {
        Manifest manifest;
        std::vector<ValidationError> errors;

        try
        {
            YamlParser parser;
            errors = parser.ParseManifest(input, manifest, fullValidation);
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

    ManifestVer YamlParser::ValidateInput(std::vector<YamlManifestInfo>& input, bool fullValidation, bool isPartialManifest)
    {
        std::vector<ValidationError> errors;

        std::string manifestVersionStr;
        ManifestVer manifestVersion;
        ManifestVer ManifestVersionV1{ s_ManifestVersionV1 };
        bool isMultifileManifest = input.size() > 1;

        // Use the first manifest doc to determine ManifestVersion
        auto& firstYamlManifest = input[0];
        if (!firstYamlManifest.Root.IsMap())
        {
            THROW_EXCEPTION_MSG(ManifestException(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST), "The manifest does not contain a valid root. File: %S", firstYamlManifest.FileName.c_str());
        }

        if (firstYamlManifest.Root["ManifestVersion"sv])
        {
            manifestVersionStr = firstYamlManifest.Root["ManifestVersion"sv].as<std::string>();
        }
        else
        {
            manifestVersionStr = s_DefaultManifestVersion;
        }
        manifestVersion = ManifestVer{ manifestVersionStr };

        // Check max supported version
        if (manifestVersion.Major() > s_MaxSupportedMajorVersion)
        {
            THROW_EXCEPTION_MSG(ManifestException(APPINSTALLER_CLI_ERROR_UNSUPPORTED_MANIFESTVERSION), "Unsupported ManifestVersion: %S", manifestVersion.ToString().c_str());
        }

        // multi file manifest is only supported starting ManifestVersion 1.0.0
        if (isMultifileManifest && manifestVersion < ManifestVersionV1)
        {
            THROW_EXCEPTION_MSG(ManifestException(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST), "Preview manifest does not support multi file manifest format.");
        }

        if (isMultifileManifest)
        {
            // Populates the PackageIdentifier and PackageVersion from first doc for later consistency check
            std::string packageId = firstYamlManifest.Root["PackageIdentifier"].as<std::string>();
            std::string packageVersion = firstYamlManifest.Root["PackageVersion"].as<std::string>();

            bool isVersionManifestFound = false;
            bool isInstallerManifestFound = false;
            bool isDefaultLocaleManifestFound = false;
            std::string defaultLocaleFromVersionManifest;
            std::string defaultLocaleFromDefaultLocaleManifest;

            for (auto& entry : input)
            {
                if (!entry.Root.IsMap())
                {
                    THROW_EXCEPTION_MSG(ManifestException(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST), "The manifest does not contain a valid root. File: %S", entry.FileName.c_str());
                }

                std::string localPackageId = entry.Root["PackageIdentifier"].as<std::string>();
                if (localPackageId != packageId)
                {
                    errors.emplace_back(ValidationError::MessageFieldValueWithFile(
                        ManifestError::InconsistentMultiFileManifestFieldValue, "PackageIdentifier", localPackageId, entry.FileName));
                }

                std::string localPackageVersion = entry.Root["PackageVersion"].as<std::string>();
                if (localPackageVersion != packageVersion)
                {
                    errors.emplace_back(ValidationError::MessageFieldValueWithFile(
                        ManifestError::InconsistentMultiFileManifestFieldValue, "PackageVersion", localPackageVersion, entry.FileName));
                }

                std::string localManifestVersion = entry.Root["ManifestVersion"].as<std::string>();
                if (localManifestVersion != manifestVersionStr)
                {
                    errors.emplace_back(ValidationError::MessageFieldValueWithFile(
                        ManifestError::InconsistentMultiFileManifestFieldValue, "ManifestVersion", localManifestVersion, entry.FileName));
                }

                std::string manifestTypeStr = entry.Root["ManifestType"sv].as<std::string>();
                ManifestTypeEnum manifestType = ConvertToManifestTypeEnum(manifestTypeStr);
                entry.ManifestType = manifestType;

                switch (manifestType)
                {
                case ManifestTypeEnum::Version:
                    if (isVersionManifestFound)
                    {
                        errors.emplace_back(ValidationError::MessageFieldValueWithFile(
                            ManifestError::DuplicateMultiFileManifestType, "ManifestType", manifestTypeStr, entry.FileName));
                    }
                    else
                    {
                        isVersionManifestFound = true;
                        defaultLocaleFromVersionManifest = entry.Root["DefaultLocale"sv].as<std::string>();
                    }
                    break;
                case ManifestTypeEnum::Installer:
                    if (isInstallerManifestFound)
                    {
                        errors.emplace_back(ValidationError::MessageFieldValueWithFile(
                            ManifestError::DuplicateMultiFileManifestType, "ManifestType", manifestTypeStr, entry.FileName));
                    }
                    else
                    {
                        isInstallerManifestFound = true;
                    }
                    break;
                case ManifestTypeEnum::DefaultLocale:
                    if (isDefaultLocaleManifestFound)
                    {
                        errors.emplace_back(ValidationError::MessageFieldValueWithFile(
                            ManifestError::DuplicateMultiFileManifestType, "ManifestType", manifestTypeStr, entry.FileName));
                    }
                    else
                    {
                        isDefaultLocaleManifestFound = true;
                        defaultLocaleFromDefaultLocaleManifest = entry.Root["PackageLocale"sv].as<std::string>();
                    }
                    break;
                case ManifestTypeEnum::Locale:
                    // Nothing to validate
                    break;
                default:
                    errors.emplace_back(ValidationError::MessageFieldValueWithFile(
                        ManifestError::UnsupportedMultiFileManifestType, "ManifestType", manifestTypeStr, entry.FileName));
                }
            }

            if (isVersionManifestFound && isDefaultLocaleManifestFound && defaultLocaleFromDefaultLocaleManifest != defaultLocaleFromVersionManifest)
            {
                errors.emplace_back(ManifestError::InconsistentMultiFileManifestDefaultLocale);
            }

            if (!isPartialManifest && !(isVersionManifestFound && isInstallerManifestFound && isDefaultLocaleManifestFound))
            {
                errors.emplace_back(ManifestError::IncompleteMultiFileManifest);
            }
        }
        else
        {
            if (manifestVersion >= ManifestVersionV1)
            {
                std::string manifestTypeStr = firstYamlManifest.Root["ManifestType"sv].as<std::string>();
                ManifestTypeEnum manifestType = ConvertToManifestTypeEnum(manifestTypeStr);
                firstYamlManifest.ManifestType = manifestType;

                if (fullValidation && manifestType == ManifestTypeEnum::Merged)
                {
                    errors.emplace_back(ValidationError::MessageFieldValueWithFile(ManifestError::FieldValueNotSupported, "ManifestType", manifestTypeStr, firstYamlManifest.FileName));
                }

                if (!isPartialManifest && manifestType != ManifestTypeEnum::Merged && manifestType != ManifestTypeEnum::Singleton)
                {
                    errors.emplace_back(ValidationError::MessageWithFile(ManifestError::IncompleteMultiFileManifest, firstYamlManifest.FileName));
                }
            }
            else
            {
                firstYamlManifest.ManifestType = ManifestTypeEnum::Preview;
            }
        }

        if (!errors.empty())
        {
            ManifestException ex{ std::move(errors) };
            THROW_EXCEPTION(ex);
        }

        return manifestVersion;
    }

    const YAML::Node& FindUniqueRequiredDocFromMultiFileManifest(const std::vector<YamlManifestInfo>& input, ManifestTypeEnum manifestType)
    {
        // We'll do case insensitive search first and validate correct case later.
        auto iter = std::find_if(input.begin(), input.end(),
            [](auto const& s)
            {
                return s.ManifestType == manifestType;
            });

        THROW_HR_IF(E_UNEXPECTED, iter == input.end());

        return iter->Root;
    }

    void MergeOneManifestToMultiFileManifest(const YAML::Node& input, YAML::Node& destination)
    {
        THROW_HR_IF(E_UNEXPECTED, !input.IsMap());
        THROW_HR_IF(E_UNEXPECTED, !destination.IsMap());

        const std::vector<std::string> FieldsToIgnore = { "PackageIdentifier", "PackageVersion", "ManifestType", "ManifestVersion" };

        for (auto const& keyValuePair : input.Mapping())
        {
            // We only support string type as key in our manifest
            if (std::find(FieldsToIgnore.begin(), FieldsToIgnore.end(), keyValuePair.first.as<std::string>()) == FieldsToIgnore.end())
            {
                YAML::Node key = keyValuePair.first;
                YAML::Node value = keyValuePair.second;
                destination.AddMappingNode(std::move(key), std::move(value));
            }
        }
    }

    YAML::Node MergeMultiFileManifest(const std::vector<YamlManifestInfo>& input)
    {
        // Starts with installer manifest
        YAML::Node result = FindUniqueRequiredDocFromMultiFileManifest(input, ManifestTypeEnum::Installer);

        // Copy default locale manifest content into manifest root
        YAML::Node defaultLocaleManifest = FindUniqueRequiredDocFromMultiFileManifest(input, ManifestTypeEnum::DefaultLocale);
        MergeOneManifestToMultiFileManifest(defaultLocaleManifest, result);

        // Copy additional locale manifests
        YAML::Node localizations{ YAML::Node::Type::Sequence, "", YAML::Mark() };
        for (const auto& entry : input)
        {
            if (entry.ManifestType == ManifestTypeEnum::Locale)
            {
                YAML::Node localization{ YAML::Node::Type::Mapping, "", YAML::Mark() };
                MergeOneManifestToMultiFileManifest(entry.Root, localization);
                localizations.AddSequenceNode(std::move(localization));
            }
        }

        if (localizations.size() > 0)
        {
            YAML::Node key{ YAML::Node::Type::Scalar, "", YAML::Mark() };
            key.SetScalar("Localization");
            result.AddMappingNode(std::move(key), std::move(localizations));
        }

        result["ManifestType"sv].SetScalar("merged");

        return result;
    }

    void EmitYamlNode(const YAML::Node& input, YAML::Emitter& emitter)
    {
        if (input.IsMap())
        {
            emitter << YAML::BeginMap;
            for (auto const& keyValuePair : input.Mapping())
            {
                emitter << YAML::Key;
                EmitYamlNode(keyValuePair.first, emitter);
                emitter << YAML::Value;
                EmitYamlNode(keyValuePair.second, emitter);
            }
            emitter << YAML::EndMap;
        }
        else if (input.IsSequence())
        {
            emitter << YAML::BeginSeq;
            for (auto const& value : input.Sequence())
            {
                EmitYamlNode(value, emitter);
            }
            emitter << YAML::EndSeq;
        }
        else if (input.IsScalar())
        {
            emitter << input.as<std::string>();
        }
        else if (input.IsNull())
        {
            emitter << "";
        }
        else
        {
            THROW_HR(E_UNEXPECTED);
        }
    }

    void OutputYamlDoc(const YAML::Node& input, const std::filesystem::path& out)
    {
        THROW_HR_IF(E_UNEXPECTED, !input.IsMap());

        YAML::Emitter emitter;
        EmitYamlNode(input, emitter);

        std::filesystem::create_directories(out.parent_path());
        std::ofstream outFileStream(out);
        emitter.Get(outFileStream);
        outFileStream.close();
    }

    std::vector<ValidationError> YamlParser::ParseManifest(const std::vector<YamlManifestInfo>& input, Manifest& manifest, bool fullValidation, bool isPartialManifest)
    {
        manifest.ManifestVersion = ValidateInput(input, fullValidation, isPartialManifest);

        auto resultErrors = ValidateAgainstSchema(input, manifest.ManifestVersion, NULL);

        if (isPartialManifest)
        {
            return resultErrors;
        }

        const YAML::Node& manifestDoc = (input.size() > 1) ? MergeMultiFileManifest(input) : input[0].Root;

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

        if (true)
        {
            OutputYamlDoc(manifestDoc, "merged.yaml");
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