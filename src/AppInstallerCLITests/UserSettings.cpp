// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include "TestHooks.h"
#include <AppInstallerRuntime.h>
#include <winget/Settings.h>
#include "AppInstallerLogging.h"

#include <AppInstallerErrors.h>

#include <filesystem>
#include <string>
#include <chrono>

using namespace AppInstaller::Settings;
using namespace AppInstaller::Logging;
using namespace AppInstaller::Runtime;
using namespace TestCommon;
using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;

namespace
{
    static constexpr std::string_view s_goodJson = "{}";
    static constexpr std::string_view s_badJson = "{";
    static constexpr std::string_view s_settings = "settings.json"sv;
    static constexpr std::string_view s_settingsBackup = "settings.json.backup"sv;

    void SetAdminSettingsFile(const StreamDefinition& stream, std::string_view content)
    {
        REQUIRE(stream.Type == Type::AdminFile);
        const auto path = GetPathTo(stream);
        std::filesystem::create_directories(path.parent_path());

        // Resolver tests provision files directly under the test path overrides, without requiring elevation.
        std::ofstream file{ path, std::ios::binary | std::ios::trunc };
        REQUIRE(file.good());
        file << content;
        file.close();
        REQUIRE(!file.fail());
    }

    size_t MigrationWarningCount(const UserSettings& settings)
    {
        const auto& warnings = settings.GetWarnings();
        return static_cast<size_t>(std::count_if(warnings.begin(), warnings.end(),
            [](const UserSettings::Warning& warning) { return warning.IsMigrationWarning; }));
    }

    wil::unique_hfile LockSettingsFile(const StreamDefinition& stream)
    {
        const auto path = GetPathTo(stream);
        wil::unique_hfile file{ CreateFileW(path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) };
        REQUIRE(file);
        return file;
    }
}

TEST_CASE("UserSettingsOverlay_TreatmentMatrix", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    const bool fullToken = GENERATE(false, true);
    const bool userPresent = GENERATE(false, true);
    const bool adminPresent = GENERATE(false, true);
    const bool userBackup = GENERATE(false, true);
    CAPTURE(fullToken, userPresent, adminPresent, userBackup);
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(fullToken);

    if (userPresent)
    {
        constexpr std::string_view user = R"({
            "visual": { "progressBar": "rainbow" },
            "installBehavior": {
                "portablePackageUserRoot": "C:\\User",
                "portablePackageMachineRoot": "C:\\User",
                "defaultInstallRoot": "C:\\User"
            },
            "configureBehavior": { "defaultModuleRoot": "C:\\User" },
            "downloadBehavior": { "defaultDownloadDirectory": "C:\\User" },
            "uninstallBehavior": { "purgePortablePackage": true }
        })";
        if (userBackup)
        {
            SetSetting(Stream::PrimaryUserSettings, s_badJson);
        }
        SetSetting(userBackup ? Stream::BackupUserSettings : Stream::PrimaryUserSettings, user);
    }

    if (adminPresent)
    {
        SetAdminSettingsFile(Stream::PrimaryAdminSettingsFile, R"({
            "visual": { "progressBar": "retro" },
            "installBehavior": {
                "portablePackageUserRoot": "C:\\Admin",
                "portablePackageMachineRoot": "C:\\Admin",
                "defaultInstallRoot": "C:\\Admin"
            },
            "configureBehavior": { "defaultModuleRoot": "C:\\Admin" },
            "downloadBehavior": { "defaultDownloadDirectory": "C:\\Admin" },
            "uninstallBehavior": { "purgePortablePackage": false }
        })");
    }

    UserSettingsTest settings;
    const std::filesystem::path sharedRoot = adminPresent ? LR"(C:\Admin)" : (userPresent && !fullToken ? LR"(C:\User)" : L"");
    const std::filesystem::path splitRoot = fullToken ?
        (adminPresent ? LR"(C:\Admin)" : L"") : (userPresent ? LR"(C:\User)" : L"");
    REQUIRE(settings.Get<Setting::ProgressBarVisualStyle>() ==
        (adminPresent ? VisualStyle::Retro : (userPresent ? VisualStyle::Rainbow : VisualStyle::Accent)));
    REQUIRE(settings.Get<Setting::PortablePackageUserRoot>() == sharedRoot);
    REQUIRE(settings.Get<Setting::PortablePackageMachineRoot>() == sharedRoot);
    REQUIRE(settings.Get<Setting::DownloadDefaultDirectory>() == sharedRoot);
    REQUIRE(settings.Get<Setting::InstallDefaultRoot>() == splitRoot);
    REQUIRE(settings.Get<Setting::ConfigureDefaultModuleRoot>() == splitRoot);
    REQUIRE(settings.Get<Setting::UninstallPurgePortablePackage>() == (!adminPresent && userPresent && !fullToken));
    REQUIRE(settings.GetType() == (userPresent ?
        (userBackup ? UserSettingsType::Backup : UserSettingsType::Standard) : UserSettingsType::Default));
    REQUIRE(MigrationWarningCount(settings) == (fullToken && userPresent && !adminPresent ? 6u : 0u));
    for (const auto& warning : settings.GetWarnings())
    {
        if (warning.IsMigrationWarning)
        {
            REQUIRE(warning.Message == AppInstaller::StringResource::String::SettingsWarningIgnoredWithFullToken);
            REQUIRE(warning.Source == (userBackup ? Stream::BackupUserSettings.Name : Stream::PrimaryUserSettings.Name));
            REQUIRE(warning.Data.empty());
        }
    }
}

