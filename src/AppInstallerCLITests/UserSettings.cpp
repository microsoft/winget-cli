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
    void DeleteUserSettingsFiles()
    {
        auto settingsPath = UserSettings::Instance().SettingsFilePath();
        if (std::filesystem::exists(settingsPath))
        {
            std::filesystem::remove(settingsPath);
        }

        auto settingsBackupPath = UserSettings::Instance().SettingsBackupFilePath();
        if (std::filesystem::exists(settingsBackupPath))
        {
            std::filesystem::remove(settingsBackupPath);
        }
    }
}

TEST_CASE("UserSettingsFilePaths", "[settings]")
{
    auto settingsPath = UserSettings::Instance().SettingsFilePath();
    auto expectedPath = GetPathTo(PathName::UserFileSettings) / "settings.json" ;
    REQUIRE(settingsPath == expectedPath);

    auto settingsBackupPath = UserSettings::Instance().SettingsBackupFilePath();
    auto expectedBackupPath = GetPathTo(PathName::UserFileSettings) / "settings.json.backup";
    REQUIRE(expectedBackupPath == expectedBackupPath);
}

TEST_CASE("UserSettingsType", "[settings]")
{
    static constexpr std::string_view goodJson = "{}";
    static constexpr std::string_view badJson = "{";
    static constexpr std::string_view settings = "settings.json"sv;
    static constexpr std::string_view settingsBackup = "settings.json.backup"sv;

    // These are all the possible combinations between (7 of them are impossible):
    // 1 - No settings.json file exists
    // 2 - Bad settings.json file
    // 3 - No settings.json.backup file exists
    // 4 - Bad settings.json.backup file exists.
    DeleteUserSettingsFiles();

    SECTION("No setting.json No setting.json.backup")
    {
        UserSettings::Instance().Reload();
        REQUIRE(UserSettings::Instance().GetType() == UserSettingsType::Default);
    }
    SECTION("No setting.json Bad setting.json.backup")
    {
        SetSetting(Type::UserFile, settingsBackup, badJson);

        UserSettings::Instance().Reload();
        REQUIRE(UserSettings::Instance().GetType() == UserSettingsType::Default);
    }
    SECTION("No setting.json Good setting.json.backup")
    {
        SetSetting(Type::UserFile, settingsBackup, goodJson);

        UserSettings::Instance().Reload();
        REQUIRE(UserSettings::Instance().GetType() == UserSettingsType::Backup);
    }
    SECTION("Bad setting.json No setting.json.backup")
    {
        SetSetting(Type::UserFile, settings, badJson);

        UserSettings::Instance().Reload();
        REQUIRE(UserSettings::Instance().GetType() == UserSettingsType::Default);
    }
    SECTION("Bad setting.json Bad setting.json.backup")
    {
        SetSetting(Type::UserFile, settings, badJson);
        SetSetting(Type::UserFile, settingsBackup, badJson);

        UserSettings::Instance().Reload();
        REQUIRE(UserSettings::Instance().GetType() == UserSettingsType::Default);
    }
    SECTION("Bad setting.json Good setting.json.backup")
    {
        SetSetting(Type::UserFile, settings, badJson);
        SetSetting(Type::UserFile, settingsBackup, goodJson);
        UserSettings::Instance().Reload();

        REQUIRE(UserSettings::Instance().GetType() == UserSettingsType::Backup);
    }
    SECTION("Good setting.json No setting.json.backup")
    {
        SetSetting(Type::UserFile, settings, goodJson);
        UserSettings::Instance().Reload();

        REQUIRE(UserSettings::Instance().GetType() == UserSettingsType::Standard);
    }
    SECTION("Good setting.json Bad setting.json.backup")
    {
        SetSetting(Type::UserFile, settings, goodJson);
        SetSetting(Type::UserFile, settingsBackup, badJson);
        UserSettings::Instance().Reload();

        REQUIRE(UserSettings::Instance().GetType() == UserSettingsType::Standard);
    }
    SECTION("Good setting.json Good setting.json.backup")
    {
        SetSetting(Type::UserFile, settings, goodJson);
        SetSetting(Type::UserFile, settingsBackup, goodJson);
        UserSettings::Instance().Reload();

        REQUIRE(UserSettings::Instance().GetType() == UserSettingsType::Standard);
    }
}

TEST_CASE("UserSettingsCreateFiles", "[settings]")
{
    DeleteUserSettingsFiles();

    auto settingsPath = UserSettings::Instance().SettingsFilePath();
    auto settingsBackupPath = UserSettings::Instance().SettingsBackupFilePath();

    REQUIRE(!std::filesystem::exists(settingsPath));
    REQUIRE(!std::filesystem::exists(settingsBackupPath));

    REQUIRE_THROWS_HR(UserSettings::Instance().CreateBackup(), HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    REQUIRE(!std::filesystem::exists(settingsBackupPath));

    UserSettings::Instance().CreateFileIfNeeded();
    REQUIRE(std::filesystem::exists(settingsPath));

    UserSettings::Instance().CreateBackup();
    REQUIRE(std::filesystem::exists(settingsBackupPath));
}
