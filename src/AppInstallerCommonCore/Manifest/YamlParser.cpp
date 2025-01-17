// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerSHA256.h"
#include "winget/Yaml.h"
#include "winget/ManifestSchemaValidation.h"
#include "winget/ManifestYamlPopulator.h"
#include "winget/ManifestYamlParser.h"

namespace AppInstaller::Manifest::YamlParser
{
    namespace
    {
        // Basic V1 manifest required fields check for later manifest consistency check
        void ValidateV1ManifestInput(const YamlManifestInfo& entry)
        {
            std::vector<ValidationError> errors;

            if (!entry.Root.IsMap())
            {
                THROW_EXCEPTION_MSG(ManifestException(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST), "The manifest does not contain a valid root. File: %hs", entry.FileName.c_str());
            }

            if (!entry.Root["PackageIdentifier"])
            {
                errors.emplace_back(ValidationError::MessageContextWithFile(
                    ManifestError::RequiredFieldMissing, "PackageIdentifier", entry.FileName));
            }

            if (!entry.Root["PackageVersion"])
            {
                errors.emplace_back(ValidationError::MessageContextWithFile(
                    ManifestError::RequiredFieldMissing, "PackageVersion", entry.FileName));
            }

            if (!entry.Root["ManifestVersion"])
            {
                errors.emplace_back(ValidationError::MessageContextWithFile(
                    ManifestError::RequiredFieldMissing, "ManifestVersion", entry.FileName));
            }

            if (!entry.Root["ManifestType"])
            {
                errors.emplace_back(ValidationError::MessageContextWithFile(
                    ManifestError::InconsistentMultiFileManifestFieldValue, "ManifestType", entry.FileName));
            }
            else
            {
                auto manifestType = ConvertToManifestTypeEnum(entry.Root["ManifestType"].as<std::string>());

                switch (manifestType)
                {
                case ManifestTypeEnum::Version:
                    if (!entry.Root["DefaultLocale"])
                    {
                        errors.emplace_back(ValidationError::MessageContextWithFile(
                            ManifestError::RequiredFieldMissing, "DefaultLocale", entry.FileName));
                    }
                    break;
                case ManifestTypeEnum::Singleton:
                case ManifestTypeEnum::Locale:
                case ManifestTypeEnum::DefaultLocale:
                    if (!entry.Root["PackageLocale"])
                    {
                        errors.emplace_back(ValidationError::MessageContextWithFile(
                            ManifestError::RequiredFieldMissing, "PackageLocale", entry.FileName));
                    }
                    break;
                }
            }

            if (!errors.empty())
            {
                ManifestException ex{ std::move(errors) };
                THROW_EXCEPTION(ex);
            }
        }