TEST_CASE("UserSettingsOverlay_ExplicitValuesAndSiblings", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(GENERATE(false, true));
    SetSetting(Stream::PrimaryUserSettings, R"({
        "visual": { "anonymizeDisplayedPaths": true },
        "source": { "autoUpdateIntervalInMinutes": 23 },
        "installBehavior": {
            "preferences": { "architectures": ["x86"], "locale": ["en-US", "fr-FR"], "scope": "machine" },
            "requirements": { "locale": ["en-US", "fr-FR"] }
        }
    })");
    SetAdminSettingsFile(Stream::PrimaryAdminSettingsFile, R"({
        "visual": { "anonymizeDisplayedPaths": false },
        "source": { "autoUpdateIntervalInMinutes": 0 },
        "installBehavior": {
            "preferences": { "architectures": [] },
            "requirements": { "locale": ["de-DE"] }
        }
    })");

    UserSettingsTest settings;
    REQUIRE_FALSE(settings.Get<Setting::AnonymizePathForDisplay>());
    REQUIRE(settings.Get<Setting::AutoUpdateTimeInMinutes>() == 0min);
    REQUIRE(settings.Get<Setting::InstallArchitecturePreference>().empty());
    REQUIRE(settings.Get<Setting::InstallLocalePreference>() == std::vector<std::string>{ "en-US", "fr-FR" });
    REQUIRE(settings.Get<Setting::InstallLocaleRequirement>() == std::vector<std::string>{ "de-DE" });
    REQUIRE(settings.Get<Setting::InstallScopePreference>() == AppInstaller::Manifest::ScopeEnum::Machine);
    REQUIRE(settings.GetWarnings().empty());
}

TEST_CASE("UserSettingsOverlay_InvalidAdminValueBlocksUserFallback", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    const bool fullToken = GENERATE(false, true);
    const std::string value = GENERATE("17"s, R"("relative")"s);
    CAPTURE(fullToken, value);
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(fullToken);
    SetSetting(Stream::PrimaryUserSettings, R"({
        "downloadBehavior": { "defaultDownloadDirectory": "C:\\User" }
    })");
    SetAdminSettingsFile(Stream::PrimaryAdminSettingsFile,
        R"({ "downloadBehavior": { "defaultDownloadDirectory": )" + value + " } }");
    SetAdminSettingsFile(Stream::BackupAdminSettingsFile, R"({
        "downloadBehavior": { "defaultDownloadDirectory": "C:\\Backup" }
    })");

    UserSettingsTest settings;
    REQUIRE(settings.Get<Setting::DownloadDefaultDirectory>().empty());
    const auto& warnings = settings.GetWarnings();
    REQUIRE(warnings.size() == (fullToken ? 2u : 1u));
    REQUIRE(warnings[0].Source == Stream::PrimaryAdminSettingsFile.Name);
    REQUIRE(warnings[0].Path == ".downloadBehavior.defaultDownloadDirectory");
    REQUIRE(warnings[0].Message == (value == R"("relative")" ?
        AppInstaller::StringResource::String::SettingsWarningInvalidFieldValue :
        AppInstaller::StringResource::String::SettingsWarningInvalidFieldFormat));
    REQUIRE(MigrationWarningCount(settings) == (fullToken ? 1u : 0u));
}

TEST_CASE("UserSettingsOverlay_InvalidUnrestrictedAdminValueBlocksUserFallback", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(GENERATE(false, true));
    SetSetting(Stream::PrimaryUserSettings, R"({ "visual": { "progressBar": "rainbow" } })");
    SetAdminSettingsFile(Stream::PrimaryAdminSettingsFile, R"({ "visual": { "progressBar": "invalid" } })");

    UserSettingsTest settings;
    REQUIRE(settings.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Accent);
    REQUIRE(settings.GetWarnings().size() == 1);
    REQUIRE(settings.GetWarnings()[0].Source == Stream::PrimaryAdminSettingsFile.Name);
    REQUIRE(MigrationWarningCount(settings) == 0);
}

TEST_CASE("UserSettingsOverlay_AdminNullSelectsDefault", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    const bool fullToken = GENERATE(false, true);
    CAPTURE(fullToken);
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(fullToken);
    SetSetting(Stream::PrimaryUserSettings, R"({
        "downloadBehavior": { "defaultDownloadDirectory": "C:\\User" },
        "installBehavior": { "defaultInstallRoot": "C:\\User" }
    })");
    SetAdminSettingsFile(Stream::PrimaryAdminSettingsFile, R"({
        "downloadBehavior": { "defaultDownloadDirectory": null },
        "installBehavior": { "defaultInstallRoot": null }
    })");
    SetAdminSettingsFile(Stream::BackupAdminSettingsFile, R"({
        "downloadBehavior": { "defaultDownloadDirectory": "C:\\Backup" },
        "installBehavior": { "defaultInstallRoot": "C:\\Backup" }
    })");

    UserSettingsTest settings;
    REQUIRE(settings.Get<Setting::DownloadDefaultDirectory>().empty());
    const std::filesystem::path expectedRoot = fullToken ? L"" : LR"(C:\User)";
    REQUIRE(settings.Get<Setting::InstallDefaultRoot>() == expectedRoot);
    REQUIRE(settings.GetWarnings().empty());
}

TEST_CASE("UserSettingsOverlay_ResolveMissingAndNull", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(GENERATE(false, true));
    const std::string preferences = GENERATE("{}"s, "null"s, "[]"s, R"({ "scope": null })"s);
    CAPTURE(preferences);
    const bool presentNull = preferences == R"({ "scope": null })";
    SetSetting(Stream::PrimaryUserSettings, R"({
        "installBehavior": { "preferences": { "scope": "machine" } }
    })");
    SetAdminSettingsFile(Stream::PrimaryAdminSettingsFile,
        R"({ "installBehavior": { "preferences": )" + preferences + " } }");

    UserSettingsTest settings;
    REQUIRE(settings.Get<Setting::InstallScopePreference>() == (presentNull ?
        AppInstaller::Manifest::ScopeEnum::User : AppInstaller::Manifest::ScopeEnum::Machine));
    REQUIRE(settings.GetWarnings().empty());
}

