// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Yaml.h"
#include "winget/JsonSchemaValidation.h"
#include "winget/ManifestCommon.h"
#include "winget/ManifestSchemaValidation.h"
#include "winget/ManifestYamlParser.h"
#include "winget/Resources.h"

#include <ManifestSchema.h>

namespace AppInstaller::Manifest::YamlParser
{
    using namespace std::string_view_literals;

    namespace
    {
        constexpr std::string_view YamlLanguageServerKey = "yaml-language-server"sv;

        struct ManifestSchemaHeader
        {
            std::string SchemaHeaderString;
            std::string ManifestType;
            std::string ManifestVersion;
            std::string FileName;
            size_t Line;
            size_t column;
        };

        enum class YamlScalarType
        {
            String,
            Int,
            Bool
        };

        // List of fields that use non string scalar types
        const std::map<std::string_view, YamlScalarType> ManifestFieldTypes =
        {
            { "InstallerSuccessCodes"sv, YamlScalarType::Int },
            { "InstallerAbortsTerminal"sv, YamlScalarType::Bool },
            { "InstallLocationRequired"sv, YamlScalarType::Bool },
            { "RequireExplicitUpgrade"sv, YamlScalarType::Bool },
            { "DisplayInstallWarnings"sv, YamlScalarType::Bool },
            { "InstallerReturnCode"sv, YamlScalarType::Int },
            { "DownloadCommandProhibited", YamlScalarType::Bool },
            { "ArchiveBinariesDependOnPath", YamlScalarType::Bool }
        };

        YamlScalarType GetManifestScalarValueType(const std::string& key)
        {
            auto iter = ManifestFieldTypes.find(key);
            if (iter != ManifestFieldTypes.end())
            {
                return iter->second;
            }

            return YamlScalarType::String;
        }

        Json::Value YamlScalarNodeToJson(const YAML::Node& scalarNode, YamlScalarType scalarType)
        {
            if (scalarType == YamlScalarType::Int)
            {
                return Json::Value(scalarNode.as<int>());
            }
            else if (scalarType == YamlScalarType::Bool)
            {
                return Json::Value(scalarNode.as<bool>());
            }
            else
            {
                return Json::Value(scalarNode.as<std::string>());
            }
        }

        Json::Value ManifestYamlNodeToJson(const YAML::Node& rootNode, YamlScalarType scalarType = YamlScalarType::String)
        {
            Json::Value result;

            if (rootNode.IsNull())
            {
                result = Json::Value::nullSingleton();
            }
            else if (rootNode.IsMap())
            {
                for (auto const& keyValuePair : rootNode.Mapping())
                {
                    // We only support string type as key in our manifest
                    auto key = keyValuePair.first.as<std::string>();
                    result[keyValuePair.first.as<std::string>()] = ManifestYamlNodeToJson(keyValuePair.second, GetManifestScalarValueType(key));
                }
            }
            else if (rootNode.IsSequence())
            {
                for (auto const& value : rootNode.Sequence())
                {
                    result.append(ManifestYamlNodeToJson(value, scalarType));
                }
            }
            else if (rootNode.IsScalar())
            {
                result = YamlScalarNodeToJson(rootNode, scalarType);
            }
            else
            {
                THROW_HR(E_UNEXPECTED);
            }

            return result;
        }

        bool SearchForManifestSchemaHeaderString(std::shared_ptr<std::istream> yamlInputStream, const size_t& rootNodeBeginsAtLine, ManifestSchemaHeader& schemaHeader)
        {
            std::string line;
            size_t currentLine = 1;
            schemaHeader.SchemaHeaderString.clear();

            // Search for the schema header string in the comments before the root node.
            while (currentLine < rootNodeBeginsAtLine && std::getline(*yamlInputStream, line))
            {
                std::string comment = Utility::Trim(line);

                // Check if the line is a comment
                if (!comment.empty() && comment[0] == '#')
                {
                    size_t pos = comment.find(YamlLanguageServerKey);

                    // Check if the comment contains the schema header string
                    if (pos != std::string::npos)
                    {
                        schemaHeader.SchemaHeaderString = std::move(comment);
                        schemaHeader.Line = currentLine;
                        schemaHeader.column = pos;

                        return true;
                    }
                }

                currentLine++;
            }

            return false;
        }

