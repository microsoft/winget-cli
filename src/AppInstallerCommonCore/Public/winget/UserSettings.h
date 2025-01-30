// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerStrings.h"
#include "AppInstallerLogging.h"
#include "winget/Archive.h"
#include "winget/GroupPolicy.h"
#include "winget/Resources.h"
#include "winget/ManifestCommon.h"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "AppInstallerArchitecture.h"

using namespace std::chrono_literals;
using namespace std::string_view_literals;

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
        // Loaded from custom settings content
        Custom,
    };

    // The visual style of the progress bar.
    enum class VisualStyle
    {
        NoVT,
        Retro,
        Accent,
        Rainbow,
        Sixel,
        Disabled,
    };

    // The download code to use for *installers*.
    enum class InstallerDownloader
    {
        Default,
        WinInet,
        DeliveryOptimization,
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
        // Visual
        ProgressBarVisualStyle,
        AnonymizePathForDisplay,
        EnableSixelDisplay,
        // Source
        AutoUpdateTimeInMinutes,
        // Experimental
        EFExperimentalCmd,
        EFExperimentalArg,
        EFDirectMSI,
        EFResume,
        EFConfiguration03,
        EFConfigureExport,
        EFFonts,
        // Telemetry
        TelemetryDisable,
        // Install behavior
        InstallScopePreference,
        InstallScopeRequirement,
        InstallArchitecturePreference,
        InstallArchitectureRequirement,
        InstallLocalePreference,
        InstallLocaleRequirement,
        InstallerTypePreference,
        InstallerTypeRequirement,
        InstallDefaultRoot,
        InstallSkipDependencies,
        ArchiveExtractionMethod,
        DisableInstallNotes,
        PortablePackageUserRoot,
        PortablePackageMachineRoot,
        MaxResumes,
        // Network
        NetworkDownloader,
        NetworkDOProgressTimeoutInSeconds,
        NetworkWingetAlternateSourceURL,
        // Logging
        LoggingLevelPreference,
        LoggingChannelPreference,
        // Uninstall behavior
        UninstallPurgePortablePackage,
        // Download behavior
        DownloadDefaultDirectory,
        // Configure behavior
        ConfigureDefaultModuleRoot,
        // Interactivity
        InteractivityDisable,
#ifndef AICLI_DISABLE_TEST_HOOKS
        // Debug
        EnableSelfInitiatedMinidump,
        KeepAllLogFiles,
#endif
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

#define SETTINGMAPPING_SPECIALIZATION_POLICY(_setting_, _json_, _value_, _default_, _path_, _valuePolicy_) \
        template <> \
        struct SettingMapping<_setting_> \
        { \
            using json_t = _json_; \
            using value_t = _value_; \
            inline static const value_t DefaultValue = _default_; \
            static constexpr std::string_view Path = _path_; \
            static std::optional<value_t> Validate(const json_t& value); \
            static constexpr ValuePolicy Policy = _valuePolicy_; \
            using policy_t = GroupPolicy::ValueType<Policy>; \
            static_assert(Policy == ValuePolicy::None || std::is_same<json_t, policy_t>::value); \
        }

#define SETTINGMAPPING_SPECIALIZATION(_setting_, _json_, _value_, _default_, _path_) \
        SETTINGMAPPING_SPECIALIZATION_POLICY(_setting_, _json_, _value_, _default_, _path_, ValuePolicy::None)

        // Visual
        SETTINGMAPPING_SPECIALIZATION(Setting::ProgressBarVisualStyle, std::string, VisualStyle, VisualStyle::Accent, ".visual.progressBar"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::AnonymizePathForDisplay, bool, bool, true, ".visual.anonymizeDisplayedPaths"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EnableSixelDisplay, bool, bool, false, ".visual.enableSixels"sv);
        // Source
        SETTINGMAPPING_SPECIALIZATION_POLICY(Setting::AutoUpdateTimeInMinutes, uint32_t, std::chrono::minutes, 15min, ".source.autoUpdateIntervalInMinutes"sv, ValuePolicy::SourceAutoUpdateIntervalInMinutes);
        // Experimental
        SETTINGMAPPING_SPECIALIZATION(Setting::EFExperimentalCmd, bool, bool, false, ".experimentalFeatures.experimentalCmd"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFExperimentalArg, bool, bool, false, ".experimentalFeatures.experimentalArg"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFDirectMSI, bool, bool, false, ".experimentalFeatures.directMSI"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFResume, bool, bool, false, ".experimentalFeatures.resume"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFConfiguration03, bool, bool, false, ".experimentalFeatures.configuration03"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFConfigureExport, bool, bool, false, ".experimentalFeatures.configureExport"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::EFFonts, bool, bool, false, ".experimentalFeatures.fonts"sv);
        // Telemetry
        SETTINGMAPPING_SPECIALIZATION(Setting::TelemetryDisable, bool, bool, false, ".telemetry.disable"sv);
        // Install behavior
        SETTINGMAPPING_SPECIALIZATION(Setting::InstallArchitecturePreference, std::vector<std::string>, std::vector<Utility::Architecture>, {}, ".installBehavior.preferences.architectures"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::InstallArchitectureRequirement, std::vector<std::string>, std::vector<Utility::Architecture>, {}, ".installBehavior.requirements.architectures"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::InstallScopePreference, std::string, Manifest::ScopeEnum, Manifest::ScopeEnum::User, ".installBehavior.preferences.scope"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::InstallScopeRequirement, std::string, Manifest::ScopeEnum, Manifest::ScopeEnum::Unknown, ".installBehavior.requirements.scope"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::InstallLocalePreference, std::vector<std::string>, std::vector<std::string>, {}, ".installBehavior.preferences.locale"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::InstallLocaleRequirement, std::vector<std::string>, std::vector<std::string>, {}, ".installBehavior.requirements.locale"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::InstallerTypePreference, std::vector<std::string>, std::vector<Manifest::InstallerTypeEnum>, {}, ".installBehavior.preferences.installerTypes"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::InstallerTypeRequirement, std::vector<std::string>, std::vector<Manifest::InstallerTypeEnum>, {}, ".installBehavior.requirements.installerTypes"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::InstallSkipDependencies, bool, bool, false, ".installBehavior.skipDependencies"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::ArchiveExtractionMethod, std::string, Archive::ExtractionMethod, Archive::ExtractionMethod::ShellApi, ".installBehavior.archiveExtractionMethod"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::DisableInstallNotes, bool, bool, false, ".installBehavior.disableInstallNotes"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::PortablePackageUserRoot, std::string, std::filesystem::path, {}, ".installBehavior.portablePackageUserRoot"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::PortablePackageMachineRoot, std::string, std::filesystem::path, {}, ".installBehavior.portablePackageMachineRoot"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::InstallDefaultRoot, std::string, std::filesystem::path, {}, ".installBehavior.defaultInstallRoot"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::MaxResumes, uint32_t, int, 3, ".installBehavior.maxResumes"sv);
        // Uninstall behavior
        SETTINGMAPPING_SPECIALIZATION(Setting::UninstallPurgePortablePackage, bool, bool, false, ".uninstallBehavior.purgePortablePackage"sv);
        // Download behavior
        SETTINGMAPPING_SPECIALIZATION(Setting::DownloadDefaultDirectory, std::string, std::filesystem::path, {}, ".downloadBehavior.defaultDownloadDirectory"sv);
        // Configure behavior
        SETTINGMAPPING_SPECIALIZATION(Setting::ConfigureDefaultModuleRoot, std::string, std::filesystem::path, {}, ".configureBehavior.defaultModuleRoot"sv);

        // Network
        SETTINGMAPPING_SPECIALIZATION(Setting::NetworkDownloader, std::string, InstallerDownloader, InstallerDownloader::Default, ".network.downloader"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::NetworkDOProgressTimeoutInSeconds, uint32_t, std::chrono::seconds, 60s, ".network.doProgressTimeoutInSeconds"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::NetworkWingetAlternateSourceURL, bool, bool, true, ".network.enableWingetAlternateSourceURL"sv);
#ifndef AICLI_DISABLE_TEST_HOOKS
        // Debug
        SETTINGMAPPING_SPECIALIZATION(Setting::EnableSelfInitiatedMinidump, bool, bool, false, ".debugging.enableSelfInitiatedMinidump"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::KeepAllLogFiles, bool, bool, false, ".debugging.keepAllLogFiles"sv);
#endif
        // Logging
        SETTINGMAPPING_SPECIALIZATION(Setting::LoggingLevelPreference, std::string, Logging::Level, Logging::Level::Info, ".logging.level"sv);
        SETTINGMAPPING_SPECIALIZATION(Setting::LoggingChannelPreference, std::vector<std::string>, Logging::Channel, Logging::Channel::Defaults, ".logging.channels"sv);
        // Interactivity
        SETTINGMAPPING_SPECIALIZATION(Setting::InteractivityDisable, bool, bool, false, ".interactivity.disable"sv);
        
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
        // Jsoncpp doesn't provide line number and column for an individual Json::Value node.
        struct Warning
        {
            Warning(StringResource::StringId message) : Message(message) {}
            Warning(StringResource::StringId message, std::string_view settingPath) : Message(message), Path(settingPath) {}
            Warning(StringResource::StringId message, std::string_view settingPath, std::string_view settingValue, bool isField = true) :
                Message(message), Path(settingPath), Data(settingValue), IsFieldWarning(isField) {}

            StringResource::StringId Message;
            Utility::LocIndString Path;
            Utility::LocIndString Data;
            bool IsFieldWarning = true;
        };

        static UserSettings const& Instance(const std::optional<std::string>& content = std::nullopt);

        static std::filesystem::path SettingsFilePath(bool forDisplay = false);

        UserSettings(const UserSettings&) = delete;
        UserSettings& operator=(const UserSettings&) = delete;

        UserSettings(UserSettings&&) = delete;
        UserSettings& operator=(UserSettings&&) = delete;

        UserSettingsType GetType() const { return m_type; }
        std::vector<Warning> const& GetWarnings() const { return m_warnings; }

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

    protected:
        UserSettingsType m_type = UserSettingsType::Default;
        std::vector<Warning> m_warnings;
        std::map<Setting, details::SettingVariant> m_settings;

        UserSettings(const std::optional<std::string>& content = std::nullopt);
        ~UserSettings() = default;
    };

    const UserSettings* TryGetUser();
    UserSettings const& User();
    bool TryInitializeCustomUserSettings(std::string content);
}
