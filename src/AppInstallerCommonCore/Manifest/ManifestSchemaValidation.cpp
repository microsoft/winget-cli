// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Yaml.h"
#include "winget/JsonSchemaValidation.h"
#include "winget/ManifestCommon.h"
#include "winget/ManifestSchemaValidation.h"
#include "winget/ManifestYamlParser.h"

#include <ManifestSchema.h>

namespace AppInstaller::Manifest::YamlParser
{
    using namespace std::string_view_literals;

    namespace
    {
        enum class YamlScalarType
        {
            String,
            Int
        };

        // List of fields that use non string scalar types
        const std::map<std::string_view, YamlScalarType> ManifestFieldTypes=
        {
            { "InstallerSuccessCodes"sv, YamlScalarType::Int }
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
        std::string schemaStr;

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1 })
        {
            switch (manifestType)
            {
            case AppInstaller::Manifest::ManifestTypeEnum::Singleton:
                schemaStr = JsonSchema::LoadResourceAsString(MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_SINGLETON), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            case AppInstaller::Manifest::ManifestTypeEnum::Version:
                schemaStr = JsonSchema::LoadResourceAsString(MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_VERSION), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            case AppInstaller::Manifest::ManifestTypeEnum::Installer:
                schemaStr = JsonSchema::LoadResourceAsString(MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_INSTALLER), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            case AppInstaller::Manifest::ManifestTypeEnum::DefaultLocale:
                schemaStr = JsonSchema::LoadResourceAsString(MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_DEFAULTLOCALE), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            case AppInstaller::Manifest::ManifestTypeEnum::Locale:
                schemaStr = JsonSchema::LoadResourceAsString(MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_LOCALE), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            default:
                THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
            }
        }
        else
        {
            schemaStr = JsonSchema::LoadResourceAsString(MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_PREVIEW), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
        }

        return JsonSchema::LoadSchemaDoc(schemaStr);
    }

    std::vector<ValidationError> ValidateAgainstSchema(const std::vector<YamlManifestInfo>& manifestList, const ManifestVer& manifestVersion)
    {
        std::vector<ValidationError> errors;
        // A list of schema validator to avoid multiple loadings of same schema
        std::map<ManifestTypeEnum, valijson::Schema> schemaList;

        for (const auto& entry : manifestList)
        {
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
                errors.emplace_back(ValidationError::MessageWithFile(JsonSchema::GetErrorStringFromResults(results), entry.FileName));
            }
        }

        return errors;
    }
}