        std::vector<ValidationError> ParseSchemaHeaderString(const ManifestSchemaHeader& manifestSchemaHeader, const ValidationError::Level& errorLevel, std::string& schemaHeaderUrlString)
        {
            std::vector<ValidationError> errors;
            std::string schemaHeader = manifestSchemaHeader.SchemaHeaderString;

            // Remove the leading '#' and any leading/trailing whitespaces
            if (schemaHeader[0] == '#')
            {
                schemaHeader = schemaHeader.substr(1); // Remove the leading '#'
                schemaHeader = Utility::Trim(schemaHeader); // Trim leading/trailing whitespaces
            }

            // Parse the schema header string as YAML string to get the schema header URL

            try
            {
                auto root = YAML::Load(schemaHeader);

                if (root.IsNull() || (!root.IsNull() && !root.IsDefined()))
                {
                    errors.emplace_back(ValidationError::MessageContextValueLineLevelWithFile(ManifestError::InvalidSchemaHeader, "", schemaHeader, manifestSchemaHeader.Line, manifestSchemaHeader.column, errorLevel, manifestSchemaHeader.FileName));
                }
                else
                {
                    schemaHeaderUrlString = root[YamlLanguageServerKey].as<std::string>();
                }
            }
            catch (const YAML::Exception&)
            {
                errors.emplace_back(ValidationError::MessageContextValueLineLevelWithFile(ManifestError::InvalidSchemaHeader, "", schemaHeader, manifestSchemaHeader.Line, manifestSchemaHeader.column, errorLevel, manifestSchemaHeader.FileName));
            }
            catch (const std::exception&)
            {
                errors.emplace_back(ValidationError::MessageContextValueLineLevelWithFile(ManifestError::InvalidSchemaHeader, "", schemaHeader, manifestSchemaHeader.Line, manifestSchemaHeader.column, errorLevel, manifestSchemaHeader.FileName));
            }

            return errors;
        }

        bool ParseSchemaHeaderUrl(const std::string& schemaHeaderValue, std::string& schemaType, std::string& schemaVersion)
        {
            // Use regex to match the pattern of @"winget-manifest\.(?<type>\w+)\.(?<version>[\d\.]+)\.schema\.json$"
            std::regex schemaUrlPattern(R"(winget-manifest\.(\w+)\.([\d\.]+)\.schema\.json$)");
            std::smatch match;

            if (std::regex_search(schemaHeaderValue, match, schemaUrlPattern))
            {
                schemaType = match[1].str();
                schemaVersion = match[2].str();
                return true;
            }

            return false;
        }

        std::vector<ValidationError> ValidateManifestSchemaHeaderDetails(const ManifestSchemaHeader& schemaHeader, const ManifestTypeEnum& expectedManifestType, const ManifestVer& expectedManifestVersion, ValidationError::Level errorLevel)
        {
            std::vector<ValidationError> errors;
            ManifestTypeEnum actualManifestType = ConvertToManifestTypeEnum(schemaHeader.ManifestType);
            ManifestVer actualHeaderVersion(schemaHeader.ManifestVersion);

            size_t actualManifestTypeIndex = schemaHeader.SchemaHeaderString.find(schemaHeader.ManifestType) + 1;
            size_t actualHeaderVersionIndex = schemaHeader.SchemaHeaderString.find(schemaHeader.ManifestVersion) + 1;

            if (actualManifestType != expectedManifestType)
            {
                errors.emplace_back(ValidationError::MessageContextValueLineLevelWithFile(ManifestError::SchemaHeaderManifestTypeMismatch, "", schemaHeader.ManifestType, schemaHeader.Line, actualManifestTypeIndex, errorLevel, schemaHeader.FileName));
            }

            if (actualHeaderVersion != expectedManifestVersion)
            {
                errors.emplace_back(ValidationError::MessageContextValueLineLevelWithFile(ManifestError::SchemaHeaderManifestVersionMismatch, "", schemaHeader.ManifestVersion, schemaHeader.Line, actualHeaderVersionIndex, errorLevel, schemaHeader.FileName));
            }

            return errors;
        }

        bool IsValidSchemaHeaderUrl(const std::string& schemaHeaderUrlString, const YamlManifestInfo& manifestInfo, const ManifestVer& manifestVersion)
        {
            // Load the schema file to compare the schema header URL with the schema ID in the schema file
            Json::Value schemaFile = LoadSchemaDoc(manifestVersion, manifestInfo.ManifestType);

            if (schemaFile.isMember("$id"))
            {
                std::string schemaId = schemaFile["$id"].asString();

                // Prefix schema ID with "schema=" to match the schema header URL pattern and compare it with the schema header URL
                schemaId = "$schema=" + schemaId;

                if (std::equal(schemaId.begin(), schemaId.end(), schemaHeaderUrlString.begin(), schemaHeaderUrlString.end(),
                    [](char a, char b) { return tolower(a) == tolower(b); }))
                {
                    return true;
                }
            }

            return false;
        }