TEST_CASE("UserSettingsOverlay_IgnoredNullIsPresent", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    const bool fullToken = GENERATE(false, true);
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(fullToken);
    SetSetting(Stream::PrimaryUserSettings, R"({
        "downloadBehavior": { "defaultDownloadDirectory": null }
    })");

    UserSettingsTest settings;
    REQUIRE(settings.Get<Setting::DownloadDefaultDirectory>().empty());
    REQUIRE(settings.GetWarnings().size() == (fullToken ? 1u : 0u));
    REQUIRE(MigrationWarningCount(settings) == (fullToken ? 1u : 0u));
    if (fullToken)
    {
        REQUIRE(settings.GetWarnings()[0].Path == ".downloadBehavior.defaultDownloadDirectory");
        REQUIRE(settings.GetWarnings()[0].Source == Stream::PrimaryUserSettings.Name);
    }
}

TEST_CASE("UserSettingsOverlay_IneligibleSplitFields", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    const bool fullToken = GENERATE(false, true);
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(fullToken);
    SetSetting(Stream::PrimaryUserSettings, R"({
        "installBehavior": { "defaultInstallRoot": "C:\\User" },
        "configureBehavior": { "defaultModuleRoot": "C:\\User" }
    })");
    SetAdminSettingsFile(Stream::PrimaryAdminSettingsFile, R"({
        "installBehavior": { "defaultInstallRoot": 17 },
        "configureBehavior": { "defaultModuleRoot": null }
    })");

    UserSettingsTest settings;
    const std::filesystem::path expectedRoot = fullToken ? L"" : LR"(C:\User)";
    REQUIRE(settings.Get<Setting::InstallDefaultRoot>() == expectedRoot);
    REQUIRE(settings.Get<Setting::ConfigureDefaultModuleRoot>() == expectedRoot);
    REQUIRE(settings.GetWarnings().size() == (fullToken ? 2u : 0u));
    REQUIRE(MigrationWarningCount(settings) == (fullToken ? 1u : 0u));
}

TEST_CASE("UserSettingsOverlay_ProtectedFileRecovery", "[settings]")
{
    enum class FileState { Missing, Valid, Malformed, InvalidRoot };
    auto files = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(GENERATE(false, true));
    const auto primary = GENERATE(FileState::Missing, FileState::Valid, FileState::Malformed, FileState::InvalidRoot);
    const auto backup = GENERATE(FileState::Missing, FileState::Valid, FileState::Malformed, FileState::InvalidRoot);
    CAPTURE(primary, backup);
    SetSetting(Stream::PrimaryUserSettings, R"({
        "visual": { "progressBar": "rainbow" },
        "source": { "autoUpdateIntervalInMinutes": 23 }
    })");

    if (primary != FileState::Missing)
    {
        SetAdminSettingsFile(Stream::PrimaryAdminSettingsFile, primary == FileState::Valid ?
            R"({ "visual": { "progressBar": "retro" } })" : (primary == FileState::Malformed ? "{" : "[]"));
    }
    if (backup != FileState::Missing)
    {
        SetAdminSettingsFile(Stream::BackupAdminSettingsFile, backup == FileState::Valid ?
            R"({ "visual": { "progressBar": "rainbow" } })" : (backup == FileState::Malformed ? "{" : "null"));
    }

    const bool parseFailure = primary != FileState::Valid && backup != FileState::Valid &&
        (primary != FileState::Missing || backup != FileState::Missing);
    UserSettingsTest settings;
    const bool recovered = primary != FileState::Valid && backup == FileState::Valid;
    REQUIRE(settings.GetType() == UserSettingsType::Standard);
    REQUIRE(settings.Get<Setting::ProgressBarVisualStyle>() ==
        (parseFailure ? VisualStyle::Accent : (primary == FileState::Valid ? VisualStyle::Retro : VisualStyle::Rainbow)));
    REQUIRE(settings.Get<Setting::AutoUpdateTimeInMinutes>() == (parseFailure ? 15min : 23min));
    REQUIRE(MigrationWarningCount(settings) == 0);
    if (parseFailure)
    {
        const auto& warning = settings.GetWarnings().back();
        REQUIRE(warning.Message == AppInstaller::StringResource::String::SettingsWarningUsingDefault);
        REQUIRE(warning.Source == (primary != FileState::Missing ?
            Stream::PrimaryAdminSettingsFile.Name : Stream::BackupAdminSettingsFile.Name));
    }
    else if (recovered)
    {
        const auto& warning = settings.GetWarnings().back();
        REQUIRE(warning.Message == AppInstaller::StringResource::String::SettingsWarningLoadedBackupSettings);
        REQUIRE(warning.Source == Stream::BackupAdminSettingsFile.Name);
        REQUIRE(warning.Path == Stream::BackupAdminSettingsFile.Name);
        REQUIRE_FALSE(warning.IsFieldWarning);
    }
    else
    {
        REQUIRE(settings.GetWarnings().empty());
    }
}

TEST_CASE("UserSettingsOverlay_UserFileRequiresObject", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(GENERATE(false, true));
    const std::string root = GENERATE("null"s, "[]"s);
    const bool validBackup = GENERATE(false, true);
    CAPTURE(root, validBackup);
    SetSetting(Stream::PrimaryUserSettings, root);
    if (validBackup)
    {
        SetSetting(Stream::BackupUserSettings, R"({ "visual": { "progressBar": "rainbow" } })");
    }

    UserSettingsTest settings;
    REQUIRE(settings.GetType() == (validBackup ? UserSettingsType::Backup : UserSettingsType::Default));
    REQUIRE(settings.Get<Setting::ProgressBarVisualStyle>() == (validBackup ? VisualStyle::Rainbow : VisualStyle::Accent));
    const auto& warnings = settings.GetWarnings();
    REQUIRE(warnings.size() == 2);
    REQUIRE(warnings[0].Message == AppInstaller::StringResource::String::SettingsWarningInvalidFileFormat);
    REQUIRE(warnings[0].Source == Stream::PrimaryUserSettings.Name);
    REQUIRE(warnings.back().Message == (validBackup ?
        AppInstaller::StringResource::String::SettingsWarningLoadedBackupSettings :
        AppInstaller::StringResource::String::SettingsWarningUsingDefault));
}

