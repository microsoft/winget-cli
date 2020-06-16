// Copyright(c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/JsonUtil.h"
#include "AppInstallerStrings.h"

namespace AppInstaller::Utility
{
    std::optional<std::string> GetValueString(const std::string_view& key, const Json::Value& node)
    {
        std::optional<std::string> value = std::nullopt;

        // jsoncpp doesn't support std::string_view yet.
        auto keyStr = Utility::ToString(key);

        if (node.isMember(keyStr))
        {
            auto keyValue = node[keyStr];

            if (keyValue.isString())
            {
                value = keyValue.asString();
            }
        }

        return value;
    }

    std::optional<uint32_t> GetValueUInt(const std::string_view& key, const Json::Value& node)
    {
        std::optional<uint32_t> value = std::nullopt;

        // jsoncpp doesn't support std::string_view yet.
        auto keyStr = Utility::ToString(key);

        if (node.isMember(keyStr))
        {
            auto keyValue = node[keyStr];

            if (keyValue.isUInt())
            {
                value = keyValue.asUInt();
            }
        }

        return value;
    }

}

