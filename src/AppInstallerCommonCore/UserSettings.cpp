// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "AppInstallerLanguageUtilities.h"
#include "AppInstallerLogging.h"
#include "winget/JsonUtil.h"
#include "winget/Settings.h"
#include "winget/UserSettings.h"
#include "winget/filesystem.h"

#include "AppInstallerArchitecture.h"
#include "winget/Locale.h"

namespace AppInstaller::Settings
{
    using namespace std::string_view_literals;
    using namespace Runtime;
    using namespace Utility;
    using namespace Logging;
    using namespace JSON;
    using namespace Filesystem;

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

        template<>
        inline std::string GetValueString(std::vector<std::string> value)
        {
            std::string convertedValue = "[";

            bool first = true;
            for (auto const& entry : value)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    convertedValue += ", ";
                }

                convertedValue += entry;
            }

            convertedValue += ']';

            return convertedValue;
        }

        std::optional<Json::Value> ParseSettingsContent(const std::string& content, std::string_view settingName, std::vector<UserSettings::Warning>& warnings)
        {
            Json::Value root;
            Json::CharReaderBuilder builder;
            const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
            std::string error;

            if (reader->parse(content.c_str(), content.c_str() + content.size(), &root, &error))
            {
                return root;
            }

            AICLI_LOG(Core, Error, << "Error parsing " << settingName << ": " << error);
            warnings.emplace_back(StringResource::String::SettingsWarningParseError, settingName, error, false);
            warnings.back().Source = LocIndString{ settingName };

            return {};
        }

        struct ParsedSettingsFile
        {
            std::optional<Json::Value> Root;
            bool Failed = false;
        };

        ParsedSettingsFile ParseFile(const StreamDefinition& setting, std::vector<UserSettings::Warning>& warnings)
        {
            try
            {
                auto stream = Stream{ setting }.Get();
                if (stream)
                {
                    std::string settingsContentStr = Utility::ReadEntireStream(*stream);
                    if (stream->fail())
                    {
                        AICLI_LOG(Core, Error, << "Failed to read " << setting.Name << "; Stream failure.");
                        warnings.emplace_back(StringResource::String::SettingsWarningReadError, setting.Name, ""sv, false);
                        warnings.back().Source = LocIndString{ setting.Name };
                        return { {}, true };
                    }
                    auto root = ParseSettingsContent(settingsContentStr, setting.Name, warnings);
                    if (!root)
                    {
                        return { {}, true };
                    }

                    if (!root->isObject())
                    {
                        AICLI_LOG(Core, Error, << "Settings file must contain a JSON object: " << setting.Name);
                        warnings.emplace_back(StringResource::String::SettingsWarningInvalidFileFormat, setting.Name, ""sv, false);
                        warnings.back().Source = LocIndString{ setting.Name };
                        return { {}, true };
                    }

                    return { std::move(root), false };
                }
            }
            catch (const std::exception& e)
            {
                AICLI_LOG(Core, Error, << "Failed to read " << setting.Name << "; Reason: " << e.what());
                warnings.emplace_back(StringResource::String::SettingsWarningReadError, setting.Name, e.what(), false);
                warnings.back().Source = LocIndString{ setting.Name };
                return { {}, true };
            }
            catch (...)
            {
                AICLI_LOG(Core, Error, << "Failed to read " << setting.Name << "; Reason unknown.");
                warnings.emplace_back(StringResource::String::SettingsWarningReadError, setting.Name, ""sv, false);
                warnings.back().Source = LocIndString{ setting.Name };
                return { {}, true };
            }

            return {};
        }

        struct SettingsSourceInfo
        {
            Json::Value Root;
            std::string_view Name;
            UserSettingsType Type = UserSettingsType::Default;
            bool Failed = false;

            void LoadSettingsSource(
                const StreamDefinition& primary,
                const StreamDefinition& backup,
                std::vector<UserSettings::Warning>& warnings)
            {
                Name = primary.Name;

                auto primarySettings = ParseFile(primary, warnings);
                if (primarySettings.Root.has_value())
                {
                    Root = std::move(primarySettings.Root.value());
                    Type = UserSettingsType::Standard;
                    AICLI_LOG(Core, Info, << "Settings loaded from " << Name);
                }

                if (Root.isNull())
                {
                    auto backupSettings = ParseFile(backup, warnings);
                    if (backupSettings.Root.has_value())
                    {
                        Root = std::move(backupSettings.Root.value());
                        Name = backup.Name;
                        Type = UserSettingsType::Backup;
                        AICLI_LOG(Core, Info, << "Settings loaded from " << Name);
                        if (backup.Type == Type::AdminFile)
                        {
                            warnings.emplace_back(StringResource::String::SettingsWarningLoadedBackupSettings, Name, ""sv, false);
                        }
                        else
                        {
                            warnings.emplace_back(StringResource::String::SettingsWarningLoadedBackupSettings);
                        }
                        warnings.back().Source = LocIndString{ Name };
                    }
                    else if (primarySettings.Failed || primarySettings.Root.has_value() || backupSettings.Failed)
                    {
                        Failed = true;
                        warnings.emplace_back(StringResource::String::SettingsWarningUsingDefault);
                        warnings.back().Source = LocIndString{ primarySettings.Failed || primarySettings.Root.has_value() ?
                            primary.Name : backup.Name };
                    }
                }
            }
        };

        struct SettingsSources
        {
            SettingsSourceInfo User;
            SettingsSourceInfo Admin;
            bool FileBacked = false;
            bool FullToken = false;
        };

        const Json::Value* ResolveSetting(const Json::Value& root, std::string_view path)
        {
            const Json::Value* current = &root;
            while (!path.empty())
            {
                if (!current->isObject())
                {
                    return nullptr;
                }

                path.remove_prefix(1);
                const auto separator = path.find('.');
                const auto member = path.substr(0, separator);

                current = current->find(member.data(), member.data() + member.size());
                if (!current)
                {
                    return nullptr;
                }

                if (separator == std::string_view::npos)
                {
                    break;
                }
                path.remove_prefix(separator);
            }

            return current;
        }

        template <Setting S>
        std::optional<typename details::SettingMapping<S>::json_t> GetValueFromPolicy()
        {
            return GroupPolicies().GetValue<details::SettingMapping<S>::Policy>();
        }

        template <Setting S>
        void Validate(
            const SettingsSources& sources,
            std::map<Setting, details::SettingVariant>& settings,
            std::vector<UserSettings::Warning>& warnings)
        {
            constexpr auto path = details::SettingMapping<S>::Path;

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
                    AICLI_LOG(Core, Verbose, << "Valid setting from Group Policy. Field: " << path << " Value: " << GetValueString(policyValue.value()));
                }
                else
                {
                    auto valueAsString = GetValueString(policyValue.value());
                    AICLI_LOG(Core, Error, << "Invalid setting from Group Policy. Field: " << path << " Value: " << valueAsString);
                    warnings.emplace_back(StringResource::String::SettingsWarningInvalidValueFromPolicy, path, valueAsString);
                }

                return;
            }

            constexpr auto treatment = details::SettingMapping<S>::Treatment;
            const bool ignoreUser = sources.FileBacked && sources.FullToken && treatment != details::SettingTreatment::Unrestricted;
            const Json::Value* result = nullptr;
            if (sources.FileBacked && (sources.FullToken || treatment != details::SettingTreatment::RestrictedSplit))
            {
                result = ResolveSetting(sources.Admin.Root, path);
            }
            const bool useAdmin = result != nullptr;
            if (!useAdmin && !ignoreUser)
            {
                result = ResolveSetting(sources.User.Root, path);
            }
            const auto sourceName = useAdmin ? sources.Admin.Name : sources.User.Name;
            bool validAdminValue = false;

            if (result != nullptr)
            {
                auto jsonValue = GetValue<typename details::SettingMapping<S>::json_t>(*result);

                if (result->isNull())
                {
                    validAdminValue = useAdmin;
                    AICLI_LOG(Core, Verbose, << "Explicit null value. Source: " << sourceName << " Field: " << path << " Using default");
                }
                else if (jsonValue.has_value())
                {
                    auto validatedValue = details::SettingMapping<S>::Validate(jsonValue.value());

                    if (validatedValue.has_value())
                    {
                        // Finally add it to the map
                        settings[S].emplace<details::SettingIndex(S)>(
                            std::forward<typename details::SettingMapping<S>::value_t>(validatedValue.value()));
                        validAdminValue = useAdmin;
                        AICLI_LOG(Core, Verbose, << "Valid setting. Source: " << sourceName << " Field: " << path << " Value: " << GetValueString(jsonValue.value()));
                    }
                    else
                    {
                        auto valueAsString = GetValueString(jsonValue.value());
                        AICLI_LOG(Core, Error, << "Invalid field value. Source: " << sourceName << " Field: " << path << " Value: " << valueAsString);
                        warnings.emplace_back(StringResource::String::SettingsWarningInvalidFieldValue, path, valueAsString);
                        warnings.back().Source = LocIndString{ sourceName };
                    }
                }
                else
                {
                    AICLI_LOG(Core, Error, << "Invalid field format. Source: " << sourceName << " Field: " << path << " Using default");
                    warnings.emplace_back(StringResource::String::SettingsWarningInvalidFieldFormat, path);
                    warnings.back().Source = LocIndString{ sourceName };
                }
            }
            else
            {
                AICLI_LOG(Core, Verbose, << "Setting " << path << " not found. Using default");
            }

            if (ignoreUser && !validAdminValue && ResolveSetting(sources.User.Root, path) != nullptr)
            {
                AICLI_LOG(Core, Warning, << "Ignoring setting from " << sources.User.Name << " with the full token. Field: " << path);
                warnings.emplace_back(StringResource::String::SettingsWarningIgnoredWithFullToken, path);
                warnings.back().Source = LocIndString{ sources.User.Name };
                warnings.back().IsMigrationWarning = true;
            }
        }

        template <size_t... S>
        void ValidateAll(
            const SettingsSources& sources,
            std::map<Setting, details::SettingVariant>& settings,
            std::vector<UserSettings::Warning>& warnings,
            std::index_sequence<S...>)
        {
            // Use folding to call each setting validate function.
            (FoldHelper{}, ..., Validate<static_cast<Setting>(S)>(sources, settings, warnings));
        }

        std::optional<std::filesystem::path> ValidatePathValue(std::string_view value)
        {
            std::filesystem::path path = ConvertToUTF16(value);
            if (!path.is_absolute())
            {
                return {};
            }

            return path;
        }
    }

    std::optional<SortField> ConvertToSortField(std::string_view value)
    {
        static constexpr std::string_view s_sortField_relevance = "relevance";
        static constexpr std::string_view s_sortField_name = "name";
        static constexpr std::string_view s_sortField_id = "id";
        static constexpr std::string_view s_sortField_version = "version";
        static constexpr std::string_view s_sortField_source = "source";
        static constexpr std::string_view s_sortField_available = "available";

        std::string lowered = Utility::ToLower(value);

        if (lowered == s_sortField_relevance) return SortField::Relevance;
        if (lowered == s_sortField_name) return SortField::Name;
        if (lowered == s_sortField_id) return SortField::Id;
        if (lowered == s_sortField_version) return SortField::Version;
        if (lowered == s_sortField_source) return SortField::Source;
        if (lowered == s_sortField_available) return SortField::Available;
        return std::nullopt;
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
            std::string lowerValue = ToLower(value);

            if (value == "accent")
            {
                return VisualStyle::Accent;
            }
            else if (value == "rainbow")
            {
                return VisualStyle::Rainbow;
            }
            else if (value == "retro")
            {
                return VisualStyle::Retro;
            }
            else if (value == "sixel")
            {
                return VisualStyle::Sixel;
            }
            else if (value == "disabled")
            {
                return VisualStyle::Disabled;
            }

            return {};
        }

        WINGET_VALIDATE_PASS_THROUGH(EnableSixelDisplay)
        WINGET_VALIDATE_PASS_THROUGH(EFExperimentalCmd)
        WINGET_VALIDATE_PASS_THROUGH(EFExperimentalArg)
        WINGET_VALIDATE_PASS_THROUGH(EFDirectMSI)
        WINGET_VALIDATE_PASS_THROUGH(EFResume)
        WINGET_VALIDATE_PASS_THROUGH(EFFonts)
        WINGET_VALIDATE_PASS_THROUGH(EFInteractivePackageSelection)
        WINGET_VALIDATE_PASS_THROUGH(EFDeltaIndex)
        WINGET_VALIDATE_PASS_THROUGH(AnonymizePathForDisplay)
        WINGET_VALIDATE_PASS_THROUGH(TelemetryDisable)
        WINGET_VALIDATE_PASS_THROUGH(InteractivityDisable)
        WINGET_VALIDATE_PASS_THROUGH(InstallSkipDependencies)
        WINGET_VALIDATE_PASS_THROUGH(DisableInstallNotes)
        WINGET_VALIDATE_PASS_THROUGH(UninstallPurgePortablePackage)
        WINGET_VALIDATE_PASS_THROUGH(NetworkWingetAlternateSourceURL)
        WINGET_VALIDATE_PASS_THROUGH(MaxResumes)
        WINGET_VALIDATE_PASS_THROUGH(LoggingFileTotalSizeLimitInMB)
        WINGET_VALIDATE_PASS_THROUGH(LoggingFileIndividualSizeLimitInMB)
        WINGET_VALIDATE_PASS_THROUGH(LoggingFileCountLimit)

