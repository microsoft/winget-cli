// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Yaml.h"
#include "winget/ManifestTypes.h"
#include "winget/ManifestSchemaValidation.h"
#include "winget/ManifestValidation.h"

#include <ManifestSchema.h>

namespace AppInstaller::Manifest
{
    Json::Value YamlNodeToJson(const YAML::Node& rootNode)
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
                result[keyValuePair.first.as<std::string>()] = YamlNodeToJson(keyValuePair.second);
            }
        }
        else if (rootNode.IsSequence())
        {
            for (auto const& value : rootNode.Sequence())
            {
                result.append(YamlNodeToJson(value));
            }
        }
        else if (rootNode.IsScalar())
        {
            result = YamlScalarNodeToJson(rootNode);
        }
        else
        {
            THROW_HR(E_UNEXPECTED);
        }

        return result;
    }

    Json::Value YamlScalarNodeToJson(const YAML::Node& scalarNode)
    {
        return Json::Value(scalarNode.as<std::string>());
    }

    std::vector<ValidationError> ValidateAgainstSchema(const std::vector<YamlManifestInfo> manifestList, const ManifestVer& manifestVersion, PCWSTR resourceModuleName)
    {
        std::vector<ValidationError> errors;
        std::map<ManifestTypeEnum, valijson::Schema> schemaList;
        valijson::Validator schemaValidator;

        for (const auto& entry : manifestList)
        {
            if (schemaList.find(entry.ManifestType) == schemaList.end())
            {
                valijson::Schema newSchema;
                valijson::SchemaParser schemaParser;
                Json::Value schemaJson = LoadSchemaDoc(manifestVersion, entry.ManifestType, resourceModuleName);
                valijson::adapters::JsonCppAdapter jsonSchemaAdapter(schemaJson);
                schemaParser.populateSchema(jsonSchemaAdapter, newSchema);
                schemaList.emplace(entry.ManifestType, std::move(newSchema));
            }

            const auto& schema = schemaList.find(entry.ManifestType)->second;

            Json::Value manifestJson = YamlNodeToJson(entry.Root);
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

                    ss << "Error:" << std::endl
                       << "  context: " << context << std::endl
                       << "  description: " << error.description << std::endl;
                }

                errors.emplace_back(ValidationError::MessageWithFile(ss.str(), entry.FileName));
            }
        }
    }

    std::string LoadResourceAsString(PCWSTR resourceModuleName, PCWSTR resourceName, PCWSTR resourceType)
    {
        HMODULE resourceModule = GetModuleHandle(resourceModuleName);
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

    Json::Value LoadSchemaDoc(const ManifestVer& manifestVersion, ManifestTypeEnum manifestType, PCWSTR resourceModuleName)
    {
        std::string schemaStr;

        if (manifestVersion >= ManifestVer{ s_ManifestVersionV1 })
        {
            switch (manifestType)
            {
            case AppInstaller::Manifest::ManifestTypeEnum::Singleton:
                schemaStr = LoadResourceAsString(resourceModuleName, MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_SINGLETON), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            case AppInstaller::Manifest::ManifestTypeEnum::Version:
                schemaStr = LoadResourceAsString(resourceModuleName, MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_VERSION), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            case AppInstaller::Manifest::ManifestTypeEnum::Installer:
                schemaStr = LoadResourceAsString(resourceModuleName, MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_INSTALLER), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            case AppInstaller::Manifest::ManifestTypeEnum::DefaultLocale:
                schemaStr = LoadResourceAsString(resourceModuleName, MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_DEFAULTLOCALE), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
                break;
            case AppInstaller::Manifest::ManifestTypeEnum::Locale:
                schemaStr = LoadResourceAsString(resourceModuleName, MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_V1_LOCALE), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
            default:
                THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
            }
        }
        else
        {
            schemaStr = LoadResourceAsString(resourceModuleName, MAKEINTRESOURCE(IDX_MANIFEST_SCHEMA_PREVIEW), MAKEINTRESOURCE(MANIFESTSCHEMA_RESOURCE_TYPE));
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
}