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
        enum class YamlScalarType
        {
            String,
            Int,
            Bool
        };

        // List of fields that use non string scalar types
        const std::map<std::string_view, YamlScalarType> ManifestFieldTypes=
        {
            { "InstallerSuccessCodes"sv, YamlScalarType::Int },
            { "InstallerAbortsTerminal"sv, YamlScalarType::Bool },
            { "InstallLocationRequired"sv, YamlScalarType::Bool },
            { "RequireExplicitUpgrade"sv, YamlScalarType::Bool },
            { "DisplayInstallWarnings"sv, YamlScalarType::Bool },
            { "InstallerReturnCode"sv, YamlScalarType::Int },
            { "DownloadCommandProhibited", YamlScalarType::Bool }
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
    }

    Json::Value LoadSchemaDoc(const ManifestVer& manifestVersion, ManifestTypeEnum manifestType)
    {
        int idx = MANIFESTSCHEMA_NO_RESOURCE;
        std::map<ManifestTypeEnum, int> resourceMap;

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1_7 })
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
}