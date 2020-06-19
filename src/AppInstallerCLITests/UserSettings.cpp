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
    static constexpr std::string_view goodJson = "{}";
    static constexpr std::string_view badJson = "{";
    static constexpr std::string_view settings = "settings.json"sv;
    static constexpr std::string_view settingsBackup = "settings.json.backup"sv;

    std::filesystem::path GetBackupPath()
    {
        return GetPathTo(PathName::UserFileSettings) / settingsBackup;
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
        SetSetting(Type::UserFile, settingsBackup, badJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Default);
    }
    SECTION("No setting.json Good setting.json.backup")
    {
        SetSetting(Type::UserFile, settingsBackup, goodJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Backup);
    }
    SECTION("Bad setting.json No setting.json.backup")
    {
        SetSetting(Type::UserFile, settings, badJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Default);
    }
    SECTION("Bad setting.json Bad setting.json.backup")
    {
        SetSetting(Type::UserFile, settings, badJson);
        SetSetting(Type::UserFile, settingsBackup, badJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Default);
    }
    SECTION("Bad setting.json Good setting.json.backup")
    {
        SetSetting(Type::UserFile, settings, badJson);
        SetSetting(Type::UserFile, settingsBackup, goodJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Backup);
    }
    SECTION("Good setting.json No setting.json.backup")
    {
        SetSetting(Type::UserFile, settings, goodJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Standard);
    }
    SECTION("Good setting.json Bad setting.json.backup")
    {
        SetSetting(Type::UserFile, settings, goodJson);
        SetSetting(Type::UserFile, settingsBackup, badJson);

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Standard);
    }
    SECTION("Good setting.json Good setting.json.backup")
    {
        SetSetting(Type::UserFile, settings, goodJson);
        SetSetting(Type::UserFile, settingsBackup, goodJson);

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
        SetSetting(Type::UserFile, settings, goodJson);
        REQUIRE(std::filesystem::exists(settingsPath));
        REQUIRE(!std::filesystem::exists(settingsBackupPath));

        UserSettingsTest userSettingTest;
        REQUIRE(userSettingTest.GetType() == UserSettingsType::Standard);
        userSettingTest.PrepareToShellExecuteFile();

        REQUIRE(std::filesystem::exists(settingsPath));
        REQUIRE(std::filesystem::exists(settingsBackupPath));
    }
}
