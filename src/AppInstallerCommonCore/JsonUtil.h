#pragma once
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <json.h>

#include <optional>
#include <string>

namespace AppInstaller::Utility
{
    std::optional<std::string> GetValueString(const std::string_view& key, const Json::Value& node);

    std::optional<uint32_t> GetValueUInt(const std::string_view& key, const Json::Value& node);

}
