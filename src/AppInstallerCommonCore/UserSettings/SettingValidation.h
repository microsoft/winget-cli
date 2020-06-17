#pragma once
#include <json.h>

#include <string>
#include <vector>
#include <functional>

namespace AppInstaller::Settings
{
    namespace SettingsWarnings
    {
        const char* const Field = " Field: ";
        const char* const Value = " Value: ";
        const char* const InvalidFieldValue = "Invalid field value.";
        const char* const InvalidFieldFormat = "Invalid field format.";
        const char* const LoadedBackupSettings = "Loaded settings from backup file.";
    }

    // Jsoncpp doesn't provide line number and column for an individual Json::Value node.
    inline std::string GetSettingsMessage(const std::string& message, const std::string& field)
    {
        return message + SettingsWarnings::Field + field;
    }

    inline std::string GetSettingsMessage(const std::string& message, const std::string& field, const std::string& value)
    {
        return GetSettingsMessage(message, field) + SettingsWarnings::Value + value;
    }

    struct SettingFieldInfo
    {
        std::string Name;
        std::function<void(const std::string& key, const Json::Value& node)> ProcessFunc;
    };

    std::vector<std::string> ValidateAndProcessFields(
        const Json::Value& rootNode,
        const std::vector<SettingFieldInfo> fieldInfos);
}
