// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Yaml.h"
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

    std::string LoadResourceAsString(PCWSTR resourceName, PCWSTR resourceType)
    {
        HMODULE resourceModule = NULL;
        GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            (PCWSTR)LoadResourceAsString,
            &resourceModule);
        THROW_LAST_ERROR_IF_NULL(resourceModule);

        HRSRC resourceInfoHandle = FindResource(resourceModule, resourceName, resourceType);
        THROW_LAST_ERROR_IF_NULL(resourceInfoHandle);

        HGLOBAL resourceMemoryHandle = LoadResource(resourceModule, resourceInfoHandle);
        THROW_LAST_ERROR_IF_NULL(resourceMemoryHandle);

        ULONG resourceSize = 0;
        char* resourceContent = NULL;
        resourceSize = SizeofResource(resourceModule, resourceInfoHandle);
        THROW_LAST_ERROR_IF(resourceSize == 0);

        resourceContent = reinterpret_cast<char*>(LockResource(resourceMemoryHandle));
        THROW_HR_IF_NULL(E_UNEXPECTED, resourceContent);

        std::string resourceStr;
        resourceStr.assign(resourceContent, resourceSize);

        return resourceStr;
    }

    Json::Value LoadSchemaDoc(const ManifestVer& manifestVersion, ManifestTypeEnum manifestType)
    {
        std::string schemaStr;

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1 })
        {
            switch (manifestType)
            {
            case AppInstaller::Manifest::ManifestTypeEnum::Singleton:
                schemaStr = LoadResourceAsString(MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_SINGLETON), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            case AppInstaller::Manifest::ManifestTypeEnum::Version:
                schemaStr = LoadResourceAsString(MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_VERSION), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            case AppInstaller::Manifest::ManifestTypeEnum::Installer:
                schemaStr = LoadResourceAsString(MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_INSTALLER), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            case AppInstaller::Manifest::ManifestTypeEnum::DefaultLocale:
                schemaStr = LoadResourceAsString(MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_DEFAULTLOCALE), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            case AppInstaller::Manifest::ManifestTypeEnum::Locale:
                schemaStr = LoadResourceAsString(MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_LOCALE), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            default:
                THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
            }
        }
        else
        {
            schemaStr = LoadResourceAsString(MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_PREVIEW), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
        }

        Json::Value schemaJson;
        int schemaLength = static_cast<int>(schemaStr.length());
        Json::CharReaderBuilder charReaderBuilder;
        const std::unique_ptr<Json::CharReader> jsonReader(charReaderBuilder.newCharReader());
        std::string errorMsg;
        if (!jsonReader->parse(schemaStr.c_str(), schemaStr.c_str() + schemaLength, &schemaJson, &errorMsg)) {
            THROW_HR_MSG(E_UNEXPECTED, "Jsoncpp parser failed to parse the schema doc. Reason: %s", errorMsg.c_str());
        }

        return schemaJson;
    }

    std::vector<ValidationError> ValidateAgainstSchema(const std::vector<YamlManifestInfo>& manifestList, const ManifestVer& manifestVersion)
    {
        std::vector<ValidationError> errors;
        // A list of schema validator to avoid multiple loadings of same schema
        std::map<ManifestTypeEnum, valijson::Schema> schemaList;
        valijson::Validator schemaValidator;

        for (const auto& entry : manifestList)
        {
            if (schemaList.find(entry.ManifestType) == schemaList.end())
            {
                // Copy constructor of valijson::Schema was private
                valijson::Schema& newSchema = schemaList.emplace(
                    std::piecewise_construct, std::make_tuple(entry.ManifestType), std::make_tuple()).first->second;
                valijson::SchemaParser schemaParser;
                Json::Value schemaJson = LoadSchemaDoc(manifestVersion, entry.ManifestType);
                valijson::adapters::JsonCppAdapter jsonSchemaAdapter(schemaJson);
                schemaParser.populateSchema(jsonSchemaAdapter, newSchema);
            }

            const auto& schema = schemaList.find(entry.ManifestType)->second;

            Json::Value manifestJson = ManifestYamlNodeToJson(entry.Root);
            valijson::adapters::JsonCppAdapter manifestJsonAdapter(manifestJson);
            valijson::ValidationResults results;

            if (!schemaValidator.validate(schema, manifestJsonAdapter, &results))
            {
                valijson::ValidationResults::Error error;
                std::stringstream ss;

                ss << "Schema validation failed." << std::endl;
                while (results.popError(error))
                {
                    std::string context;
                    for (auto itr = error.context.begin(); itr != error.context.end(); itr++)
                    {
                        context += *itr;
                    }

                    ss << "Error context: " << context << " Description: " << error.description << std::endl;
                }

                errors.emplace_back(ValidationError::MessageWithFile(ss.str(), entry.FileName));
            }
        }

        return errors;
    }
}