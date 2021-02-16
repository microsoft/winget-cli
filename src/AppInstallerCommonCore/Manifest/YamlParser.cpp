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
        ManifestVer ValidateInput(std::vector<YamlManifestInfo>& input, bool fullValidation, bool schemaValidationOnly)
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

                std::set<std::string> localesSet;

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
                            auto packageLocale = entry.Root["PackageLocale"sv].as<std::string>();
                            defaultLocaleFromDefaultLocaleManifest = packageLocale;

                            if (localesSet.find(packageLocale) != localesSet.end())
                            {
                                errors.emplace_back(ValidationError::MessageFieldValueWithFile(
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
                                errors.emplace_back(ValidationError::MessageFieldValueWithFile(
                                    ManifestError::DuplicateMultiFileManifestLocale, "PackageLocale", packageLocale, entry.FileName));
                            }
                            else
                            {
                                localesSet.insert(packageLocale);
                            }
                        }
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

                if (!schemaValidationOnly && !(isVersionManifestFound && isInstallerManifestFound && isDefaultLocaleManifestFound))
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

                    if (!schemaValidationOnly && manifestType != ManifestTypeEnum::Merged && manifestType != ManifestTypeEnum::Singleton)
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
            bool fullValidation,
            const std::filesystem::path& mergedManifestPath,
            bool schemaValidationOnly)
        {
            THROW_HR_IF_MSG(E_INVALIDARG, input.size() == 0, "No manifest file found");
            THROW_HR_IF_MSG(E_INVALIDARG, schemaValidationOnly && !mergedManifestPath.empty(), "Manifest cannot be merged if only schema validation is performed");
            THROW_HR_IF_MSG(E_INVALIDARG, input.size() == 1 && !mergedManifestPath.empty(), "Manifest cannot be merged from a single manifest");

            auto manifestVersion = ValidateInput(input, fullValidation, schemaValidationOnly);

            std::vector<ValidationError> resultErrors;

            if (fullValidation || schemaValidationOnly)
            {
                resultErrors = ValidateAgainstSchema(input, manifestVersion);
            }

            if (schemaValidationOnly)
            {
                return resultErrors;
            }

            // Merge manifests in multi file manifest case
            const YAML::Node& manifestDoc = (input.size() > 1) ? MergeMultiFileManifest(input) : input[0].Root;

            auto errors = ManifestYamlPopulator::PopulateManifest(manifestDoc, manifest, manifestVersion, fullValidation);
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));

            // Extra semantic validations after basic validation and field population
            if (fullValidation)
            {
                errors = ValidateManifest(manifest);
                std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            }

            // Output merged manifest if requested
            if (!mergedManifestPath.empty())
            {
                OutputYamlDoc(manifestDoc, mergedManifestPath);
            }

            return resultErrors;
        }
    }

    Manifest CreateFromPath(
        const std::filesystem::path& inputPath,
        bool fullValidation,
        bool throwOnWarning,
        const std::filesystem::path& mergedManifestPath,
        bool schemaValidationOnly)
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

        return ParseManifest(docList, fullValidation, throwOnWarning, mergedManifestPath, schemaValidationOnly);
    }

    Manifest Create(
        const std::string& input,
        bool fullValidation,
        bool throwOnWarning,
        const std::filesystem::path& mergedManifestPath,
        bool schemaValidationOnly)
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

        return ParseManifest(docList, fullValidation, throwOnWarning, mergedManifestPath, schemaValidationOnly);
    }

    Manifest ParseManifest(
        std::vector<YamlManifestInfo>& input,
        bool fullValidation,
        bool throwOnWarning,
        const std::filesystem::path& mergedManifestPath,
        bool schemaValidationOnly)
    {
        Manifest manifest;
        std::vector<ValidationError> errors;

        try
        {
            errors = ParseManifestImpl(input, manifest, fullValidation, mergedManifestPath, schemaValidationOnly);
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
}