// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Settings.h"
#include "AppInstallerUserSettings.h"
#include <AppInstallerRuntime.h>

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

    // TODO: add message and link to aka.ms. The message will need to be localized?
    static constexpr std::string_view s_SettingEmpty =
        R"""({
    // "source": {
    //    "autoUpdateIntervalInMinutes": 5
    // }
})"""sv;

    UserSettings::UserSettings() : m_type(UserSettingsType::Default)
    {
        Json::Value settingsRoot = Json::Value::nullSingleton();

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
                // TODO: Localize
                m_warnings.emplace_back("Loaded settings from backup file.");
                m_type = UserSettingsType::Backup;
                settingsRoot = settingsBackupJson.value();
            }
        }

        // Populate the settings.
        auto sourceStr = Utility::ToString(Source::GetPropertyName());
        m_source = std::make_unique<Source>(settingsRoot[sourceStr]);
        auto sourceWarnings = m_source->Warnings();
        std::move(sourceWarnings.begin(), sourceWarnings.end(), std::inserter(m_warnings, m_warnings.end()));

        auto visualStr = Utility::ToString(Visual::GetPropertyName());
        m_visual = std::make_unique<Visual>(settingsRoot[visualStr]);
        auto visualWarnings = m_visual->Warnings();
        std::move(visualWarnings.begin(), visualWarnings.end(), std::inserter(m_warnings, m_warnings.end()));
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

    void UserSettings::CreateFileIfNeeded()
    {
        if (!std::filesystem::exists(SettingsFilePath()))
        {
            SetSetting(Type::UserFile, s_SettingFileName, s_SettingEmpty);
        }
    }

    void UserSettings::CreateBackup()
    {
        if (std::filesystem::exists(SettingsFilePath()))
        {
            auto from = SettingsFilePath();
            auto to = GetPathTo(PathName::UserFileSettings) / s_SettingBackupFileName;
            std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
        }
        else
        {
            THROW_HR(ERROR_FILE_NOT_FOUND);
        }
    }
}
