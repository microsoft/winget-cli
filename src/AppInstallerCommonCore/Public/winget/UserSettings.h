// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerStrings.h"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace AppInstaller::Settings
{
    using namespace std::chrono_literals;
    using namespace std::string_view_literals;

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

    // The visual style of the progress bar.
    enum class VisualStyle
    {
        NoVT,
        Retro,
        Accent,
        Rainbow,
    };

    // Enum of settings.
    // Must start at 0 to enable direct access to variant in UserSettings.
    // Max must be last and unused.
    // How to add a setting
    // 1 - Add to enum.
    // 2 - Implement SettingMap specialization via SETTINGMAPPING_SPECIALIZATION
    // Validate will be called by ValidateAll without any more changes.
    enum class Setting : size_t
    {
        ProgressBarVisualStyle,
        AutoUpdateTimeInMinutes,
        EFExperimentalCmd,
        EFExperimentalArg,
        EFExperimentalMSStore,
        EFList,
        EFExperimentalUpgrade,
        EFUninstall,
        EFImportExport,
        Max
    };

    namespace details
    {
        template <Setting S>
        struct SettingMapping
        {
            // json_t - type the setting in json.
            // value_t - the type of this setting.
            // DefaultValue - the value_t default value when setting is absent or semantically wrong.
            // Path - json path to the property. See Json::Path in json.h for syntax. So far, this is sufficient
            //        but since is "brief" and "untested" we might implement our own if needed.
            // Validate - Function that does semantic validation.
        };

#define SETTINGMAPPING_SPECIALIZATION(_setting_, _json_, _value_, _default_, _path_) \
        template <> \
        struct SettingMapping<_setting_> \
        { \
            using json_t = _json_; \
            using value_t = _value_; \
            static constexpr value_t DefaultValue = _default_; \
            static constexpr std::string_view Path = _path_; \
            static std::optional<value_t> Validate(const json_t& value); \
        }

        SETTINGMAPPING_SPECIALIZATION(Setting::ProgressBarVisualStyle, std::string, VisualStyle, VisualStyle::Accent, ".visual.progressBar"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::AutoUpdateTimeInMinutes, uint32_t, std::chrono::minutes, 5min, ".source.autoUpdateIntervalInMinutes"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFExperimentalCmd, bool, bool, false, ".experimentalFeatures.experimentalCmd"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFExperimentalArg, bool, bool, false, ".experimentalFeatures.experimentalArg"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFExperimentalMSStore, bool, bool, false, ".experimentalFeatures.experimentalMSStore"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFList, bool, bool, false, ".experimentalFeatures.list"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFExperimentalUpgrade, bool, bool, false, ".experimentalFeatures.upgrade"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFUninstall, bool, bool, false, ".experimentalFeatures.uninstall"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFImportExport, bool, bool, false, ".experimentalFeatures.importExport"sv);

        // Used to deduce the SettingVariant type; making a variant that includes std::monostate and all SettingMapping types.
        template <size_t... I>
        inline auto Deduce(std::index_sequence<I...>) { return std::variant<std::monostate, typename SettingMapping<static_cast<Setting>(I)>::value_t...>{}; }

        // Holds data of any type listed in a SettingMapping.
        using SettingVariant = decltype(Deduce(std::make_index_sequence<static_cast<size_t>(Setting::Max)>()));

        // Gets the index into the variant for the given Setting.
        constexpr inline size_t SettingIndex(Setting s) { return static_cast<size_t>(s) + 1; }
    }


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

        void PrepareToShellExecuteFile() const;

        // Gets setting value, if its not in the map it returns the default value.
        template <Setting S>
        typename details::SettingMapping<S>::value_t Get() const
        {
            auto itr = m_settings.find(S);
            if (itr == m_settings.end())
            {
                return details::SettingMapping<S>::DefaultValue;
            }

            return std::get<details::SettingIndex(S)>(itr->second);
        }

    private:
        UserSettingsType m_type = UserSettingsType::Default;
        std::vector<std::string> m_warnings;
        std::map<Setting, details::SettingVariant> m_settings;

    protected:
        UserSettings();
        ~UserSettings() = default;

    };

    inline UserSettings const& User()
    {
        return UserSettings::Instance();
    }
}
