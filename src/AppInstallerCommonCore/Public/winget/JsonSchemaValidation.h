// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <json.h>

namespace AppInstaller::JsonSchema
{
    // Load an embedded resource from binary and return as std::string
    std::string LoadResourceAsString(PCWSTR resourceName, PCWSTR resourceType);

    // Load schema as parsed json doc
    Json::Value LoadSchemaDoc(const std::string& schemaStr);

    // Load an embedded resource from binary and return as Json::Value
    Json::Value LoadResourceAsSchemaDoc(PCWSTR resourceName, PCWSTR resourceType);

    // Populate a valijson Schema object from a json value
    void PopulateSchema(const Json::Value& schemaJson, valijson::Schema& schema);

    // Validate a json doc with a schema
    // Returns whether it was successful and fills the results object
    bool Validate(const valijson::Schema& schema, const Json::Value& json, valijson::ValidationResults& results);

    // Extracts the error messages from a result into a single non-localized string
    std::string GetErrorStringFromResults(valijson::ValidationResults& results);
}