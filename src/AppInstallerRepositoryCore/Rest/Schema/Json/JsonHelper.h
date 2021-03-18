// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include <cpprest/json.h>
#include "winget/Manifest.h"

namespace AppInstaller::Repository::Rest::Schema::Json
{
    // Json helper.
    struct JsonHelper
    {
        static std::optional<std::reference_wrapper<const web::json::value>> GetJsonValueFromNode(const web::json::value& node, const utility::string_t& keyName);

        static std::optional<std::string> GetRawStringValueFromJsonValue(const web::json::value& value);

        static std::optional<std::string> GetRawStringValueFromJsonNode(const web::json::value& node, const utility::string_t& keyName);

        static std::optional<std::reference_wrapper<const web::json::array>> GetRawJsonArrayFromJsonNode(const web::json::value& node, const utility::string_t& keyName);

        static std::optional<int> GetRawIntValueFromJsonValue(const web::json::value& node);

        static utility::string_t GetUtilityString(std::string_view nodeName);

        static std::vector<Manifest::string_t> GetRawStringArrayFromJsonNode(const web::json::value& node, const utility::string_t& keyName);

        static bool IsValidNonEmptyStringValue(std::optional<std::string>& value);
    };
}