#ifndef AICLI_DISABLE_TEST_HOOKS
        WINGET_VALIDATE_PASS_THROUGH(EnableSelfInitiatedMinidump)
        WINGET_VALIDATE_PASS_THROUGH(KeepAllLogFiles)
#endif

        WINGET_VALIDATE_SIGNATURE(PortablePackageUserRoot)
        {
            return ValidatePathValue(value);
        }

        WINGET_VALIDATE_SIGNATURE(PortablePackageMachineRoot)
        {
            return ValidatePathValue(value);
        }

        WINGET_VALIDATE_SIGNATURE(ArchiveExtractionMethod)
        {
            static constexpr std::string_view s_archiveExtractionMethod_shellApi = "shellApi";
            static constexpr std::string_view s_archiveExtractionMethod_tar = "tar";

            if (Utility::CaseInsensitiveEquals(value, s_archiveExtractionMethod_tar))
            {
                return Archive::ExtractionMethod::Tar;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_archiveExtractionMethod_shellApi))
            {
                return Archive::ExtractionMethod::ShellApi;
            }

            return {};
        }

        WINGET_VALIDATE_SIGNATURE(InstallArchitecturePreference)
        {
            std::vector<Utility::Architecture> archs;
            for (auto const& i : value)
            {
                Utility::Architecture arch = Utility::ConvertToArchitectureEnum(i);
                if (Utility::IsApplicableArchitecture(arch) == Utility::InapplicableArchitecture)
                {
                    return {};
                }
                archs.emplace_back(arch);
            }
            return archs;
        }

        WINGET_VALIDATE_SIGNATURE(InstallArchitectureRequirement)
        {
            return SettingMapping<Setting::InstallArchitecturePreference>::Validate(value);
        }

        WINGET_VALIDATE_SIGNATURE(InstallScopePreference)
        {
            static constexpr std::string_view s_scope_user = "user";
            static constexpr std::string_view s_scope_machine = "machine";

            if (Utility::CaseInsensitiveEquals(value, s_scope_user))
            {
                return Manifest::ScopeEnum::User;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_scope_machine))
            {
                return Manifest::ScopeEnum::Machine;
            }

            return {};
        }

        WINGET_VALIDATE_SIGNATURE(InstallScopeRequirement)
        {
            return SettingMapping<Setting::InstallScopePreference>::Validate(value);
        }

        WINGET_VALIDATE_SIGNATURE(InstallLocalePreference)
        {
            for (auto const& entry : value)
            {
                if (!Locale::IsWellFormedBcp47Tag(entry))
                {
                    return {};
                }
            }

            return value;
        }

        WINGET_VALIDATE_SIGNATURE(InstallLocaleRequirement)
        {
            return SettingMapping<Setting::InstallLocalePreference>::Validate(value);
        }

        WINGET_VALIDATE_SIGNATURE(InstallerTypePreference)
        {
            std::vector<Manifest::InstallerTypeEnum> installerTypes;
            for (auto const& i : value)
            {
                Manifest::InstallerTypeEnum installerType = Manifest::ConvertToInstallerTypeEnum(i);
                if (installerType == Manifest::InstallerTypeEnum::Unknown)
                {
                    return {};
                }
                installerTypes.emplace_back(installerType);
            }
            return installerTypes;
        }

        WINGET_VALIDATE_SIGNATURE(InstallerTypeRequirement)
        {
            return SettingMapping<Setting::InstallerTypePreference>::Validate(value);
        }

        WINGET_VALIDATE_SIGNATURE(InstallDefaultRoot)
        {
            return ValidatePathValue(value);
        }

        WINGET_VALIDATE_SIGNATURE(DownloadDefaultDirectory)
        {
            return ValidatePathValue(value);
        }

        WINGET_VALIDATE_SIGNATURE(ConfigureDefaultModuleRoot)
        {
            return ValidatePathValue(value);
        }

        WINGET_VALIDATE_SIGNATURE(NetworkDownloader)
        {
            static constexpr std::string_view s_downloader_default = "default";
            static constexpr std::string_view s_downloader_wininet = "wininet";
            static constexpr std::string_view s_downloader_do = "do";

            if (Utility::CaseInsensitiveEquals(value, s_downloader_default))
            {
                return InstallerDownloader::Default;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_downloader_wininet))
            {
                return InstallerDownloader::WinInet;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_downloader_do))
            {
                return InstallerDownloader::DeliveryOptimization;
            }

            return {};
        }

        WINGET_VALIDATE_SIGNATURE(NetworkDOProgressTimeoutInSeconds)
        {
            return std::chrono::seconds(value);
        }

        WINGET_VALIDATE_SIGNATURE(LoggingLevelPreference)
        {
            // logging preference possible values
            static constexpr std::string_view s_logging_verbose = "verbose";
            static constexpr std::string_view s_logging_info = "info";
            static constexpr std::string_view s_logging_warning = "warning";
            static constexpr std::string_view s_logging_error = "error";
            static constexpr std::string_view s_logging_critical = "critical";

            if (Utility::CaseInsensitiveEquals(value, s_logging_verbose))
            {
                return Level::Verbose;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_logging_info))
            {
                return Level::Info;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_logging_warning))
            {
                return Level::Warning;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_logging_error))
            {
                return Level::Error;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_logging_critical))
            {
                return Level::Crit;
            }
            return {};
        }

        WINGET_VALIDATE_SIGNATURE(LoggingChannelPreference)
        {
            Logging::Channel result = Logging::Channel::None;

            for (auto const& entry : value)
            {
                result |= GetChannelFromName(entry);
            }

            return result;
        }

        WINGET_VALIDATE_SIGNATURE(LoggingFileNameStrategy)
        {
            // logging name strategy possible values
            static constexpr std::string_view s_strategy_manifest = "manifest";
            static constexpr std::string_view s_strategy_timestamp = "timestamp";
            static constexpr std::string_view s_strategy_guid = "guid";
            static constexpr std::string_view s_strategy_shortguid = "shortguid";

            if (Utility::CaseInsensitiveEquals(value, s_strategy_manifest))
            {
                return LogNameStrategy::Manifest;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_strategy_timestamp))
            {
                return LogNameStrategy::Timestamp;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_strategy_guid))
            {
                return LogNameStrategy::Guid;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_strategy_shortguid))
            {
                return LogNameStrategy::ShortGuid;
            }
            return {};
        }

        WINGET_VALIDATE_SIGNATURE(LoggingFileAgeLimitInDays)
        {
            return value * 24h;
        }

        WINGET_VALIDATE_SIGNATURE(OutputSortOrder)
        {
            std::vector<SortField> fields;
            for (auto const& entry : value)
            {
                auto field = ConvertToSortField(entry);
                if (!field)
                {
                    return {};
                }
                fields.emplace_back(field.value());
            }
            return fields;
        }

        WINGET_VALIDATE_SIGNATURE(OutputSortDirection)
        {
            static constexpr std::string_view s_sortDirection_ascending = "ascending";
            static constexpr std::string_view s_sortDirection_descending = "descending";

            if (Utility::CaseInsensitiveEquals(value, s_sortDirection_ascending))
            {
                return SortDirection::Ascending;
            }
            else if (Utility::CaseInsensitiveEquals(value, s_sortDirection_descending))
            {
                return SortDirection::Descending;
            }

            return {};
        }

        WINGET_VALIDATE_SIGNATURE(OutputLocale)
        {
            auto normalizedLocale = Locale::NormalizeOutputLocale(value);
            if (normalizedLocale)
            {
                return std::string{ normalizedLocale.value() };
            }

            return {};
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    static UserSettings* s_UserSettings_Override = nullptr;

    void SetUserSettingsOverride(UserSettings* value)
    {
        s_UserSettings_Override = value;
    }
#endif

    static std::atomic_bool s_userSettingsInitialized{ false };
    static std::atomic_bool s_userSettingsInInitialization{ false };

    UserSettings const& UserSettings::Instance(const std::optional<std::string>& content)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_UserSettings_Override)
        {
            return *s_UserSettings_Override;
        }