TEST_CASE("UserSettingsOverlay_ProtectedReadFailure", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(GENERATE(false, true));
    const bool lockPrimary = GENERATE(false, true);
    const bool validBackup = GENERATE(false, true);
    SetSetting(Stream::PrimaryUserSettings, R"({
        "visual": { "progressBar": "retro" },
        "source": { "autoUpdateIntervalInMinutes": 23 }
    })");
    const auto& lockedStream = lockPrimary ? Stream::PrimaryAdminSettingsFile : Stream::BackupAdminSettingsFile;
    SetAdminSettingsFile(lockedStream, s_goodJson);
    if (lockPrimary && validBackup)
    {
        SetAdminSettingsFile(Stream::BackupAdminSettingsFile, R"({ "visual": { "progressBar": "rainbow" } })");
    }

    auto lockedFile = LockSettingsFile(lockedStream);
    UserSettingsTest settings;
    REQUIRE(settings.GetType() == UserSettingsType::Standard);
    REQUIRE(settings.GetWarnings().size() == 2);
    REQUIRE(settings.GetWarnings()[0].Message == AppInstaller::StringResource::String::SettingsWarningReadError);
    REQUIRE(settings.GetWarnings()[0].Source == lockedStream.Name);
    if (lockPrimary && validBackup)
    {
        REQUIRE(settings.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Rainbow);
        REQUIRE(settings.Get<Setting::AutoUpdateTimeInMinutes>() == 23min);
        REQUIRE(settings.GetWarnings()[1].Message == AppInstaller::StringResource::String::SettingsWarningLoadedBackupSettings);
    }
    else
    {
        REQUIRE(settings.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Accent);
        REQUIRE(settings.Get<Setting::AutoUpdateTimeInMinutes>() == 15min);
        REQUIRE(settings.GetWarnings()[1].Message == AppInstaller::StringResource::String::SettingsWarningUsingDefault);
    }
}

TEST_CASE("UserSettingsOverlay_UserReadFailure", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(GENERATE(false, true));
    const bool readableBackup = GENERATE(false, true);
    SetSetting(Stream::PrimaryUserSettings, R"({ "visual": { "progressBar": "rainbow" } })");
    SetSetting(Stream::BackupUserSettings, R"({ "visual": { "progressBar": "retro" } })");
    auto lockedPrimary = LockSettingsFile(Stream::PrimaryUserSettings);
    wil::unique_hfile lockedBackup;
    if (!readableBackup)
    {
        lockedBackup = LockSettingsFile(Stream::BackupUserSettings);
    }

    UserSettingsTest settings;
    REQUIRE(settings.GetType() == (readableBackup ? UserSettingsType::Backup : UserSettingsType::Default));
    REQUIRE(settings.Get<Setting::ProgressBarVisualStyle>() == (readableBackup ? VisualStyle::Retro : VisualStyle::Accent));
    const auto& warnings = settings.GetWarnings();
    REQUIRE(warnings.size() == (readableBackup ? 2u : 3u));
    REQUIRE(warnings[0].Message == AppInstaller::StringResource::String::SettingsWarningReadError);
    REQUIRE(warnings[0].Source == Stream::PrimaryUserSettings.Name);
    if (!readableBackup)
    {
        REQUIRE(warnings[1].Message == AppInstaller::StringResource::String::SettingsWarningReadError);
        REQUIRE(warnings[1].Source == Stream::BackupUserSettings.Name);
    }
    REQUIRE(warnings.back().Message == (readableBackup ?
        AppInstaller::StringResource::String::SettingsWarningLoadedBackupSettings :
        AppInstaller::StringResource::String::SettingsWarningUsingDefault));
    REQUIRE(warnings.back().Source == (readableBackup ? Stream::BackupUserSettings.Name : Stream::PrimaryUserSettings.Name));
}

TEST_CASE("UserSettingsOverlay_CustomContentBypassesFiles", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(GENERATE(false, true));
    const bool validContent = GENERATE(false, true);
    SetSetting(Stream::PrimaryUserSettings, R"({ "visual": { "progressBar": "retro" } })");
    SetAdminSettingsFile(Stream::PrimaryAdminSettingsFile, s_badJson);
    SetAdminSettingsFile(Stream::BackupAdminSettingsFile, s_badJson);

    UserSettingsTest settings{ validContent ? R"({
        "visual": { "progressBar": "rainbow" },
        "configureBehavior": { "defaultModuleRoot": "C:\\Custom" },
        "downloadBehavior": { "defaultDownloadDirectory": "C:\\Custom" }
    })"s : "{"s };
    REQUIRE(settings.GetType() == (validContent ? UserSettingsType::Custom : UserSettingsType::Default));
    REQUIRE(settings.Get<Setting::ProgressBarVisualStyle>() == (validContent ? VisualStyle::Rainbow : VisualStyle::Accent));
    const std::filesystem::path expectedRoot = validContent ? LR"(C:\Custom)" : L"";
    REQUIRE(settings.Get<Setting::ConfigureDefaultModuleRoot>() == expectedRoot);
    REQUIRE(settings.Get<Setting::DownloadDefaultDirectory>() == expectedRoot);
    REQUIRE(settings.GetWarnings().size() == (validContent ? 0u : 1u));
    REQUIRE(MigrationWarningCount(settings) == 0);
}

