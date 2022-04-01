// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
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
}

TEST_CASE("UserSettingsFilePaths", "[settings]")
{
    auto settingsPath = UserSettings::SettingsFilePath();
    auto expectedPath = GetPathTo(PathName::UserFileSettings) / "settings.json";
    REQUIRE(settingsPath == expectedPath);
}

TEST_CASE("UserSettingsType", "[settings]")
{
    // These are all the possible combinations between (7 of them are impossible):
    // 1 - No settings.json file exists
    // 2 - Bad settings.json file
    // 3 - No settings.json.backup file exists
    // 4 - Bad settings.json.backup file exists.
    DeleteUserSettingsFiles();

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
    DeleteUserSettingsFiles();

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
    DeleteUserSettingsFiles();

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

TEST_CASE("SettingLoggingLevelPreference", "[settings]")
{
    DeleteUserSettingsFiles();

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

TEST_CASE("SettingAutoUpdateIntervalInMinutes", "[settings]")
{
    DeleteUserSettingsFiles();

    constexpr static auto cinq = 5min;
    constexpr static auto cero = 0min;
    constexpr static auto threehundred = 300min;

    SECTION("Default value")
    {
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == cinq);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Valid value")
    {
        std::string_view json = R"({ "source": { "autoUpdateIntervalInMinutes": 0 } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == cero);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Valid value 0")
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

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == cinq);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Invalid type string")
    {
        std::string_view json = R"({ "source": { "autoUpdateIntervalInMinutes": "not a number" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == cinq);
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
    DeleteUserSettingsFiles();

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

TEST_CASE("SettingsPortableAppRoot", "[settings]")
{
    SECTION("Relative path")
    {
        std::string_view json = R"({ "installBehavior": { "portableAppUserRoot": %LOCALAPPDATA%/Portable/Root } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;
        
        REQUIRE(userSettingTest.Get<Setting::PortableAppUserRoot>().empty());
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
}
