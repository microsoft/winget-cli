// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerRuntime.h>
#include "SettingValidation.h"
#include "winget/Settings.h"
#include "winget/UserSettings.h"

#include "winget/settings/Source.h"
#include "winget/settings/Visual.h"

#include <iostream>

namespace AppInstaller::Settings
{
    using namespace std::string_view_literals;
    using namespace Runtime;
    using namespace Utility;

    static constexpr std::string_view s_SettingFileName = "settings.json"sv;
    static constexpr std::string_view s_SettingBackupFileName = "settings.json.backup"sv;

    static constexpr std::string_view s_SettingEmpty =
        R"""({
    // For documentation on these settings, see: https://aka.ms/winget-settings
    // "source": {
    //    "autoUpdateIntervalInMinutes": 5
    // }
})"""sv;

    UserSettings::UserSettings() : m_type(UserSettingsType::Default)
    {
        Reload();
    }

    void UserSettings::Reload()
    {
        Json::Value settingsRoot = Json::Value::nullSingleton();
        m_type = UserSettingsType::Default;

        // Settings can be loaded from settings.json or settings.json.backup files.
        // 1 - Use settings.json if exists and passes parsing.
        // 2 - Use settings.backup.json if settings.json fails to parse.
        // 3 - Use default (empty) if both settings files fail to load.

        auto settingsJson = ParseFile(s_SettingFileName);
        if (settingsJson.has_value())
        {
            m_type = UserSettingsType::Standard;
            settingsRoot = settingsJson.value();
        }

        // Settings didn't parse or doesn't exist, try with backup.
        if (settingsRoot.isNull())
        {
            auto settingsBackupJson = ParseFile(s_SettingBackupFileName);
            if (settingsBackupJson.has_value())
            {
                m_warnings.emplace_back(SettingsWarnings::LoadedBackupSettings);
                m_type = UserSettingsType::Backup;
                settingsRoot = settingsBackupJson.value();
            }
        }

        // Populate the settings.
        auto sourceStr = Utility::ToString(Source::GetPropertyName());
        auto source = std::make_unique<Source>(settingsRoot[sourceStr]);
        auto sourceWarnings = source->Warnings();
        std::move(sourceWarnings.begin(), sourceWarnings.end(), std::inserter(m_warnings, m_warnings.end()));

        auto visualStr = Utility::ToString(Visual::GetPropertyName());
        auto visual = std::make_unique<Visual>(settingsRoot[visualStr]);
        auto visualWarnings = visual->Warnings();
        std::move(visualWarnings.begin(), visualWarnings.end(), std::inserter(m_warnings, m_warnings.end()));

        m_source = std::move(source);
        m_visual = std::move(visual);
    }

    std::optional<Json::Value> UserSettings::ParseFile(const std::string_view& fileName)
    {
        auto stream = GetSettingStream(Type::UserFile, fileName);
        if (stream)
        {
            Json::Value root;
            Json::CharReaderBuilder builder;
            const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

            std::stringstream settingsContent;
            settingsContent << stream->rdbuf();
            std::string settingsContentStr = settingsContent.str();
            std::string error;

            if (reader->parse(settingsContentStr.c_str(), settingsContentStr.c_str() + settingsContentStr.size(), &root, &error))
            {
                return root;
            }

            m_warnings.emplace_back(fileName);
            m_warnings.emplace_back(error);
        }

        return {};
    }

    std::filesystem::path UserSettings::SettingsFilePath()
    {
        return GetPathTo(PathName::UserFileSettings) / s_SettingFileName;
    }

    std::filesystem::path UserSettings::SettingsBackupFilePath()
    {
        return GetPathTo(PathName::UserFileSettings) / s_SettingBackupFileName;
    }

    void UserSettings::CreateFileIfNeeded()
    {
        if (!std::filesystem::exists(SettingsFilePath()))
        {
            SetSetting(Type::UserFile, s_SettingFileName, s_SettingEmpty);
        }
    }

    void UserSettings::CreateBackup()
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !std::filesystem::exists(SettingsFilePath()));

        auto from = SettingsFilePath();
        auto to = GetPathTo(PathName::UserFileSettings) / s_SettingBackupFileName;
        std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
    }
}
