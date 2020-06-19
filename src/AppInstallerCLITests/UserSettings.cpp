// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerRuntime.h>
#include <winget/Settings.h>
#include <winget/UserSettings.h>

#include <AppInstallerErrors.h>

#include <filesystem>
#include <string>

using namespace AppInstaller::Settings;
using namespace AppInstaller::Runtime;
using namespace std::string_view_literals;

namespace
{
    static constexpr std::string_view s_goodJson = "{}";
    static constexpr std::string_view s_badJson = "{";
    static constexpr std::string_view s_settings = "settings.json"sv;
    static constexpr std::string_view s_settingsBackup = "settings.json.backup"sv;

    std::filesystem::path GetBackupPath()
    {
        return GetPathTo(PathName::UserFileSettings) / s_settingsBackup;
    }

    void DeleteUserSettingsFiles()
    {
        auto settingsPath = UserSettings::SettingsFilePath();
        if (std::filesystem::exists(settingsPath))
        {
            std::filesystem::remove(settingsPath);
        }

        auto settingsBackupPath = GetBackupPath();
        if (std::filesystem::exists(settingsBackupPath))
        {
            std::filesystem::remove(settingsBackupPath);
        }
    }

    struct UserSettingsTest : UserSettings
    {
    };
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
        SetSetting(Type::UserFile, s_settingsBackup, s_badJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Default);
    }
    SECTION("No setting.json Good setting.json.backup")
    {
        SetSetting(Type::UserFile, s_settingsBackup, s_goodJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Backup);
    }
    SECTION("Bad setting.json No setting.json.backup")
    {
        SetSetting(Type::UserFile, s_settings, s_badJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Default);
    }
    SECTION("Bad setting.json Bad setting.json.backup")
    {
        SetSetting(Type::UserFile, s_settings, s_badJson);
        SetSetting(Type::UserFile, s_settingsBackup, s_badJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Default);
    }
    SECTION("Bad setting.json Good setting.json.backup")
    {
        SetSetting(Type::UserFile, s_settings, s_badJson);
        SetSetting(Type::UserFile, s_settingsBackup, s_goodJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Backup);
    }
    SECTION("Good setting.json No setting.json.backup")
    {
        SetSetting(Type::UserFile, s_settings, s_goodJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Standard);
    }
    SECTION("Good setting.json Bad setting.json.backup")
    {
        SetSetting(Type::UserFile, s_settings, s_goodJson);
        SetSetting(Type::UserFile, s_settingsBackup, s_badJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Standard);
    }
    SECTION("Good setting.json Good setting.json.backup")
    {
        SetSetting(Type::UserFile, s_settings, s_goodJson);
        SetSetting(Type::UserFile, s_settingsBackup, s_goodJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Standard);
    }
}

TEST_CASE("UserSettingsCreateFiles", "[settings]")
{
    DeleteUserSettingsFiles();

    auto settingsPath = UserSettings::SettingsFilePath();
    auto settingsBackupPath = GetBackupPath();

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
        SetSetting(Type::UserFile, s_settings, s_goodJson);
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
        SetSetting(Type::UserFile, s_settings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Accent);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Rainbow")
    {
        std::string_view json = R"({ "visual": { "progressBar": "rainbow" } })";
        SetSetting(Type::UserFile, s_settings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Rainbow);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("retro")
    {
        std::string_view json = R"({ "visual": { "progressBar": "retro" } })";
        SetSetting(Type::UserFile, s_settings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Retro);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Bad value")
    {
        std::string_view json = R"({ "visual": { "progressBar": "fake" } })";
        SetSetting(Type::UserFile, s_settings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Accent);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Bad value type")
    {
        std::string_view json = R"({ "visual": { "progressBar": 5 } })";
        SetSetting(Type::UserFile, s_settings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::ProgressBarVisualStyle>() == VisualStyle::Accent);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
}

TEST_CASE("SettingAutoUpdateIntervalInMinutes", "[settings]")
{
    DeleteUserSettingsFiles();

    SECTION("Default value")
    {
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == 5);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Valid value")
    {
        std::string_view json = R"({ "source": { "autoUpdateIntervalInMinutes": 300 } })";
        SetSetting(Type::UserFile, s_settings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == 300);
        REQUIRE(userSettingTest.GetWarnings().size() == 0);
    }
    SECTION("Invalid type negative integer")
    {
        std::string_view json = R"({ "source": { "autoUpdateIntervalInMinutes": -20 } })";
        SetSetting(Type::UserFile, s_settings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == 5);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
    SECTION("Invalid type string")
    {
        std::string_view json = R"({ "source": { "autoUpdateIntervalInMinutes": "not a number" } })";
        SetSetting(Type::UserFile, s_settings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(userSettingTest.Get<Setting::AutoUpdateTimeInMinutes>() == 5);
        REQUIRE(userSettingTest.GetWarnings().size() == 1);
    }
}
