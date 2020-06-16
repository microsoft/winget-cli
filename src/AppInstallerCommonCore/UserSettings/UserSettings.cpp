// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerUserSettings.h"
#include <AppInstallerRuntime.h>

#include "winget/settings/Source.h"
#include "winget/settings/Visual.h"

#include <iostream>

namespace AppInstaller::Settings
{
    using namespace std::string_view_literals;
    using namespace Utility;

    static constexpr std::string_view s_SettingFileName = "settings.json"sv;
    static constexpr std::string_view s_SettingBackupFileName = "settings.json.backup"sv;

    static constexpr std::string_view s_SettingEmpty =
        R"""({
    "source": {
        "autoUpdateIntervalInMinutes": 15
    },
    "visual": {
        "progressBar": "rainbow"
    }
})"""sv;

    UserSettings::UserSettings() : m_type(UserSettingsType::Default)
    {
        Json::Value settingsRoot = Json::Value::nullSingleton();

        // Settings can be loaded from settings.json or settings.json.backup files.
        // 1 - Use settings.json if exists and passes parsing.
        // 2 - Use settings.backup.json if settings.json fails to parse.
        // 3 - Use default (empty) if both settings files fail to load.
        auto settingsPath = SettingsFilePath();
        if (std::filesystem::exists(settingsPath))
        {
            auto settingsJson = ParseFile(settingsPath);
            if (settingsJson.has_value())
            {
                m_type = UserSettingsType::Standard;
                settingsRoot = settingsJson.value();
            }
        }

        // Settings didn't parse or doesn't exist, try with backup.
        if (settingsRoot.isNull())
        {
            auto settingsBackup = SettingsBackupPath();
            if (std::filesystem::exists(settingsBackup))
            {
                auto settingsBackupJson = ParseFile(settingsBackup);

                // We don't expect users to modify manually this file so don't warn.
                if (settingsBackupJson.has_value())
                {
                    // TODO: Localize
                    m_warnings.emplace_back("Loaded settings from backup file.");
                    m_type = UserSettingsType::Backup;
                    settingsRoot = settingsBackupJson.value();
                }
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

    std::optional<Json::Value> UserSettings::ParseFile(const std::filesystem::path& path)
    {
        Json::Value root;
        Json::CharReaderBuilder builder;
        const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

        auto settingsStream = std::make_unique<std::ifstream>(path);
        std::stringstream settingsContent;
        settingsContent << settingsStream->rdbuf();
        std::string settingsContentStr = settingsContent.str();
        std::string error;

        if (reader->parse(settingsContentStr.c_str(), settingsContentStr.c_str() + settingsContentStr.size(), &root, &error))
        {
            return root;
        }

        std::string warning = path.filename().u8string() + " : " + error;
        m_warnings.push_back(warning);

        return {};
    }

    std::filesystem::path UserSettings::SettingsFilePath()
    {
        return Runtime::GetPathToLocalState() / s_SettingFileName;
    }

    std::filesystem::path UserSettings::SettingsBackupPath()
    {
        return Runtime::GetPathToLocalState() / s_SettingBackupFileName;
    }

    void UserSettings::CreateSettingsFile()
    {
        std::filesystem::path path = SettingsFilePath();

        // Create settings file if it doesn't exist.
        if (!std::filesystem::exists(path))
        {
            std::ofstream stream(path);
            stream << s_SettingEmpty << std::endl;
            stream.flush();
        }
    }

    void UserSettings::CreateBackupFile()
    {
        std::filesystem::path from = SettingsFilePath();
        std::filesystem::path to = SettingsBackupPath();

        std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
    }
}
