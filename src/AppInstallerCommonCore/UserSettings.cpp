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

    static constexpr std::string_view s_SettingFileName = "settings.json"sv;
    static constexpr std::string_view s_SettingBackupFileName = "settings.json.backup"sv;

    static constexpr std::string_view s_SettingEmpty =
        R"({
    // For documentation on these settings, see: https://aka.ms/winget-settings
    // "source": {
    //    "autoUpdateIntervalInMinutes": 5
    // }
})"sv;

    namespace SettingsMessage
    {
        const char* const Field = " Field: ";
        const char* const Value = " Value: ";
        const char* const ValidMessage = "Valid setting";
        const char* const InvalidFieldValue = "Invalid field value.";
        const char* const InvalidFieldFormat = "Invalid field format.";
        constexpr std::string_view LoadedBackupSettings = "Loaded settings from backup file."sv;
    }

    namespace
    {
        // Jsoncpp doesn't provide line number and column for an individual Json::Value node.
        inline std::string GetSettingsMessage(const std::string& message, const std::string& path)
        {
            return message + SettingsMessage::Field + path;
        }

        template<class T>
        inline std::string GetSettingsMessage(const std::string& message, const std::string& path, T value)
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

            return GetSettingsMessage(message, path) + SettingsMessage::Value + convertedValue;
        }


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

                AICLI_LOG(Core, Error, << "Error parsing " << fileName.u8string() << ": " << error);
                warnings.emplace_back(fileName.u8string());
                warnings.emplace_back(error);
            }

            return {};
        }

        template<typename... Args>
        ExperimentalFeature ParseAllExperimentalFeatures(
            std::function<ExperimentalFeature(ExperimentalFeature feature)> parse,
            Args... args)
        {
            return (parse(args) | ... | details::SettingMapping<Setting::ExperimentalFeatures>::DefaultValue);
        }

        template <Setting S>
        void Validate(
            Json::Value& root,
            std::map<Setting, details::SettingVariant>& settings,
            std::vector<std::string>& warnings)
        {
            // jsoncpp doesn't support std::string_view yet.
            auto path = std::string(details::SettingMapping<S>::Path);

            const Json::Path jsonPath(path);
            Json::Value result = jsonPath.resolve(root);
            if (!result.isNull())
            {
                auto jsonValue = GetValue<details::SettingMapping<S>::json_t>(result);

                if (jsonValue.has_value())
                {
                    auto validatedValue = details::SettingMapping<S>::Validate(jsonValue.value());

                    if (validatedValue.has_value())
                    {
                        // Finally add it to the map
                        settings[S].emplace<details::SettingIndex(S)>(
                            std::forward<typename details::SettingMapping<S>::value_t>(validatedValue.value()));
                        AICLI_LOG(Core, Info, << GetSettingsMessage(SettingsMessage::ValidMessage, path, jsonValue.value()));
                    }
                    else
                    {
                        auto invalidFieldMsg = GetSettingsMessage(SettingsMessage::InvalidFieldValue, path, jsonValue.value());
                        AICLI_LOG(Core, Error, << invalidFieldMsg << " Using default");
                        warnings.emplace_back(invalidFieldMsg);
                    }
                }
                else
                {
                    auto invalidFormatMsg = GetSettingsMessage(SettingsMessage::InvalidFieldFormat, path);
                    AICLI_LOG(Core, Error, << invalidFormatMsg << " Using default");
                    warnings.emplace_back(invalidFormatMsg);
                }
            }
            else
            {
                AICLI_LOG(Core, Info, << "Setting " << path <<" not found. Using default");
            }
        }

        template <>
        void Validate<Setting::ExperimentalFeatures>(
            Json::Value& root,
            std::map<Setting, details::SettingVariant>& settings,
            std::vector<std::string>& warnings)
        {
            auto parse = [&root, &warnings](ExperimentalFeature feature) -> ExperimentalFeature
            {
                auto path = std::string{ details::SettingMapping<Setting::ExperimentalFeatures>::Path };
                auto name = std::string{ ToString(feature) };
                const Json::Path jsonPath(path, name);
                Json::Value result = jsonPath.resolve(root);
                if (!result.isNull())
                {
                    auto jsonValue = GetValue<bool>(result);
                    if (jsonValue.has_value())
                    {
                        if (jsonValue.value())
                        {
                            AICLI_LOG(Core, Info, << GetSettingsMessage(SettingsMessage::ValidMessage, name, jsonValue.value()));
                            return feature;
                        }
                    }
                    else
                    {
                        auto invalidFormatMsg = GetSettingsMessage(SettingsMessage::InvalidFieldFormat, name);
                        AICLI_LOG(Core, Error, << invalidFormatMsg);
                        warnings.emplace_back(invalidFormatMsg);
                    }
                }

                return ExperimentalFeature::None;
            };

            auto result = ParseAllExperimentalFeatures(
                parse,
                ExperimentalFeature::ExperimentalTestA,
                ExperimentalFeature::ExperimentalTestB);

            settings[Setting::ExperimentalFeatures].emplace<details::SettingIndex(Setting::ExperimentalFeatures)>(
                std::forward<typename details::SettingMapping<Setting::ExperimentalFeatures>::value_t>(result));
        }

        template <size_t... S>
        void ValidateAll(
            Json::Value& root,
            std::map<Setting, details::SettingVariant>& settings,
            std::vector<std::string>& warnings,
            std::index_sequence<S...>)
        {
            // Use folding to call each setting validate function.
            (FoldHelper{}, ..., Validate<static_cast<Setting>(S)>(root, settings, warnings));
        }
    }

    namespace details
    {
        std::optional<SettingMapping<Setting::AutoUpdateTimeInMinutes>::value_t>
        SettingMapping<Setting::AutoUpdateTimeInMinutes>::Validate(const SettingMapping<Setting::AutoUpdateTimeInMinutes>::json_t& value)
        {
            return std::chrono::minutes(value);
        }

        std::optional<SettingMapping<Setting::ProgressBarVisualStyle>::value_t>
        SettingMapping<Setting::ProgressBarVisualStyle>::Validate(const SettingMapping<Setting::ProgressBarVisualStyle>::json_t& value)
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
    }

    UserSettings::UserSettings() : m_type(UserSettingsType::Default)
    {
        Json::Value settingsRoot = Json::Value::nullSingleton();

        // Settings can be loaded from settings.json or settings.json.backup files.
        // 1 - Use settings.json if exists and passes parsing.
        // 2 - Use settings.backup.json if settings.json fails to parse.
        // 3 - Use default (empty) if both settings files fail to load.

        auto settingsJson = ParseFile(s_SettingFileName, m_warnings);
        if (settingsJson.has_value())
        {
            AICLI_LOG(Core, Info, << "Settings loaded from " << s_SettingFileName);
            m_type = UserSettingsType::Standard;
            settingsRoot = settingsJson.value();
        }

        // Settings didn't parse or doesn't exist, try with backup.
        if (settingsRoot.isNull())
        {
            auto settingsBackupJson = ParseFile(s_SettingBackupFileName, m_warnings);
            if (settingsBackupJson.has_value())
            {
                AICLI_LOG(Core, Info, << "Settings loaded from " << s_SettingFileName);
                m_warnings.emplace_back(SettingsMessage::LoadedBackupSettings);
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
                SetSetting(Type::UserFile, s_SettingFileName, s_SettingEmpty);
                AICLI_LOG(Core, Info, << "Created new settings file");
            }
        }
        else if (userSettingType == UserSettingsType::Standard)
        {
            // Settings file was loaded correctly, create backup.
            auto from = SettingsFilePath();
            auto to = SettingsBackupFilePath();
            std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
            AICLI_LOG(Core, Info, << "Copied settings to backup file");
        }
    }

    std::filesystem::path UserSettings::SettingsFilePath()
    {
        return GetPathTo(PathName::UserFileSettings) / s_SettingFileName;
    }

    bool UserSettings::isEnabled(ExperimentalFeature feature) const
    {
        switch (feature)
        {
        case ExperimentalFeature::ExperimentalTestA:
            return (Get<Setting::ExperimentalFeatures>() & feature) == feature ||
                    isEnabled(ExperimentalFeature::ExperimentalTestB);
        default:
            return (Get<Setting::ExperimentalFeatures>() & feature) == feature;
        }
    }

    std::string_view ToString(ExperimentalFeature feature)
    {
        switch (feature)
        {
        case ExperimentalFeature::ExperimentalTestA:
            return "experimentalTestA"sv;
        case ExperimentalFeature::ExperimentalTestB:
            return "experimentalTestB"sv;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }
}
