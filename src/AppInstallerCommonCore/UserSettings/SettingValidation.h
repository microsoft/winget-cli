#pragma once
#include <json.h>

#include <string>
#include <vector>
#include <functional>

namespace AppInstaller::Settings
{
    struct SettingFieldInfo
    {
        std::string Name;
        std::function<void(const std::string& key, const Json::Value& node)> ProcessFunc;
    };

    std::vector<std::string> ValidateAndProcessFields(
        const Json::Value& rootNode,
        const std::vector<SettingFieldInfo> fieldInfos);
}