TEST_CASE("UserSettingsOverlay_GroupPolicyPrecedence", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(GENERATE(false, true));
    const bool custom = GENERATE(false, true);
    GroupPolicyTestOverride policies;
    policies.SetValue<ValuePolicy::SourceAutoUpdateIntervalInMinutes>(300);
    SetSetting(Stream::PrimaryUserSettings, R"({ "source": { "autoUpdateIntervalInMinutes": 5 } })");
    SetAdminSettingsFile(Stream::PrimaryAdminSettingsFile, R"({ "source": { "autoUpdateIntervalInMinutes": "invalid" } })");

    UserSettingsTest settings{ custom ? std::optional<std::string>{ R"({ "source": { "autoUpdateIntervalInMinutes": 7 } })" } : std::nullopt };
    REQUIRE(settings.Get<Setting::AutoUpdateTimeInMinutes>() == 300min);
    REQUIRE(settings.GetWarnings().empty());
}

TEST_CASE("UserSettingsOverlay_DisabledSettingsBypassFilesAndCustomContent", "[settings]")
{
    auto files = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(GENERATE(false, true));
    const bool custom = GENERATE(false, true);
    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::Settings, PolicyState::Disabled);
    SetSetting(Stream::PrimaryUserSettings, s_badJson);
    SetAdminSettingsFile(Stream::PrimaryAdminSettingsFile, s_badJson);
    SetAdminSettingsFile(Stream::BackupAdminSettingsFile, s_badJson);

    UserSettingsTest settings{ custom ? std::optional<std::string>{ "{" } : std::nullopt };
    REQUIRE(settings.GetType() == UserSettingsType::Default);
    REQUIRE(settings.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Accent);
    REQUIRE(settings.GetWarnings().empty());
}

TEST_CASE("UserSettingsFilePaths", "[settings]")
{
    auto settingsPath = UserSettings::SettingsFilePath();
    auto expectedPath = GetPathTo(PathName::UserFileSettings) / "settings.json";
    REQUIRE(settingsPath == expectedPath);
    REQUIRE(UserSettings::AdminSettingsFilePath() == GetPathTo(Stream::PrimaryAdminSettingsFile));
}

TEST_CASE("UserSettingsType", "[settings]")
{
    // These are all the possible combinations between (7 of them are impossible):
    // 1 - No settings.json file exists
    // 2 - Bad settings.json file
    // 3 - No settings.json.backup file exists
    // 4 - Bad settings.json.backup file exists.
    auto again = DeleteUserSettingsFiles();

    SECTION("No setting.json No setting.json.backup")
    {
        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Default);
    }
    SECTION("No setting.json Bad setting.json.backup")
    {
        SetSetting(Stream::BackupUserSettings, s_badJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Default);
    }
    SECTION("No setting.json Good setting.json.backup")
    {
        SetSetting(Stream::BackupUserSettings, s_goodJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Backup);
    }
    SECTION("Bad setting.json No setting.json.backup")
    {
        SetSetting(Stream::PrimaryUserSettings, s_badJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Default);
    }
    SECTION("Bad setting.json Bad setting.json.backup")
    {
        SetSetting(Stream::PrimaryUserSettings, s_badJson);
        SetSetting(Stream::BackupUserSettings, s_badJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Default);
    }
    SECTION("Bad setting.json Good setting.json.backup")
    {
        SetSetting(Stream::PrimaryUserSettings, s_badJson);
        SetSetting(Stream::BackupUserSettings, s_goodJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Backup);
    }
    SECTION("Good setting.json No setting.json.backup")
    {
        SetSetting(Stream::PrimaryUserSettings, s_goodJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Standard);
    }
    SECTION("Good setting.json Bad setting.json.backup")
    {
        SetSetting(Stream::PrimaryUserSettings, s_goodJson);
        SetSetting(Stream::BackupUserSettings, s_badJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Standard);
    }
    SECTION("Good setting.json Good setting.json.backup")
    {
        SetSetting(Stream::PrimaryUserSettings, s_goodJson);
        SetSetting(Stream::BackupUserSettings, s_goodJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Standard);
    }
}

TEST_CASE("UserSettingsCreateFiles", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    auto settingsPath = UserSettings::SettingsFilePath();
    auto settingsBackupPath = GetPathTo(Stream::BackupUserSettings);

    SECTION("No settings.json create new")
    {
        REQUIRE(!std::filesystem::exists(settingsPath));
        REQUIRE(!std::filesystem::exists(settingsBackupPath));

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Default);
        userSettingTest.PrepareToShellExecuteFile();

        REQUIRE(std::filesystem::exists(settingsPath));
        REQUIRE(!std::filesystem::exists(settingsBackupPath));
    }
    SECTION("Good settings.json create new backup")
    {
        SetSetting(Stream::PrimaryUserSettings, s_goodJson);
        REQUIRE(std::filesystem::exists(settingsPath));
        REQUIRE(!std::filesystem::exists(settingsBackupPath));

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Standard);
        userSettingTest.PrepareToShellExecuteFile();

        REQUIRE(std::filesystem::exists(settingsPath));
        REQUIRE(std::filesystem::exists(settingsBackupPath));
    }
}