        ValidationError GetSchemaHeaderUrlPatternMismatchError(const std::string& schemaHeaderUrlString,const ManifestSchemaHeader& manifestSchemaHeader, const ValidationError::Level& errorLevel)
        {
            size_t schemaHeaderUrlIndex = manifestSchemaHeader.SchemaHeaderString.find(schemaHeaderUrlString) + 1;

            return ValidationError::MessageContextValueLineLevelWithFile(ManifestError::SchemaHeaderUrlPatternMismatch, "", manifestSchemaHeader.SchemaHeaderString, manifestSchemaHeader.Line, schemaHeaderUrlIndex, errorLevel, manifestSchemaHeader.FileName);
        }

        std::vector<ValidationError> ValidateSchemaHeaderUrl(const YamlManifestInfo& manifestInfo, const ManifestVer& manifestVersion, ManifestSchemaHeader& manifestSchemaHeader, const ValidationError::Level& errorLevel)
        {
            std::vector<ValidationError> errors;

            std::string schemaHeaderUrlString;
            // Parse the schema header string to get the schema header URL
            auto parserErrors = ParseSchemaHeaderString(manifestSchemaHeader, errorLevel, schemaHeaderUrlString);
            std::move(parserErrors.begin(), parserErrors.end(), std::inserter(errors, errors.end()));

            if (!errors.empty())
            {
                return errors;
            }

            std::string manifestTypeString;
            std::string manifestVersionString;

            // Parse the schema header URL to get the manifest type and version
            if (ParseSchemaHeaderUrl(schemaHeaderUrlString, manifestTypeString, manifestVersionString))
            {
                manifestSchemaHeader.ManifestType = std::move(manifestTypeString);
                manifestSchemaHeader.ManifestVersion = std::move(manifestVersionString);

                auto compareErrors = ValidateManifestSchemaHeaderDetails(manifestSchemaHeader, manifestInfo.ManifestType, manifestVersion, errorLevel);
                std::move(compareErrors.begin(), compareErrors.end(), std::inserter(errors, errors.end()));

                // Finally, match the entire schema header URL with the schema ID in the schema file to ensure the URL domain matches the schema definition file.
                if (!IsValidSchemaHeaderUrl(schemaHeaderUrlString, manifestInfo, manifestVersion))
                {
                    errors.emplace_back(GetSchemaHeaderUrlPatternMismatchError(schemaHeaderUrlString, manifestSchemaHeader, errorLevel));
                }
            }
            else
            {
                errors.emplace_back(GetSchemaHeaderUrlPatternMismatchError(schemaHeaderUrlString, manifestSchemaHeader, errorLevel));
            }

            return errors;
        }

        std::vector<ValidationError> ValidateYamlManifestSchemaHeader(const YamlManifestInfo& manifestInfo, const ManifestVer& manifestVersion, const ValidationError::Level& errorLevel)
        {
            std::vector<ValidationError> errors;

            if (manifestInfo.ManifestType == ManifestTypeEnum::Shadow)
            {
                // There's no schema for a shadow manifest.
                return errors;
            }

            size_t rootNodeLine = manifestInfo.Root.Mark().line;
            std::string schemaHeaderString;

            ManifestSchemaHeader schemaHeader{};
            schemaHeader.FileName = manifestInfo.FileName;

            if (!SearchForManifestSchemaHeaderString(manifestInfo.InputStream, rootNodeLine, schemaHeader))
            {
                errors.emplace_back(ValidationError::MessageLevelWithFile(ManifestError::SchemaHeaderNotFound, errorLevel, manifestInfo.FileName));
                return errors;
            }

            auto parserErrors = ValidateSchemaHeaderUrl(manifestInfo, manifestVersion, schemaHeader, errorLevel);
            std::move(parserErrors.begin(), parserErrors.end(), std::inserter(errors, errors.end()));

            return errors;
        }
    }

