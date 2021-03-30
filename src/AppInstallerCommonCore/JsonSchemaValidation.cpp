// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/JsonSchemaValidation.h"

namespace AppInstaller::JsonSchema
{
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

    Json::Value LoadSchemaDoc(const std::string& schemaStr)
    {
        Json::Value schemaJson;
        int schemaLength = static_cast<int>(schemaStr.length());
        Json::CharReaderBuilder charReaderBuilder;
        const std::unique_ptr<Json::CharReader> jsonReader(charReaderBuilder.newCharReader());
        std::string errorMsg;
        if (!jsonReader->parse(schemaStr.c_str(), schemaStr.c_str() + schemaLength, &schemaJson, &errorMsg))
        {
            THROW_HR_MSG(E_UNEXPECTED, "Jsoncpp parser failed to parse the schema doc. Reason: %s", errorMsg.c_str());
        }

        return schemaJson;
    }

    Json::Value LoadResourceAsSchemaDoc(PCWSTR resourceName, PCWSTR resourceType)
    {
        return LoadSchemaDoc(LoadResourceAsString(resourceName, resourceType));
    }

    void PopulateSchema(const Json::Value& schemaJson, valijson::Schema& schema)
    {
        valijson::SchemaParser schemaParser;
        valijson::adapters::JsonCppAdapter jsonSchemaAdapter(schemaJson);
        schemaParser.populateSchema(jsonSchemaAdapter, schema);
    }

    bool Validate(const valijson::Schema& schema, const Json::Value& json, valijson::ValidationResults& results)
    {
        valijson::Validator schemaValidator;
        valijson::adapters::JsonCppAdapter jsonAdapter(json);
        return schemaValidator.validate(schema, jsonAdapter, &results);
    }

    std::string GetErrorStringFromResults(valijson::ValidationResults& results)
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

        return ss.str();
    }
}