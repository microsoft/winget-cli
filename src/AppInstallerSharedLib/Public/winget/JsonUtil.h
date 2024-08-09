// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <json/json.h>

// Disable dllimport for cpprest JSON in any downstream consumers
#ifndef _NO_ASYNCRTIMP
#define _NO_ASYNCRTIMP
#endif

#ifndef WINGET_DISABLE_FOR_FUZZING
#include <cpprest/json.h>
#endif

#include <optional>
#include <string>
#include <vector>

namespace AppInstaller::JSON
{
    // For JSON CPP Lib
    template<class T>
    std::optional<T> GetValue(const Json::Value& node);

    template<>
    std::optional<std::string> GetValue<std::string>(const Json::Value& node);

    template<>
    std::optional<uint32_t> GetValue<uint32_t>(const Json::Value& node);

    template<>
    std::optional<bool> GetValue<bool>(const Json::Value& node);

    template<>
    std::optional<std::vector<std::string>> GetValue<std::vector<std::string>>(const Json::Value& node);

#ifndef WINGET_DISABLE_FOR_FUZZING
    // For cpprestsdk JSON
    std::optional<std::reference_wrapper<const web::json::value>> GetJsonValueFromNode(const web::json::value& node, const utility::string_t& keyName);

    std::optional<std::string> GetRawStringValueFromJsonValue(const web::json::value& value);

    std::optional<std::string> GetRawStringValueFromJsonNode(const web::json::value& node, const utility::string_t& keyName);

    std::optional<bool> GetRawBoolValueFromJsonValue(const web::json::value& value);

    std::optional<bool> GetRawBoolValueFromJsonNode(const web::json::value& node, const utility::string_t& keyName);

    std::optional<std::reference_wrapper<const web::json::array>> GetRawJsonArrayFromJsonNode(const web::json::value& node, const utility::string_t& keyName);

    std::vector<std::string> GetRawStringArrayFromJsonNode(const web::json::value& node, const utility::string_t& keyName);
    std::set<std::string> GetRawStringSetFromJsonNode(const web::json::value& node, const utility::string_t& keyName);

    std::optional<int> GetRawIntValueFromJsonValue(const web::json::value& value);

    std::optional<int> GetRawIntValueFromJsonNode(const web::json::value& value, const utility::string_t& keyName);

    utility::string_t GetUtilityString(std::string_view nodeName);

    web::json::value GetStringValue(std::string_view value);

    // Base64 encode
    std::string Base64Encode(const std::vector<BYTE>& input);

    // Base64 decode
    std::vector<BYTE>Base64Decode(const std::string& input);
#endif

    bool IsValidNonEmptyStringValue(std::optional<std::string>& value);
}