#endif
        static UserSettings userSettings = [&content]()
        {
            s_userSettingsInInitialization = true;
            auto resetInitialization = wil::scope_exit([]() { s_userSettingsInInitialization = false; });
            return UserSettings(content);
        }();

        s_userSettingsInitialized = true;

        return userSettings;
    }

    const UserSettings* TryGetUser()
    {
        if (s_userSettingsInitialized)
        {
            return &UserSettings::Instance();
        }

        // Try to initialize UserSettings, return nullptr if it's already in initialization.
        if (s_userSettingsInInitialization)
        {
            return nullptr;
        }

        return &UserSettings::Instance();
    }

    UserSettings const& User()
    {
        return UserSettings::Instance();
    }

    bool TryInitializeCustomUserSettings(std::string content)
    {
        if (s_userSettingsInitialized || s_userSettingsInInitialization)
        {
            return false;
        }

        return UserSettings::Instance(std::move(content)).GetType() == UserSettingsType::Custom;
    }

    UserSettings::UserSettings(const std::optional<std::string>& content) : m_type(UserSettingsType::Default)
    {
        SettingsSources sources;

        // Select the ordinary source independently of the protected overlay.
        // 0 - Use default (empty) settings if disabled by group policy.
        // if
        // 1 - Use passed in settings content if available.
        // else
        // 2 - Use settings.json if exists and passes parsing.
        // 3 - Use settings.backup.json if settings.json fails to parse.
        // finally
        // 4 - Use default (empty) if both settings files fail to load.

        if (!GroupPolicies().IsEnabled(TogglePolicy::Policy::Settings))
        {
            AICLI_LOG(Core, Info, << "Ignoring settings file due to group policy. Using default values.");
            return;
        }

        if (content.has_value())
        {
            sources.User.Name = "CustomSettings"sv;
            auto settingsJson = ParseSettingsContent(content.value(), "CustomSettings", m_warnings);
            if (settingsJson.has_value())
            {
                AICLI_LOG(Core, Info, << "Settings loaded from custom settings");
                sources.User.Type = UserSettingsType::Custom;
                sources.User.Root = std::move(settingsJson.value());
            }
        }
        else
        {
            sources.FileBacked = true;
            sources.FullToken = IsRunningWithNonDefaultFullToken();
            sources.User.LoadSettingsSource(Stream::PrimaryUserSettings, Stream::BackupUserSettings, m_warnings);
            sources.Admin.LoadSettingsSource(Stream::PrimaryAdminSettingsFile, Stream::BackupAdminSettingsFile, m_warnings);
        }

        m_type = sources.User.Type;
        m_adminType = sources.Admin.Type;
        if (sources.Admin.Failed)
        {
            AICLI_LOG(Core, Warning, << "Protected settings could not be loaded. Ignoring file-backed settings and using policy or default values.");
            ValidateAll(SettingsSources{}, m_settings, m_warnings, std::make_index_sequence<static_cast<size_t>(Setting::Max)>());
            return;
        }

        if (!sources.User.Root.isNull() || !sources.Admin.Root.isNull())
        {
            ValidateAll(sources, m_settings, m_warnings, std::make_index_sequence<static_cast<size_t>(Setting::Max)>());
        }
        else
        {
            AICLI_LOG(Core, Info, << "Valid settings file not found. Using default values.");
        }
    }

    void UserSettings::CreateAdminSettingsFile()
    {
        THROW_HR_IF(E_ACCESSDENIED, !IsRunningAsAdminOrSystem());
        Stream primarySettings{ Stream::PrimaryAdminSettingsFile };
        if (!std::filesystem::exists(primarySettings.GetPath()))
        {
            THROW_HR_IF(E_UNEXPECTED, !primarySettings.Set(s_SettingEmpty));
            AICLI_LOG(Core, Info, << "Created new protected settings file");
        }
    }

    void UserSettings::PrepareToShellExecuteFile(bool admin) const
    {
        if (admin)
        {
            if (IsRunningAsAdminOrSystem())
            {
                if (m_adminType == UserSettingsType::Standard)
                {
                    auto stream = Stream{ Stream::PrimaryAdminSettingsFile }.Get();
                    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !stream);
                    auto content = Utility::ReadEntireStream(*stream);
                    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_READ_FAULT), stream->fail());
                    THROW_HR_IF(E_UNEXPECTED, !Stream{ Stream::BackupAdminSettingsFile }.Set(content));
                    AICLI_LOG(Core, Info, << "Copied protected settings to backup file");
                }
                else
                {
                    CreateAdminSettingsFile();
                }
            }
        }
        else
        {
            UserSettingsType userSettingType = GetType();

            if (userSettingType == UserSettingsType::Default)
            {
                Stream primarySettings{ Stream::PrimaryUserSettings };

                // Create settings file if it doesn't exist.
                if (!std::filesystem::exists(primarySettings.GetPath()))
                {
                    std::ignore = primarySettings.Set(s_SettingEmpty);
                    AICLI_LOG(Core, Info, << "Created new settings file");
                }
            }
            else if (userSettingType == UserSettingsType::Standard)
            {
                // Settings file was loaded correctly, create backup.
                auto from = SettingsFilePath();
                auto to = Stream{ Stream::BackupUserSettings }.GetPath();
                std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
                AICLI_LOG(Core, Info, << "Copied settings to backup file");
            }
        }
    }

    std::filesystem::path UserSettings::SettingsFilePath(bool forDisplay)
    {
        auto path = Stream{ Stream::PrimaryUserSettings }.GetPath();

        if (forDisplay && Settings::User().Get<Setting::AnonymizePathForDisplay>())
        {
            ReplaceCommonPathPrefix(path, GetKnownFolderPath(FOLDERID_LocalAppData), "%LOCALAPPDATA%");
        }

        return path;
    }

    std::filesystem::path UserSettings::AdminSettingsFilePath()
    {
        return Stream{ Stream::PrimaryAdminSettingsFile }.GetPath();
    }
}
