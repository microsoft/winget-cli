// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerStrings.h"
#include "winget/settings/Source.h"
#include "winget/settings/Visual.h"
#include <json.h>

#include <filesystem>
#include <vector>
#include <string>

namespace AppInstaller::Settings
{
    // The type of argument.
    enum class UserSettingsType
    {
        // Settings files don't exist. A file is created on the first call to the settings command.
        Default,
        // Loaded settings.json
        Standard,
        // Loaded settings.json.backup
        Backup,
    };

    // Representation of the parsed settings file.
    struct UserSettings
    {
        static UserSettings& Instance()
        {
            // If we go multi-threaded secure this.
            static UserSettings userSettings;
            return userSettings;
        }

        UserSettings(const UserSettings&) = delete;
        UserSettings& operator=(const UserSettings&) = delete;

        UserSettingsType GetType() const { return m_type; }
        std::vector<std::string> GetWarnings() const { return m_warnings; }

        void Reload();
        void CreateFileIfNeeded();
        void CreateBackup();

        std::filesystem::path SettingsFilePath();
        std::filesystem::path SettingsBackupFilePath();

        // Settings
        inline const Source& GetSource() const { return *m_source; }
        inline const Visual& GetVisual() const { return *m_visual; }

    private:
        UserSettings();
        ~UserSettings() {};

        std::optional<Json::Value> ParseFile(const std::string_view& fileName);

        UserSettingsType m_type = UserSettingsType::Default;
        std::vector<std::string> m_warnings;

        std::unique_ptr<Source> m_source;
        std::unique_ptr<Visual> m_visual;

    };
}