    Json::Value LoadSchemaDoc(const ManifestVer& manifestVersion, ManifestTypeEnum manifestType)
    {
        int idx = MANIFESTSCHEMA_NO_RESOURCE;
        std::map<ManifestTypeEnum, int> resourceMap;

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_10 })
        {
            resourceMap = {
                { ManifestTypeEnum::Singleton, IDX_MANIFEST_SCHEMA_V1_10_SINGLETON },
                { ManifestTypeEnum::Version, IDX_MANIFEST_SCHEMA_V1_10_VERSION },
                { ManifestTypeEnum::Installer, IDX_MANIFEST_SCHEMA_V1_10_INSTALLER },
                { ManifestTypeEnum::DefaultLocale, IDX_MANIFEST_SCHEMA_V1_10_DEFAULTLOCALE },
                { ManifestTypeEnum::Locale, IDX_MANIFEST_SCHEMA_V1_10_LOCALE },
            };
        }
        else if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_9 })
        {
            resourceMap = {
                { ManifestTypeEnum::Singleton, IDX_MANIFEST_SCHEMA_V1_9_SINGLETON },
                { ManifestTypeEnum::Version, IDX_MANIFEST_SCHEMA_V1_9_VERSION },
                { ManifestTypeEnum::Installer, IDX_MANIFEST_SCHEMA_V1_9_INSTALLER },
                { ManifestTypeEnum::DefaultLocale, IDX_MANIFEST_SCHEMA_V1_9_DEFAULTLOCALE },
                { ManifestTypeEnum::Locale, IDX_MANIFEST_SCHEMA_V1_9_LOCALE },
            };
        }
        else if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_7 })
        {
            resourceMap = {
                { ManifestTypeEnum::Singleton, IDX_MANIFEST_SCHEMA_V1_7_SINGLETON },
                { ManifestTypeEnum::Version, IDX_MANIFEST_SCHEMA_V1_7_VERSION },
                { ManifestTypeEnum::Installer, IDX_MANIFEST_SCHEMA_V1_7_INSTALLER },
                { ManifestTypeEnum::DefaultLocale, IDX_MANIFEST_SCHEMA_V1_7_DEFAULTLOCALE },
                { ManifestTypeEnum::Locale, IDX_MANIFEST_SCHEMA_V1_7_LOCALE },
            };
        }
        else if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_6 })
        {
            resourceMap = {
                { ManifestTypeEnum::Singleton, IDX_MANIFEST_SCHEMA_V1_6_SINGLETON },
                { ManifestTypeEnum::Version, IDX_MANIFEST_SCHEMA_V1_6_VERSION },
                { ManifestTypeEnum::Installer, IDX_MANIFEST_SCHEMA_V1_6_INSTALLER },
                { ManifestTypeEnum::DefaultLocale, IDX_MANIFEST_SCHEMA_V1_6_DEFAULTLOCALE },
                { ManifestTypeEnum::Locale, IDX_MANIFEST_SCHEMA_V1_6_LOCALE },
            };
        }
        else if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_5 })
        {
            resourceMap = {
                { ManifestTypeEnum::Singleton, IDX_MANIFEST_SCHEMA_V1_5_SINGLETON },
                { ManifestTypeEnum::Version, IDX_MANIFEST_SCHEMA_V1_5_VERSION },
                { ManifestTypeEnum::Installer, IDX_MANIFEST_SCHEMA_V1_5_INSTALLER },
                { ManifestTypeEnum::DefaultLocale, IDX_MANIFEST_SCHEMA_V1_5_DEFAULTLOCALE },
                { ManifestTypeEnum::Locale, IDX_MANIFEST_SCHEMA_V1_5_LOCALE },
            };
        }
        else if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_4 })
        {
            resourceMap = {
                { ManifestTypeEnum::Singleton, IDX_MANIFEST_SCHEMA_V1_4_SINGLETON },
                { ManifestTypeEnum::Version, IDX_MANIFEST_SCHEMA_V1_4_VERSION },
                { ManifestTypeEnum::Installer, IDX_MANIFEST_SCHEMA_V1_4_INSTALLER },
                { ManifestTypeEnum::DefaultLocale, IDX_MANIFEST_SCHEMA_V1_4_DEFAULTLOCALE },
                { ManifestTypeEnum::Locale, IDX_MANIFEST_SCHEMA_V1_4_LOCALE },
            };
        }
        else if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_2 })
        {
            resourceMap = {
                { ManifestTypeEnum::Singleton, IDX_MANIFEST_SCHEMA_V1_2_SINGLETON },
                { ManifestTypeEnum::Version, IDX_MANIFEST_SCHEMA_V1_2_VERSION },
                { ManifestTypeEnum::Installer, IDX_MANIFEST_SCHEMA_V1_2_INSTALLER },
                { ManifestTypeEnum::DefaultLocale, IDX_MANIFEST_SCHEMA_V1_2_DEFAULTLOCALE },
                { ManifestTypeEnum::Locale, IDX_MANIFEST_SCHEMA_V1_2_LOCALE },
            };
        }
        else if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_1 })
        {
            resourceMap = {
                { ManifestTypeEnum::Singleton, IDX_MANIFEST_SCHEMA_V1_1_SINGLETON },
                { ManifestTypeEnum::Version, IDX_MANIFEST_SCHEMA_V1_1_VERSION },
                { ManifestTypeEnum::Installer, IDX_MANIFEST_SCHEMA_V1_1_INSTALLER },
                { ManifestTypeEnum::DefaultLocale, IDX_MANIFEST_SCHEMA_V1_1_DEFAULTLOCALE },
                { ManifestTypeEnum::Locale, IDX_MANIFEST_SCHEMA_V1_1_LOCALE },
            };
        }
        else if (manifestVersion >= ManifestVer{ s_ManifestVersionV1 })
        {
            resourceMap = {
                { ManifestTypeEnum::Singleton, IDX_MANIFEST_SCHEMA_V1_SINGLETON },
                { ManifestTypeEnum::Version, IDX_MANIFEST_SCHEMA_V1_VERSION },
                { ManifestTypeEnum::Installer, IDX_MANIFEST_SCHEMA_V1_INSTALLER },
                { ManifestTypeEnum::DefaultLocale, IDX_MANIFEST_SCHEMA_V1_DEFAULTLOCALE },
                { ManifestTypeEnum::Locale, IDX_MANIFEST_SCHEMA_V1_LOCALE },
            };
        }
        else
        {
            resourceMap = {
                { ManifestTypeEnum::Preview, IDX_MANIFEST_SCHEMA_PREVIEW },
            };
        }

        auto iter = resourceMap.find(manifestType);
        if (iter != resourceMap.end())
        {
            idx = iter->second;
        }
        else
        {
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }

        std::string_view schemaStr = Resource::GetResourceAsString(idx, MANIFESTSCHEMA_RESOURCE_TYPE);
        return JsonSchema::LoadSchemaDoc(schemaStr);
    }

    std::vector<ValidationError> ValidateAgainstSchema(const std::vector<YamlManifestInfo>& manifestList, const ManifestVer& manifestVersion)
    {
        std::vector<ValidationError> errors;
        // A list of schema validator to avoid multiple loadings of same schema
        std::map<ManifestTypeEnum, valijson::Schema> schemaList;

        for (const auto& entry : manifestList)
        {
            if (entry.ManifestType == ManifestTypeEnum::Shadow)
            {
                // There's no schema for a shadow manifest.
                continue;
            }

            if (schemaList.find(entry.ManifestType) == schemaList.end())
            {
                // Copy constructor of valijson::Schema was private
                valijson::Schema& newSchema = schemaList.emplace(
                    std::piecewise_construct, std::make_tuple(entry.ManifestType), std::make_tuple()).first->second;
                Json::Value schemaJson = LoadSchemaDoc(manifestVersion, entry.ManifestType);
                JsonSchema::PopulateSchema(schemaJson, newSchema);
            }

            const auto& schema = schemaList.find(entry.ManifestType)->second;
            Json::Value manifestJson = ManifestYamlNodeToJson(entry.Root);
            valijson::ValidationResults results;

            if (!JsonSchema::Validate(schema, manifestJson, results))
            {
                errors.emplace_back(ValidationError::MessageContextWithFile(ManifestError::SchemaError, JsonSchema::GetErrorStringFromResults(results), entry.FileName));
            }
        }

        return errors;
    }

    std::vector<ValidationError> ValidateYamlManifestsSchemaHeader(const std::vector<YamlManifestInfo>& manifestList, const ManifestVer& manifestVersion, bool treatErrorAsWarning)
    {
        std::vector<ValidationError> errors;
        ValidationError::Level errorLevel = treatErrorAsWarning ? ValidationError::Level::Warning : ValidationError::Level::Error;

        // Read the manifest schema header and ensure it exists
        for (const auto& entry : manifestList)
        {
            if (entry.ManifestType == ManifestTypeEnum::Shadow)
            {
                // There's no schema for a shadow manifest.
                continue;
            }

            auto schemaHeaderErrors = ValidateYamlManifestSchemaHeader(entry, manifestVersion, errorLevel);
            std::move(schemaHeaderErrors.begin(), schemaHeaderErrors.end(), std::inserter(errors, errors.end()));
        }

        return errors;
    }
}
