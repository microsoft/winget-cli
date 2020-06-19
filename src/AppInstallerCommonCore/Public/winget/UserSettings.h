// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerStrings.h"
#include "winget/settings/Source.h"
#include "winget/settings/Visual.h"

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
        static UserSettings const& Instance()
        {
            static UserSettings userSettings;
            return userSettings;
        }

        static std::filesystem::path SettingsFilePath();

        UserSettings(const UserSettings&) = delete;
        UserSettings& operator=(const UserSettings&) = delete;

        UserSettings(UserSettings&&) = delete;
        UserSettings& operator=(UserSettings&&) = delete;

        UserSettingsType GetType() const { return m_type; }
        std::vector<std::string> const& GetWarnings() const { return m_warnings; }

        void Reload();
        void PrepareToShellExecuteFile() const;

        // Settings
        inline const Source& GetSource() const { return *m_source; }
        inline const Visual& GetVisual() const { return *m_visual; }

    private:
        UserSettingsType m_type = UserSettingsType::Default;
        std::vector<std::string> m_warnings;

        std::unique_ptr<Source> m_source;
        std::unique_ptr<Visual> m_visual;

    protected:
        UserSettings();
        ~UserSettings() = default;

    };

    inline UserSettings const& User()
    {
        return UserSettings::Instance();
    }
}
