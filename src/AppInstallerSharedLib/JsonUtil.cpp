// Copyright(c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/JsonUtil.h"
#include "AppInstallerStrings.h"

namespace AppInstaller::JSON
{
    template<>
    std::optional<std::string> GetValue(const Json::Value& node)
    {
        std::optional<std::string> value;

        if (node.isString())
        {
            value = node.asString();
        }

        return value;
    }

    template<>
    std::optional<uint32_t> GetValue(const Json::Value& node)
    {
        std::optional<uint32_t> value;

        if (node.isUInt())
        {
            value = node.asUInt();
        }

        return value;
    }

    template<>
    std::optional<bool> GetValue(const Json::Value& node)
    {
        std::optional<bool> value;

        if (node.isBool())
        {
            value = node.asBool();
        }

        return value;
    }

    template<>
    std::optional<std::vector<std::string>> GetValue(const Json::Value& node)
    {
        std::vector<std::string> result;

        if (node.isArray())
        {
            for (const Json::Value& entry : node)
            {
                if (!entry.isString())
                {
                    return std::nullopt;
                }

                result.emplace_back(entry.asString());
            }

            return result;
        }

        return std::nullopt;
    }

#ifndef WINGET_DISABLE_FOR_FUZZING
    utility::string_t GetUtilityString(std::string_view nodeName)
    {
        return utility::conversions::to_string_t(nodeName.data());
    }

    web::json::value GetStringValue(std::string_view value)
    {
        return web::json::value::string(Utility::ConvertToUTF16(value));
    }

    std::optional<std::reference_wrapper<const web::json::value>> GetJsonValueFromNode(const web::json::value& node, const utility::string_t& keyName)
    {
        if (node.is_null() || !node.has_field(keyName))
        {
            return {};
        }

        return node.at(keyName);
    }

    std::optional<std::string> GetRawStringValueFromJsonValue(const web::json::value& value)
    {
        if (value.is_null() || !value.is_string())
        {
            return {};
        }

        return utility::conversions::to_utf8string(value.as_string());
    }

    std::optional<std::string> GetRawStringValueFromJsonNode(const web::json::value& node, const utility::string_t& keyName)
    {
        std::optional<std::reference_wrapper<const web::json::value>> jsonValue = GetJsonValueFromNode(node, keyName);

        if (jsonValue)
        {
            return GetRawStringValueFromJsonValue(jsonValue.value().get());
        }

        return {};
    }

    std::optional<int> GetRawIntValueFromJsonValue(const web::json::value& value)
    {
        if (value.is_null() || !value.is_integer())
        {
            return {};
        }

        return value.as_integer();
    }

    std::optional<int> GetRawIntValueFromJsonNode(const web::json::value& node, const utility::string_t& keyName)
    {
        std::optional<std::reference_wrapper<const web::json::value>> jsonValue = GetJsonValueFromNode(node, keyName);

        if (jsonValue)
        {
            return GetRawIntValueFromJsonValue(jsonValue.value().get());
        }

        return {};
    }

    std::optional<bool> GetRawBoolValueFromJsonValue(const web::json::value& value)
    {
        if (value.is_null() || !value.is_boolean())
        {
            return {};
        }

        return value.as_bool();
    }

    std::optional<bool> GetRawBoolValueFromJsonNode(const web::json::value& node, const utility::string_t& keyName)
    {
        std::optional<std::reference_wrapper<const web::json::value>> jsonValue = GetJsonValueFromNode(node, keyName);

        if (jsonValue)
        {
            return GetRawBoolValueFromJsonValue(jsonValue.value().get());
        }

        return {};
    }

    std::optional<std::reference_wrapper<const web::json::array>> GetRawJsonArrayFromJsonNode(const web::json::value& node, const utility::string_t& keyName)
    {
        std::optional<std::reference_wrapper<const web::json::value>> jsonValue = GetJsonValueFromNode(node, keyName);

        if (!jsonValue || !jsonValue.value().get().is_array())
        {
            return {};
        }

        return jsonValue.value().get().as_array();
    }

    std::vector<std::string> GetRawStringArrayFromJsonNode(
        const web::json::value& node, const utility::string_t& keyName)
    {
        std::optional<std::reference_wrapper<const web::json::array>> arrayValue = GetRawJsonArrayFromJsonNode(node, keyName);

        std::vector<std::string> result;
        if (!arrayValue)
        {
            return result;
        }

        for (auto& value : arrayValue.value().get())
        {
            std::optional<std::string> item = GetRawStringValueFromJsonValue(value);
            if (item)
            {
                result.emplace_back(std::move(item.value()));
            }
        }

        return result;
    }

    std::set<std::string> GetRawStringSetFromJsonNode(
        const web::json::value& node, const utility::string_t& keyName)
    {
        std::optional<std::reference_wrapper<const web::json::array>> arrayValue = GetRawJsonArrayFromJsonNode(node, keyName);

        std::set<std::string> result;
        if (!arrayValue)
        {
            return result;
        }

        for (auto& value : arrayValue.value().get())
        {
            std::optional<std::string> item = GetRawStringValueFromJsonValue(value);
            if (item)
            {
                result.emplace(std::move(item.value()));
            }
        }

        return result;
    }

    std::string Base64Encode(const std::vector<BYTE>& input)
    {
        if (input.size() == 0)
        {
            return {};
        }

        std::wstring result;
        DWORD resultSize = 0;
        CryptBinaryToStringW(input.data(), static_cast<DWORD>(input.size()), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &resultSize);
        THROW_LAST_ERROR_IF(resultSize == 0);

        result.resize(resultSize);
        THROW_LAST_ERROR_IF(!CryptBinaryToStringW(input.data(), static_cast<DWORD>(input.size()), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, result.data(), &resultSize));
        // Resize to remove trailing null terminator
        result.resize(resultSize);

        return Utility::ConvertToUTF8(result);
    }

    std::vector<BYTE> Base64Decode(const std::string& input)
    {
        if (input.empty())
        {
            return {};
        }

        auto inputWide = Utility::ConvertToUTF16(input);

        std::vector<BYTE> result;
        DWORD resultSize = 0;
        CryptStringToBinaryW(inputWide.data(), static_cast<DWORD>(inputWide.size()), CRYPT_STRING_BASE64, nullptr, &resultSize, nullptr, nullptr);
        THROW_LAST_ERROR_IF(resultSize == 0);

        result.resize(resultSize);
        THROW_LAST_ERROR_IF(!CryptStringToBinaryW(inputWide.data(), static_cast<DWORD>(inputWide.size()), CRYPT_STRING_BASE64, result.data(), &resultSize, nullptr, nullptr));

        return result;
    }
#endif

    bool IsValidNonEmptyStringValue(std::optional<std::string>& value)
    {
        if (Utility::IsEmptyOrWhitespace(value.value_or("")))
        {
            return false;
        }

        return true;
    }
}
