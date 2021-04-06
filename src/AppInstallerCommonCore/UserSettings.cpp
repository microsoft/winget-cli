// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerRuntime.h>
#include "AppInstallerLanguageUtilities.h"
#include "AppInstallerLogging.h"
#include "JsonUtil.h"
#include "winget/Settings.h"
#include "winget/UserSettings.h"

namespace AppInstaller::Settings
{
    using namespace std::string_view_literals;
    using namespace Runtime;
    using namespace Utility;

    static constexpr std::string_view s_SettingEmpty =
        R"({
    "$schema": "https://aka.ms/winget-settings.schema.json",

    // For documentation on these settings, see: https://aka.ms/winget-settings
    // "source": {
    //    "autoUpdateIntervalInMinutes": 5
    // },
})"sv;

    namespace
    {
        template<class T>
        inline std::string GetValueString(T value)
        {
            std::string convertedValue;

            if constexpr (std::is_arithmetic_v<T>)
            {
                convertedValue = std::to_string(value);
            }
            else
            {
                convertedValue = value;
            }

            return convertedValue;
        }

        std::optional<Json::Value> ParseFile(const StreamDefinition& setting, std::vector<UserSettings::Warning>& warnings)
        {
            auto stream = GetSettingStream(setting);
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

                AICLI_LOG(Core, Error, << "Error parsing " << setting.Path << ": " << error);
                warnings.emplace_back(StringResource::String::SettingsWarningParseError, setting.Path, error, false);
            }

            return {};
        }

        template <Setting S>
        std::optional<typename details::SettingMapping<S>::json_t> GetValueFromPolicy()
        {
            return GroupPolicies().GetValue<details::SettingMapping<S>::Policy>();
        }

        template <Setting S>
        void Validate(
            Json::Value& root,
            std::map<Setting, details::SettingVariant>& settings,
            std::vector<UserSettings::Warning>& warnings)
        {
            // jsoncpp doesn't support std::string_view yet.
            auto path = std::string(details::SettingMapping<S>::Path);

            // Settings set by Group Policy override anything else. See if there is one.
            auto policyValue = GetValueFromPolicy<S>();
            if (policyValue.has_value())
            {
                // If the value is valid, use it.
                // Otherwise, fall back to default.
                // In any case, we do not need to read the setting from the JSON.
                auto validatedValue = details::SettingMapping<S>::Validate(policyValue.value());
                if (validatedValue.has_value())
                {
                    // Add it to the map
                    settings[S].emplace<details::SettingIndex(S)>(
                        std::forward<typename details::SettingMapping<S>::value_t>(validatedValue.value()));
                    AICLI_LOG(Core, Info, << "Valid setting from Group Policy. Field: " << path << " Value: " << GetValueString(policyValue.value()));
                }
                else
                {
                    auto valueAsString = GetValueString(policyValue.value());
                    AICLI_LOG(Core, Info, << "Invalid setting from Group Policy. Field: " << path << " Value: " << valueAsString);
                    warnings.emplace_back(StringResource::String::SettingsWarningInvalidValueFromPolicy, path, valueAsString);
                }

                return;
            }

            const Json::Path jsonPath(path);
            Json::Value result = jsonPath.resolve(root);
            if (!result.isNull())
            {
                auto jsonValue = GetValue<typename details::SettingMapping<S>::json_t>(result);

                if (jsonValue.has_value())
                {
                    auto validatedValue = details::SettingMapping<S>::Validate(jsonValue.value());

                    if (validatedValue.has_value())
                    {
                        // Finally add it to the map
                        settings[S].emplace<details::SettingIndex(S)>(
                            std::forward<typename details::SettingMapping<S>::value_t>(validatedValue.value()));
                        AICLI_LOG(Core, Info, << "Valid setting. Field: " << path << " Value: " << GetValueString(jsonValue.value()));
                    }
                    else
                    {
                        auto valueAsString = GetValueString(jsonValue.value());
                        AICLI_LOG(Core, Error, << "Invalid field value. Field: " << path << " Value: " << valueAsString);
                        warnings.emplace_back(StringResource::String::SettingsWarningInvalidFieldValue, path, valueAsString);
                    }
                }
                else
                {
                    AICLI_LOG(Core, Error, << "Invalid field format. Field: " << path << " Using default");
                    warnings.emplace_back(StringResource::String::SettingsWarningInvalidFieldFormat, path);
                }
            }
            else
            {
                AICLI_LOG(Core, Info, << "Setting " << path << " not found. Using default");
            }
        }

        template <size_t... S>
        void ValidateAll(
            Json::Value& root,
            std::map<Setting, details::SettingVariant>& settings,
            std::vector<UserSettings::Warning>& warnings,
            std::index_sequence<S...>)
        {
#ifdef WINGET_DISABLE_FOR_FUZZING
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif
            // Use folding to call each setting validate function.
            (FoldHelper{}, ..., Validate<static_cast<Setting>(S)>(root, settings, warnings));
#ifdef WINGET_DISABLE_FOR_FUZZING
#pragma clang diagnostic pop
#endif
        }
    }

    namespace details
    {
#define WINGET_VALIDATE_SIGNATURE(_setting_) \
        std::optional<SettingMapping<Setting::_setting_>::value_t> \
        SettingMapping<Setting::_setting_>::Validate(const SettingMapping<Setting::_setting_>::json_t& value)

        // Stamps out a validate function that simply returns the input value.
#define WINGET_VALIDATE_PASS_THROUGH(_setting_) \
        WINGET_VALIDATE_SIGNATURE(_setting_) \
        { \
            return value; \
        }

        WINGET_VALIDATE_SIGNATURE(AutoUpdateTimeInMinutes)
        {
            return std::chrono::minutes(value);
        }

        WINGET_VALIDATE_SIGNATURE(ProgressBarVisualStyle)
        {
            // progressBar property possible values
            static constexpr std::string_view s_progressBar_Accent = "accent";
            static constexpr std::string_view s_progressBar_Rainbow = "rainbow";
            static constexpr std::string_view s_progressBar_Retro = "retro";

            if (Utility::CaseInsensitiveEquals(value, s_progressBar_Accent))
            {
                return VisualStyle::Accent;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_progressBar_Rainbow))
            {
                return VisualStyle::Rainbow;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_progressBar_Retro))
            {
                return VisualStyle::Retro;
            }

            return {};
        }

        WINGET_VALIDATE_PASS_THROUGH(EFExperimentalCmd)
        WINGET_VALIDATE_PASS_THROUGH(EFExperimentalArg)
        WINGET_VALIDATE_PASS_THROUGH(EFExperimentalMSStore)
        WINGET_VALIDATE_PASS_THROUGH(EFList)
        WINGET_VALIDATE_PASS_THROUGH(EFExperimentalUpgrade)
        WINGET_VALIDATE_PASS_THROUGH(EFUninstall)
        WINGET_VALIDATE_PASS_THROUGH(EFImport)
        WINGET_VALIDATE_PASS_THROUGH(EFExport)
        WINGET_VALIDATE_PASS_THROUGH(TelemetryDisable)
        WINGET_VALIDATE_PASS_THROUGH(EFRestSource)

        WINGET_VALIDATE_SIGNATURE(InstallScopePreference)
        {
            static constexpr std::string_view s_scope_user = "user";
            static constexpr std::string_view s_scope_machine = "machine";

            if (Utility::CaseInsensitiveEquals(value, s_scope_user))
            {
                return ScopePreference::User;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_scope_machine))
            {
                return ScopePreference::Machine;
            }

            return {};
        }

        WINGET_VALIDATE_SIGNATURE(InstallScopeRequirement)
        {
            return SettingMapping<Setting::InstallScopePreference>::Validate(value);
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    static UserSettings* s_UserSettings_Override = nullptr;

    void SetUserSettingsOverride(UserSettings* value)
    {
        s_UserSettings_Override = value;
    }
#endif

    UserSettings const& UserSettings::Instance()
    {
        static UserSettings userSettings;

#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_UserSettings_Override)
        {
            return *s_UserSettings_Override;
        }
#endif

        return userSettings;
    }

    UserSettings::UserSettings() : m_type(UserSettingsType::Default)
    {
        Json::Value settingsRoot = Json::Value::nullSingleton();

        // Settings can be loaded from settings.json or settings.json.backup files.
        // 0 - Use default (empty) settings if disabled by group policy.
        // 1 - Use settings.json if exists and passes parsing.
        // 2 - Use settings.backup.json if settings.json fails to parse.
        // 3 - Use default (empty) if both settings files fail to load.

        if (!GroupPolicies().IsEnabled(TogglePolicy::Policy::Settings))
        {
            AICLI_LOG(Core, Info, << "Ignoring settings file due to group policy. Using default values.");
            return;
        }

        auto settingsJson = ParseFile(Streams::PrimaryUserSettings, m_warnings);
        if (settingsJson.has_value())
        {
            AICLI_LOG(Core, Info, << "Settings loaded from " << Streams::PrimaryUserSettings.Path);
            m_type = UserSettingsType::Standard;
            settingsRoot = settingsJson.value();
        }

        // Settings didn't parse or doesn't exist, try with backup.
        if (settingsRoot.isNull())
        {
            auto settingsBackupJson = ParseFile(Streams::BackupUserSettings, m_warnings);
            if (settingsBackupJson.has_value())
            {
                AICLI_LOG(Core, Info, << "Settings loaded from " << Streams::BackupUserSettings.Path);
                m_warnings.emplace_back(StringResource::String::SettingsWarningLoadedBackupSettings);
                m_type = UserSettingsType::Backup;
                settingsRoot = settingsBackupJson.value();
            }
        }

        if (!settingsRoot.isNull())
        {
            ValidateAll(settingsRoot, m_settings, m_warnings, std::make_index_sequence<static_cast<size_t>(Setting::Max)>());
        }
        else
        {
            AICLI_LOG(Core, Info, << "Valid settings file not found. Using default values.");
        }
    }

    void UserSettings::PrepareToShellExecuteFile() const
    {
        UserSettingsType userSettingType = GetType();

        if (userSettingType == UserSettingsType::Default)
        {
            // Create settings file if it doesn't exist.
            if (!std::filesystem::exists(UserSettings::SettingsFilePath()))
            {
                SetSetting(Streams::PrimaryUserSettings, s_SettingEmpty);
                AICLI_LOG(Core, Info, << "Created new settings file");
            }
        }
        else if (userSettingType == UserSettingsType::Standard)
        {
            // Settings file was loaded correctly, create backup.
            auto from = SettingsFilePath();
            auto to = GetPathTo(Streams::BackupUserSettings);
            std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
            AICLI_LOG(Core, Info, << "Copied settings to backup file");
        }
    }

    std::filesystem::path UserSettings::SettingsFilePath()
    {
        return GetPathTo(Streams::PrimaryUserSettings);
    }
}