TEST_CASE("SettingProgressBar", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    SECTION("Default value")
    {
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Accent);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Accent")
    {
        std::string_view json = R"({ "visual": { "progressBar": "accent" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Accent);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Rainbow")
    {
        std::string_view json = R"({ "visual": { "progressBar": "rainbow" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Rainbow);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("retro")
    {
        std::string_view json = R"({ "visual": { "progressBar": "retro" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Retro);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Bad value")
    {
        std::string_view json = R"({ "visual": { "progressBar": "fake" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Accent);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Bad value type")
    {
        std::string_view json = R"({ "visual": { "progressBar": 5 } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Accent);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
}

TEST_CASE("SettingsAnonymizePathForDisplay", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    SECTION("Default")
    {
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AnonymizePathForDisplay>() == true);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("True")
    {
        std::string_view json = R"({ "visual": { "anonymizeDisplayedPaths": true } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AnonymizePathForDisplay>() == true);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("False")
    {
        std::string_view json = R"({ "visual": { "anonymizeDisplayedPaths": false } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AnonymizePathForDisplay>() == false);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Invalid Value")
    {
        std::string_view json = R"({ "visual": { "anonymizeDisplayedPaths": "notBoolean" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AnonymizePathForDisplay>() == true);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
}

TEST_CASE("SettingLoggingLevelPreference", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    SECTION("Default value")
    {
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingLevelPreference>() == Level::Info);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Info")
    {
        std::string_view json = R"({ "logging": { "level": "info" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingLevelPreference>() == Level::Info);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Verbose")
    {
        std::string_view json = R"({ "logging": { "level": "verbose" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingLevelPreference>() == Level::Verbose);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Warning")
    {
        std::string_view json = R"({ "logging": { "level": "warning" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingLevelPreference>() == Level::Warning);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Error")
    {
        std::string_view json = R"({ "logging": { "level": "error" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingLevelPreference>() == Level::Error);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Critical")
    {
        std::string_view json = R"({ "logging": { "level": "critical" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingLevelPreference>() == Level::Crit);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Bad value")
    {
        std::string_view json = R"({ "logging": { "level": "fake" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingLevelPreference>() == Level::Info);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Bad value type")
    {
        std::string_view json = R"({ "logging": { "level": 5 } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingLevelPreference>() == Level::Info);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
}

TEST_CASE("SettingLoggingFileNameStrategy", "[settings]") {
    auto again = DeleteUserSettingsFiles();

    SECTION("Default value")
    {
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingFileNameStrategy>() == LogNameStrategy::Manifest);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Manifest")
    {
        std::string_view json = R"({ "logging": { "fileNameStrategy": "manifest" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingFileNameStrategy>() == LogNameStrategy::Manifest);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Timestamp")
    {
        std::string_view json = R"({ "logging": { "fileNameStrategy": "timestamp" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingFileNameStrategy>() == LogNameStrategy::Timestamp);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Guid")
    {
        std::string_view json = R"({ "logging": { "fileNameStrategy": "guid" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingFileNameStrategy>() == LogNameStrategy::Guid);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Short Guid")
    {
        std::string_view json = R"({ "logging": { "fileNameStrategy": "shortguid" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingFileNameStrategy>() == LogNameStrategy::ShortGuid);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Bad value")
    {
        std::string_view json = R"({ "logging": { "fileNameStrategy": "fake" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingFileNameStrategy>() == LogNameStrategy::Manifest);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Bad value type")
    {
        std::string_view json = R"({ "logging": { "fileNameStrategy": 5 } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingFileNameStrategy>() == LogNameStrategy::Manifest);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
}

TEST_CASE("SettingAutoUpdateIntervalInMinutes", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    constexpr static auto cinq = 5min;
    constexpr static auto cero = 0min;
    constexpr static auto threehundred = 300min;

    std::chrono::minutes defaultAutoUpdateTime{};

    {
        SetSetting(Stream::PrimaryUserSettings, "");
        UserSettingsTest userSettingTest;
        defaultAutoUpdateTime = userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>();
    }

    SECTION("Valid value 0")
    {
        std::string_view json = R"({ "source": { "autoUpdateIntervalInMinutes": 0 } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == cero);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Valid value 300")
    {
        std::string_view json = R"({ "source": { "autoUpdateIntervalInMinutes": 300 } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == threehundred);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Invalid type negative integer")
    {
        std::string_view json = R"({ "source": { "autoUpdateIntervalInMinutes": -20 } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == defaultAutoUpdateTime);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Invalid type string")
    {
        std::string_view json = R"({ "source": { "autoUpdateIntervalInMinutes": "not a number" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == defaultAutoUpdateTime);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Overridden by Group Policy")
    {
        auto policiesKey = RegCreateVolatileTestRoot();
        SetRegistryValue(policiesKey.get(), SourceUpdateIntervalPolicyValueName, (DWORD)threehundred.count());
        GroupPolicyTestOverride policies{ policiesKey.get() };

        std::string_view json = R"({ "source": { "autoUpdateIntervalInMinutes": 5 } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == threehundred);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Invalid Group Policy")
    {
        auto policiesKey = RegCreateVolatileTestRoot();
        SetRegistryValue(policiesKey.get(), SourceUpdateIntervalPolicyValueName, L"Not a number"s);
        GroupPolicyTestOverride policies{ policiesKey.get() };

        std::string_view json = R"({ "source": { "autoUpdateIntervalInMinutes": 5 } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == cinq);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
}

TEST_CASE("SettingsExperimentalCmd", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    SECTION("Feature off default")
    {
        UserSettingsTest userSettingTest;

        REQUIRE(!userSettingTest.Get<Setting::EFExperimentalCmd>());
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Feature on")
    {
        std::string_view json = R"({ "experimentalFeatures": { "experimentalCmd": true } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::EFExperimentalCmd>());
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Feature off")
    {
        std::string_view json = R"({ "experimentalFeatures": { "experimentalCmd": false } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(!userSettingTest.Get<Setting::EFExperimentalCmd>());
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Invalid value")
    {
        std::string_view json = R"({ "experimentalFeatures": { "experimentalCmd": "string" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(!userSettingTest.Get<Setting::EFExperimentalCmd>());
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Disabled by group policy")
    {
        auto policiesKey = RegCreateVolatileTestRoot();
        SetRegistryValue(policiesKey.get(), SourceUpdateIntervalPolicyValueName, L"Not a number"s);
        GroupPolicyTestOverride policies{ policiesKey.get() };

        std::string_view json = R"({ "experimentalFeatures": { "experimentalCmd": true } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        // Experimental features group policy is applied at the ExperimentalFeature level,
        // so it doesn't affect the settings.
        REQUIRE(userSettingTest.Get<Setting::EFExperimentalCmd>());
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
}

TEST_CASE("SettingsPortablePackageUserRoot", "[settings]")
{
    auto again = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(false);

    SECTION("Relative path")
    {
        std::string_view json = R"({ "installBehavior": { "portablePackageUserRoot": "%LOCALAPPDATA%/Portable/Root" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;
        
        REQUIRE(userSettingTest.Get<Setting::PortablePackageUserRoot>().empty());

        auto warnings = userSettingTest.GetWarnings();
        REQUIRE(warnings.size() == 1);
        REQUIRE(warnings[0].Message == AppInstaller::StringResource::String::SettingsWarningInvalidFieldValue);
        REQUIRE(warnings[0].Path == ".installBehavior.portablePackageUserRoot");
    }
    SECTION("Valid path")
    {
        std::string_view json = R"({ "installBehavior": { "portablePackageUserRoot": "C:/Foo/Bar" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::PortablePackageUserRoot>() == "C:/Foo/Bar");
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
}

TEST_CASE("SettingsPortablePackageMachineRoot", "[settings]")
{
    auto again = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(false);

    SECTION("Relative path")
    {
        std::string_view json = R"({ "installBehavior": { "portablePackageMachineRoot": "%LOCALAPPDATA%/Portable/Root" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::PortablePackageMachineRoot>().empty());

        auto warnings = userSettingTest.GetWarnings();
        REQUIRE(warnings.size() == 1);
        REQUIRE(warnings[0].Message == AppInstaller::StringResource::String::SettingsWarningInvalidFieldValue);
        REQUIRE(warnings[0].Path == ".installBehavior.portablePackageMachineRoot");
    }
    SECTION("Valid path")
    {
        std::string_view json = R"({ "installBehavior": { "portablePackageMachineRoot": "C:/Foo/Bar" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::PortablePackageMachineRoot>() == "C:/Foo/Bar");
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
}

TEST_CASE("SettingsDownloadDefaultDirectory", "[settings]")
{
    auto again = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(false);

    SECTION("Valid path")
    {
        std::string_view json = R"({ "downloadBehavior": { "defaultDownloadDirectory": "C:/Foo/Bar" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::DownloadDefaultDirectory>() == "C:/Foo/Bar");
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
}

TEST_CASE("SettingsConfigureDefaultModuleRoot", "[settings]")
{
    auto again = DeleteUserSettingsFiles();
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(false);

    SECTION("Valid path")
    {
        std::string_view json = R"({ "configureBehavior": { "defaultModuleRoot": "C:/Foo/Bar" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ConfigureDefaultModuleRoot>() == "C:/Foo/Bar");
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
}

TEST_CASE("SettingsArchiveExtractionMethod", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    SECTION("Shell api")
    {
        std::string_view json = R"({ "installBehavior": { "archiveExtractionMethod": "shellApi" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ArchiveExtractionMethod>() == AppInstaller::Archive::ExtractionMethod::ShellApi);
    }
    SECTION("Shell api")
    {
        std::string_view json = R"({ "installBehavior": { "archiveExtractionMethod": "tar" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ArchiveExtractionMethod>() == AppInstaller::Archive::ExtractionMethod::Tar);
    }
}

TEST_CASE("SettingsInstallScope", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    SECTION("User scope preference")
    {
        std::string_view json = R"({ "installBehavior": { "preferences": { "scope": "user" } } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::InstallScopePreference>() == AppInstaller::Manifest::ScopeEnum::User);
    }
    SECTION("Machine scope preference")
    {
        std::string_view json = R"({ "installBehavior": { "preferences": { "scope": "machine" } } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::InstallScopePreference>() == AppInstaller::Manifest::ScopeEnum::Machine);
    }
    SECTION("User scope requirement")
    {
        std::string_view json = R"({ "installBehavior": { "requirements": { "scope": "user" } } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::InstallScopeRequirement>() == AppInstaller::Manifest::ScopeEnum::User);
    }
    SECTION("Machine scope requirement")
    {
        std::string_view json = R"({ "installBehavior": { "requirements": { "scope": "machine" } } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::InstallScopeRequirement>() == AppInstaller::Manifest::ScopeEnum::Machine);
    }
}

TEST_CASE("SettingsMaxResumes", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    SECTION("Modify max number of resumes")
    {
        std::string_view json = R"({ "installBehavior": { "maxResumes": 5 } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::MaxResumes>() == 5);
    }
}

TEST_CASE("LoggingChannels", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    SECTION("Not provided")
    {
        std::string_view json = R"({ })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingChannelPreference>() == Channel::Defaults);
    }
    SECTION("No channels")
    {
        std::string_view json = R"({ "logging": { "channels": [] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingChannelPreference>() == Channel::None);
    }
    SECTION("Default")
    {
        std::string_view json = R"({ "logging": { "channels": ["default"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingChannelPreference>() == Channel::Defaults);
    }
    SECTION("Multiple")
    {
        std::string_view json = R"({ "logging": { "channels": ["core","Repo","YAML"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingChannelPreference>() == (Channel::Core | Channel::Repo | Channel::YAML));
    }
    SECTION("Some invalid")
    {
        std::string_view json = R"({ "logging": { "channels": ["cli","sql","INVALID"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::LoggingChannelPreference>() == (Channel::CLI | Channel::SQL));
    }
}

TEST_CASE("SettingOutputSortOrder", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    SECTION("Default value - empty vector sentinel for context-aware defaults")
    {
        UserSettingsTest userSettingTest;

        auto sortOrder = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(sortOrder.empty());
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Single field")
    {
        std::string_view json = R"({ "output": { "sortOrder": ["id"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        auto sortOrder = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(sortOrder.size() == 1);
        REQUIRE(sortOrder[0] == SortField::Id);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Relevance field")
    {
        std::string_view json = R"({ "output": { "sortOrder": ["relevance"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        auto sortOrder = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(sortOrder.size() == 1);
        REQUIRE(sortOrder[0] == SortField::Relevance);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Multiple fields")
    {
        std::string_view json = R"({ "output": { "sortOrder": ["available", "name"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        auto sortOrder = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(sortOrder.size() == 2);
        REQUIRE(sortOrder[0] == SortField::Available);
        REQUIRE(sortOrder[1] == SortField::Name);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("All valid fields")
    {
        std::string_view json = R"({ "output": { "sortOrder": ["relevance", "name", "id", "version", "source", "available"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        auto sortOrder = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(sortOrder.size() == 6);
        REQUIRE(sortOrder[0] == SortField::Relevance);
        REQUIRE(sortOrder[1] == SortField::Name);
        REQUIRE(sortOrder[2] == SortField::Id);
        REQUIRE(sortOrder[3] == SortField::Version);
        REQUIRE(sortOrder[4] == SortField::Source);
        REQUIRE(sortOrder[5] == SortField::Available);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Case insensitive")
    {
        std::string_view json = R"({ "output": { "sortOrder": ["Name", "ID", "VERSION"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        auto sortOrder = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(sortOrder.size() == 3);
        REQUIRE(sortOrder[0] == SortField::Name);
        REQUIRE(sortOrder[1] == SortField::Id);
        REQUIRE(sortOrder[2] == SortField::Version);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Empty array disables sorting")
    {
        std::string_view json = R"({ "output": { "sortOrder": [] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        auto sortOrder = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(sortOrder.size() == 0);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Invalid field name")
    {
        std::string_view json = R"({ "output": { "sortOrder": ["invalid"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        auto sortOrder = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(sortOrder.empty());
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Mixed valid and invalid field")
    {
        std::string_view json = R"({ "output": { "sortOrder": ["name", "bogus"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        auto sortOrder = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(sortOrder.empty());
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Duplicate values - case-insensitive")
    {
        std::string_view json = R"({ "output": { "sortOrder": ["name", "id", "Name"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        auto sortOrder = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(sortOrder.size() == 3);
        REQUIRE(sortOrder[0] == SortField::Name);
        REQUIRE(sortOrder[1] == SortField::Id);
        REQUIRE(sortOrder[2] == SortField::Name);
        REQUIRE(userSettingTest.GetWarnings().empty());
    }
    SECTION("Wrong type - string instead of array")
    {
        std::string_view json = R"({ "output": { "sortOrder": "name" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        auto sortOrder = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(sortOrder.empty());
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Wrong type - number in array")
    {
        std::string_view json = R"({ "output": { "sortOrder": [1] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        auto sortOrder = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(sortOrder.empty());
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
}

TEST_CASE("SettingOutputSortDirection", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    SECTION("Default value")
    {
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::OutputSortDirection>() == SortDirection::Ascending);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Ascending")
    {
        std::string_view json = R"({ "output": { "sortDirection": "ascending" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::OutputSortDirection>() == SortDirection::Ascending);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Descending")
    {
        std::string_view json = R"({ "output": { "sortDirection": "descending" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::OutputSortDirection>() == SortDirection::Descending);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Case insensitive")
    {
        std::string_view json = R"({ "output": { "sortDirection": "DESCENDING" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::OutputSortDirection>() == SortDirection::Descending);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Invalid value")
    {
        std::string_view json = R"({ "output": { "sortDirection": "sideways" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::OutputSortDirection>() == SortDirection::Ascending);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Wrong type - array instead of string")
    {
        std::string_view json = R"({ "output": { "sortDirection": ["ascending"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::OutputSortDirection>() == SortDirection::Ascending);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
}

TEST_CASE("SettingOutputLocale", "[settings]")
{
    auto again = DeleteUserSettingsFiles();

    SECTION("Default value")
    {
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::OutputLocale>().empty());
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Valid locale")
    {
        std::string_view json = R"({ "output": { "locale": "en-US" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::OutputLocale>() == "en-US"sv);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Case insensitive locale")
    {
        std::string_view json = R"({ "output": { "locale": "EN-us" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::OutputLocale>() == "en-US"sv);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Unsupported locale")
    {
        std::string_view json = R"({ "output": { "locale": "el-GR" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::OutputLocale>().empty());
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Invalid locale")
    {
        std::string_view json = R"({ "output": { "locale": "en_US.UTF-8" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::OutputLocale>().empty());
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Wrong type")
    {
        std::string_view json = R"({ "output": { "locale": ["en-US"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::OutputLocale>().empty());
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
}

TEST_CASE("ConvertToSortField", "[settings]")
{
    SECTION("Valid values - lowercase")
    {
        REQUIRE(ConvertToSortField("relevance") == SortField::Relevance);
        REQUIRE(ConvertToSortField("name") == SortField::Name);
        REQUIRE(ConvertToSortField("id") == SortField::Id);
        REQUIRE(ConvertToSortField("version") == SortField::Version);
        REQUIRE(ConvertToSortField("source") == SortField::Source);
        REQUIRE(ConvertToSortField("available") == SortField::Available);
    }
    SECTION("Case-insensitive")
    {
        REQUIRE(ConvertToSortField("RELEVANCE") == SortField::Relevance);
        REQUIRE(ConvertToSortField("NAME") == SortField::Name);
        REQUIRE(ConvertToSortField("Id") == SortField::Id);
        REQUIRE(ConvertToSortField("VERSION") == SortField::Version);
        REQUIRE(ConvertToSortField("Source") == SortField::Source);
        REQUIRE(ConvertToSortField("AVAILABLE") == SortField::Available);
    }
    SECTION("Invalid values return nullopt")
    {
        REQUIRE_FALSE(ConvertToSortField("").has_value());
        REQUIRE_FALSE(ConvertToSortField("foo").has_value());
        REQUIRE_FALSE(ConvertToSortField("names").has_value());
        REQUIRE_FALSE(ConvertToSortField("nam").has_value());
    }
    SECTION("Settings round-trip with ConvertToSortField")
    {
        auto again = DeleteUserSettingsFiles();

        std::string_view json = R"({ "output": { "sortOrder": ["name", "id"] } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        auto fields = userSettingTest.Get<Setting::OutputSortOrder>();
        REQUIRE(fields.size() == 2);
        REQUIRE(fields[0] == SortField::Name);
        REQUIRE(fields[1] == SortField::Id);
    }
}
