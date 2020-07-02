// Copyright(c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "JsonUtil.h"
#include "AppInstallerStrings.h"

namespace AppInstaller::Utility
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

}

