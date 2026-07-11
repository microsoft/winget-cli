// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/JsonSchemaValidation.h"
#include "winget/Resources.h"

namespace AppInstaller::JsonSchema
{
    Json::Value LoadSchemaDoc(std::string_view schemaStr)
    {
        Json::Value schemaJson;
        int schemaLength = static_cast<int>(schemaStr.length());
        Json::CharReaderBuilder charReaderBuilder;
        const std::unique_ptr<Json::CharReader> jsonReader(charReaderBuilder.newCharReader());
        std::string errorMsg;
        if (!jsonReader->parse(schemaStr.data(), schemaStr.data() + schemaLength, &schemaJson, &errorMsg))
        {
            THROW_HR_MSG(E_UNEXPECTED, "Jsoncpp parser failed to parse the schema doc. Reason: %hs", errorMsg.c_str());
        }

        return schemaJson;
    }

    Json::Value LoadResourceAsSchemaDoc(PCWSTR resourceName, PCWSTR resourceType)
    {
        return LoadSchemaDoc(Resource::GetResourceAsString(resourceName, resourceType));
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