        // Input validations:
        // - Determine manifest version
        // - Check multi file manifest input integrity
        //   - All manifests use same PackageIdentifier, PackageVersion, ManifestVersion
        //   - All required types exist and exist only once. i.e. version, installer, defaultLocale
        //   - No duplicate locales across manifests
        //   - DefaultLocale matches in version manifest and defaultLocale manifest
        // - Validate manifest type correctness
        //   - Allowed file type in multi file manifest: version, installer, defaultLocale, locale
        //   - Allowed file type in single file manifest: preview manifest, merged and singleton
        ManifestVer ValidateInput(std::vector<YamlManifestInfo>& input, ManifestValidateOption validateOption)
        {
            std::vector<ValidationError> errors;

            std::string manifestVersionStr;
            ManifestVer manifestVersion;
            ManifestVer ManifestVersionV1{ s_ManifestVersionV1 };
            bool isMultifileManifest = input.size() > 1;

            // Use the first manifest doc to determine ManifestVersion, there'll be checks for manifest version consistency later
            auto& firstYamlManifest = input[0];
            if (!firstYamlManifest.Root.IsMap())
            {
                THROW_EXCEPTION_MSG(ManifestException(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST), "The manifest does not contain a valid root. File: %hs", firstYamlManifest.FileName.c_str());
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
                THROW_EXCEPTION_MSG(ManifestException(APPINSTALLER_CLI_ERROR_UNSUPPORTED_MANIFESTVERSION), "Unsupported ManifestVersion: %hs", manifestVersion.ToString().c_str());
            }

            // Preview manifest validations
            if (manifestVersion < ManifestVersionV1)
            {
                // multi file manifest is only supported starting ManifestVersion 1.0.0
                if (isMultifileManifest)
                {
                    THROW_EXCEPTION_MSG(ManifestException(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST), "Preview manifest does not support multi file manifest format.");
                }

                firstYamlManifest.ManifestType = ManifestTypeEnum::Preview;
            }
            // V1 manifest validations
            else
            {
                // Check required fields used by later consistency check for better error message instead of
                // Field Type Not Match error.
                for (auto const& entry : input)
                {
                    ValidateV1ManifestInput(entry);
                }

                if (isMultifileManifest)
                {
                    // Populates the PackageIdentifier and PackageVersion from first doc for later consistency check
                    std::string packageId = firstYamlManifest.Root["PackageIdentifier"].as<std::string>();
                    std::string packageVersion = firstYamlManifest.Root["PackageVersion"].as<std::string>();

                    std::set<std::string> localesSet;

                    bool isVersionManifestFound = false;
                    bool isInstallerManifestFound = false;
                    bool isDefaultLocaleManifestFound = false;
                    bool isShadowManifestFound = false;
                    std::string defaultLocaleFromVersionManifest;
                    std::string defaultLocaleFromDefaultLocaleManifest;

                    for (auto& entry : input)
                    {
                        std::string localPackageId = entry.Root["PackageIdentifier"].as<std::string>();
                        if (localPackageId != packageId)
                        {
                            errors.emplace_back(ValidationError::MessageContextValueWithFile(
                                ManifestError::InconsistentMultiFileManifestFieldValue, "PackageIdentifier", localPackageId, entry.FileName));
                        }

                        std::string localPackageVersion = entry.Root["PackageVersion"].as<std::string>();
                        if (localPackageVersion != packageVersion)
                        {
                            errors.emplace_back(ValidationError::MessageContextValueWithFile(
                                ManifestError::InconsistentMultiFileManifestFieldValue, "PackageVersion", localPackageVersion, entry.FileName));
                        }

                        std::string localManifestVersion = entry.Root["ManifestVersion"].as<std::string>();
                        if (localManifestVersion != manifestVersionStr)
                        {
                            errors.emplace_back(ValidationError::MessageContextValueWithFile(
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
                                errors.emplace_back(ValidationError::MessageContextValueWithFile(
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
                                errors.emplace_back(ValidationError::MessageContextValueWithFile(
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
                                errors.emplace_back(ValidationError::MessageContextValueWithFile(
                                    ManifestError::DuplicateMultiFileManifestType, "ManifestType", manifestTypeStr, entry.FileName));
                            }
                            else
                            {
                                isDefaultLocaleManifestFound = true;
                                auto packageLocale = entry.Root["PackageLocale"sv].as<std::string>();
                                defaultLocaleFromDefaultLocaleManifest = packageLocale;

                                if (localesSet.find(packageLocale) != localesSet.end())
                                {
                                    errors.emplace_back(ValidationError::MessageContextValueWithFile(
                                        ManifestError::DuplicateMultiFileManifestLocale, "PackageLocale", packageLocale, entry.FileName));
                                }
                                else
                                {
                                    localesSet.insert(packageLocale);
                                }
                            }
                            break;
                        case ManifestTypeEnum::Locale:
                        {
                            auto packageLocale = entry.Root["PackageLocale"sv].as<std::string>();
                            if (localesSet.find(packageLocale) != localesSet.end())
                            {
                                errors.emplace_back(ValidationError::MessageContextValueWithFile(
                                    ManifestError::DuplicateMultiFileManifestLocale, "PackageLocale", packageLocale, entry.FileName));
                            }
                            else
                            {
                                localesSet.insert(packageLocale);
                            }
                            break;
                        }
                        case ManifestTypeEnum::Shadow:
                        {
                            if (!validateOption.AllowShadowManifest)
                            {
                                errors.emplace_back(ValidationError::MessageContextValueWithFile(
                                    ManifestError::ShadowManifestNotAllowed, "ManifestType", manifestTypeStr, entry.FileName));
                            }
                            else if (isShadowManifestFound)
                            {
                                errors.emplace_back(ValidationError::MessageContextValueWithFile(
                                    ManifestError::DuplicateMultiFileManifestType, "ManifestType", manifestTypeStr, entry.FileName));
                            }
                            else
                            {
                                isShadowManifestFound = true;
                            }
                            break;
                        }
                        default:
                            errors.emplace_back(ValidationError::MessageContextValueWithFile(
                                ManifestError::UnsupportedMultiFileManifestType, "ManifestType", manifestTypeStr, entry.FileName));
                        }
                    }

                    if (isVersionManifestFound && isDefaultLocaleManifestFound && defaultLocaleFromDefaultLocaleManifest != defaultLocaleFromVersionManifest)
                    {
                        errors.emplace_back(ManifestError::InconsistentMultiFileManifestDefaultLocale);
                    }

                    if (!validateOption.SchemaValidationOnly && !(isVersionManifestFound && isInstallerManifestFound && isDefaultLocaleManifestFound))
                    {
                        errors.emplace_back(ManifestError::IncompleteMultiFileManifest);
                    }
                }
                else
                {
                    std::string manifestTypeStr = firstYamlManifest.Root["ManifestType"sv].as<std::string>();
                    ManifestTypeEnum manifestType = ConvertToManifestTypeEnum(manifestTypeStr);
                    firstYamlManifest.ManifestType = manifestType;

                    if (validateOption.FullValidation && manifestType == ManifestTypeEnum::Merged)
                    {
                        errors.emplace_back(ValidationError::MessageContextValueWithFile(ManifestError::FieldValueNotSupported, "ManifestType", manifestTypeStr, firstYamlManifest.FileName));
                    }

                    if (!validateOption.SchemaValidationOnly && manifestType != ManifestTypeEnum::Merged && manifestType != ManifestTypeEnum::Singleton)
                    {
                        errors.emplace_back(ValidationError::MessageWithFile(ManifestError::IncompleteMultiFileManifest, firstYamlManifest.FileName));
                    }
                }
            }

            if (!errors.empty())
            {
                ManifestException ex{ std::move(errors) };
                THROW_EXCEPTION(ex);
            }

            return manifestVersion;
        }

        // Find a unique required manifest from the input in multi manifest case
        const YAML::Node& FindUniqueRequiredDocFromMultiFileManifest(const std::vector<YamlManifestInfo>& input, ManifestTypeEnum manifestType)
        {
            auto iter = std::find_if(input.begin(), input.end(),
                [=](auto const& s)
                {
                    return s.ManifestType == manifestType;
                });

            THROW_HR_IF(E_UNEXPECTED, iter == input.end());

            return iter->Root;
        }

        std::optional<YAML::Node> FindUniqueOptionalDocFromMultiFileManifest(std::vector<YamlManifestInfo>& input, ManifestTypeEnum manifestType)
        {
            auto iter = std::find_if(input.begin(), input.end(),
                [=](auto const& s)
                {
                    return s.ManifestType == manifestType;
                });

            if (iter != input.end())
            {
                return iter->Root;
            }

            return {};
        }

        // Merge one manifest file to the final merged manifest, basically copying the mapping but excluding certain common fields
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
            emitter.Emit(outFileStream);
            outFileStream.close();
        }

        std::vector<ValidationError> ParseManifestImpl(
            std::vector<YamlManifestInfo>& input,
            Manifest& manifest,
            const std::filesystem::path& mergedManifestPath,
            ManifestValidateOption validateOption)
        {
            THROW_HR_IF_MSG(E_INVALIDARG, input.size() == 0, "No manifest file found");
            THROW_HR_IF_MSG(E_INVALIDARG, validateOption.SchemaValidationOnly && !mergedManifestPath.empty(), "Manifest cannot be merged if only schema validation is performed");
            THROW_HR_IF_MSG(E_INVALIDARG, input.size() == 1 && !mergedManifestPath.empty(), "Manifest cannot be merged from a single manifest");

            auto manifestVersion = ValidateInput(input, validateOption);

            std::vector<ValidationError> resultErrors;

            if (validateOption.FullValidation || validateOption.SchemaValidationOnly)
            {
                resultErrors = ValidateAgainstSchema(input, manifestVersion);
            }

            if (validateOption.SchemaValidationOnly)
            {
                return resultErrors;
            }

            // Merge manifests in multi file manifest case
            bool isMultiFile = input.size() > 1;
            YAML::Node& manifestDoc = input[0].Root;
            if (isMultiFile)
            {
                manifestDoc = MergeMultiFileManifest(input);
            }

            auto shadowNode = isMultiFile ? FindUniqueOptionalDocFromMultiFileManifest(input, ManifestTypeEnum::Shadow) : std::optional<YAML::Node>{};

            auto errors = ManifestYamlPopulator::PopulateManifest(manifestDoc, manifest, manifestVersion, validateOption, shadowNode);
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));

            // Extra semantic validations after basic validation and field population
            if (validateOption.FullValidation)
            {
                errors = ValidateManifest(manifest);
                std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));

                // Validate the schema header for manifest version 1.10 and above
                if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_10 })
                {
                    // Validate the schema header.
                    errors = ValidateYamlManifestsSchemaHeader(input, manifestVersion, validateOption.SchemaHeaderValidationAsWarning);
                    std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
                }
            }

