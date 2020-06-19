#pragma once
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <json.h>

#include <optional>
#include <string>

namespace AppInstaller::Utility
{
    template<class T>
    std::optional<T> GetValue(const Json::Value& node);

    template<>
    std::optional<std::string> GetValue<std::string>(const Json::Value& node);

    template<>
    std::optional<uint32_t> GetValue<uint32_t>(const Json::Value& node);

}
