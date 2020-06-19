// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerRuntime.h>
#include "SettingValidation.h"
#include "winget/Settings.h"
#include "winget/UserSettings.h"

#include "winget/settings/Source.h"
#include "winget/settings/Visual.h"

namespace AppInstaller::Settings
{
    using namespace std::string_view_literals;
    using namespace Runtime;
    using namespace Utility;

    static constexpr std::string_view s_SettingFileName = "settings.json"sv;
    static constexpr std::string_view s_SettingBackupFileName = "settings.json.backup"sv;

    static constexpr std::string_view s_SettingEmpty =
        R"("{
    // For documentation on these settings, see: https://aka.ms/winget-settings
    // "source": {
    //    "autoUpdateIntervalInMinutes": 5
    // }
}")"sv;

    namespace
    {
        std::filesystem::path SettingsBackupFilePath()
        {
            return GetPathTo(PathName::UserFileSettings) / s_SettingBackupFileName;
        }

        std::optional<Json::Value> ParseFile(const std::filesystem::path& fileName, std::vector<std::string>& warnings)
        {
            auto stream = GetSettingStream(Type::UserFile, fileName);
            if (stream)
            {
                Json::Value root;
                Json::CharReaderBuilder builder;
                const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

                std::string settingsContentStr = Utility::ReadEntireStream(*stream);
                std::string error;

                if (reader->parse(settingsContentStr.c_str(), settingsContentStr.c_str() + settingsContentStr.size(), &root, &error))
                {
                    return root;
                }

                warnings.emplace_back(fileName.u8string());
                warnings.emplace_back(error);
            }

            return {};
        }
    }

    UserSettings::UserSettings() : m_type(UserSettingsType::Default)
    {
        Json::Value settingsRoot = Json::Value::nullSingleton();
        m_type = UserSettingsType::Default;

        // Settings can be loaded from settings.json or settings.json.backup files.
        // 1 - Use settings.json if exists and passes parsing.
        // 2 - Use settings.backup.json if settings.json fails to parse.
        // 3 - Use default (empty) if both settings files fail to load.

        auto settingsJson = ParseFile(s_SettingFileName, m_warnings);
        if (settingsJson.has_value())
        {
            m_type = UserSettingsType::Standard;
            settingsRoot = settingsJson.value();
        }

        // Settings didn't parse or doesn't exist, try with backup.
        if (settingsRoot.isNull())
        {
            auto settingsBackupJson = ParseFile(s_SettingBackupFileName, m_warnings);
            if (settingsBackupJson.has_value())
            {
                m_warnings.emplace_back(SettingsWarnings::LoadedBackupSettings);
                m_type = UserSettingsType::Backup;
                settingsRoot = settingsBackupJson.value();
            }
        }

        // Populate the settings.
        auto sourceStr = std::string(Source::GetPropertyName());
        auto source = std::make_unique<Source>(settingsRoot[sourceStr]);
        auto sourceWarnings = source->Warnings();
        std::move(sourceWarnings.begin(), sourceWarnings.end(), std::inserter(m_warnings, m_warnings.end()));

        auto visualStr = std::string(Visual::GetPropertyName());
        auto visual = std::make_unique<Visual>(settingsRoot[visualStr]);
        auto visualWarnings = visual->Warnings();
        std::move(visualWarnings.begin(), visualWarnings.end(), std::inserter(m_warnings, m_warnings.end()));

        m_source = std::move(source);
        m_visual = std::move(visual);
    }

    void UserSettings::PrepareToShellExecuteFile() const
    {
        UserSettingsType userSettingType = GetType();

        if (userSettingType == UserSettingsType::Default)
        {
            // Create settings file if it doesn't exist.
            if (!std::filesystem::exists(UserSettings::SettingsFilePath()))
            {
                SetSetting(Type::UserFile, s_SettingFileName, s_SettingEmpty);
            }
        }
        else if (userSettingType == UserSettingsType::Standard)
        {
            // Settings file was loaded correctly, create backup.
            auto from = SettingsFilePath();
            auto to = SettingsBackupFilePath();
            std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
        }
    }

    std::filesystem::path UserSettings::SettingsFilePath()
    {
        return GetPathTo(PathName::UserFileSettings) / s_SettingFileName;
    }
}