            if (validateOption.InstallerValidation)
            {
                errors = ValidateManifestInstallers(manifest);
                std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            }

            // Output merged manifest if requested
            if (!mergedManifestPath.empty())
            {
                OutputYamlDoc(manifestDoc, mergedManifestPath);
            }

            // If there is only one input file, use its hash for the stream
            if (input.size() == 1)
            {
                manifest.StreamSha256 = std::move(input[0].StreamSha256);
            }

            return resultErrors;
        }
    }

    Manifest CreateFromPath(
        const std::filesystem::path& inputPath,
        ManifestValidateOption validateOption,
        const std::filesystem::path& mergedManifestPath)
    {
        std::vector<YamlManifestInfo> docList;

        try
        {
            if (std::filesystem::is_directory(inputPath))
            {
                for (const auto& file : std::filesystem::directory_iterator(inputPath))
                {
                    THROW_HR_IF_MSG(HRESULT_FROM_WIN32(ERROR_DIRECTORY_NOT_SUPPORTED), std::filesystem::is_directory(file.path()), "Subdirectory not supported in manifest path");

                    YamlManifestInfo manifestInfo;
                    YAML::Document doc = YAML::LoadDocument(file.path());
                    manifestInfo.Root = std::move(doc).GetRoot();
                    manifestInfo.DocumentSchemaHeader = doc.GetSchemaHeader();
                    manifestInfo.FileName = file.path().filename().u8string();
                    docList.emplace_back(std::move(manifestInfo));
                }
            }
            else
            {
                YamlManifestInfo manifestInfo;
                YAML::Document doc = YAML::LoadDocument(inputPath, manifestInfo.StreamSha256);
                manifestInfo.Root = std::move(doc).GetRoot();
                manifestInfo.DocumentSchemaHeader = doc.GetSchemaHeader();
                manifestInfo.FileName = inputPath.filename().u8string();
                docList.emplace_back(std::move(manifestInfo));
            }
        }
        catch (const std::exception& e)
        {
            THROW_EXCEPTION_MSG(ManifestException(), "%hs", e.what());
        }

        return ParseManifest(docList, validateOption, mergedManifestPath);
    }

    Manifest Create(
        const std::string& input,
        ManifestValidateOption validateOption,
        const std::filesystem::path& mergedManifestPath)
    {
        std::vector<YamlManifestInfo> docList;

        try
        {
            YamlManifestInfo manifestInfo;
            YAML::Document doc = YAML::LoadDocument(input);
            manifestInfo.Root = std::move(doc).GetRoot();
            manifestInfo.DocumentSchemaHeader = doc.GetSchemaHeader();
            docList.emplace_back(std::move(manifestInfo));
        }
        catch (const std::exception& e)
        {
            THROW_EXCEPTION_MSG(ManifestException(), "%hs", e.what());
        }

        return ParseManifest(docList, validateOption, mergedManifestPath);
    }

    Manifest ParseManifest(
        std::vector<YamlManifestInfo>& input,
        ManifestValidateOption validateOption,
        const std::filesystem::path& mergedManifestPath)
    {
        Manifest manifest;
        std::vector<ValidationError> errors;

        try
        {
            errors = ParseManifestImpl(input, manifest, mergedManifestPath, validateOption);
        }
        catch (const ManifestException&)
        {
            // Prevent ManifestException from being wrapped in another ManifestException
            throw;
        }
        catch (const std::exception& e)
        {
            THROW_EXCEPTION_MSG(ManifestException(), "%hs", e.what());
        }

        if (!errors.empty())
        {
            ManifestException ex{ std::move(errors) };

            if (validateOption.ThrowOnWarning || !ex.IsWarningOnly())
            {
                THROW_EXCEPTION(ex);
            }
        }

        return manifest;
    }
}
