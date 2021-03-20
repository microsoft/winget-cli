// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "JsonHelper.h"

namespace AppInstaller::Repository::Rest::Schema::Json
{
    utility::string_t JsonHelper::GetUtilityString(std::string_view nodeName)
    {
        return utility::conversions::to_string_t(nodeName.data());
    }

    std::optional<std::reference_wrapper<const web::json::value>> JsonHelper::GetJsonValueFromNode(const web::json::value& node, const utility::string_t& keyName)
    {
        if (node.is_null() || !node.has_field(keyName))
        {
            return {};
        }
        
        return node.at(keyName);
    }

    std::optional<std::string> JsonHelper::GetRawStringValueFromJsonValue(const web::json::value& value)
    {
        if (value.is_null() || !value.is_string())
        {
            return {};
        }

        return utility::conversions::to_utf8string(value.as_string());
    }

    std::optional<std::string> JsonHelper::GetRawStringValueFromJsonNode(const web::json::value& node, const utility::string_t& keyName)
    {
        std::optional<std::reference_wrapper<const web::json::value>> jsonValue = GetJsonValueFromNode(node, keyName);

        if (jsonValue)
        {
            return GetRawStringValueFromJsonValue(jsonValue.value().get());
        }

        return {};
    }

    std::optional<int> JsonHelper::GetRawIntValueFromJsonValue(const web::json::value& value)
    {
        if (value.is_null() || !value.is_integer())
        {
            return {};
        }

        return value.as_integer();
    }

    std::optional<std::reference_wrapper<const web::json::array>> JsonHelper::GetRawJsonArrayFromJsonNode(const web::json::value& node, const utility::string_t& keyName)
    {
        std::optional<std::reference_wrapper<const web::json::value>> jsonValue = GetJsonValueFromNode(node, keyName);

        if (!jsonValue || !jsonValue.value().get().is_array())
        {
            return {};
        }

        return jsonValue.value().get().as_array();
    }

    std::vector<Manifest::string_t> JsonHelper::GetRawStringArrayFromJsonNode(
        const web::json::value& node, const utility::string_t& keyName)
    {
        std::optional<std::reference_wrapper<const web::json::array>> arrayValue = GetRawJsonArrayFromJsonNode(node, keyName);

        std::vector<Manifest::string_t> result;
        if (!arrayValue)
        {
            return result;
        }

        for (auto& value : arrayValue.value().get())
        {
            std::optional<std::string> item = JsonHelper::GetRawStringValueFromJsonValue(value);
            if (item)
            {
                result.emplace_back(std::move(item.value()));
            }
        }

        return result;
    }

    bool JsonHelper::IsValidNonEmptyStringValue(std::optional<std::string>& value)
    {
        if (Utility::IsEmptyOrWhitespace(value.value_or("")))
        {
            return false;
        }

        return true;
    }
}
