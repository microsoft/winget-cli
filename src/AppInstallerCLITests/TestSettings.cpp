// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include <winget/Settings.h>

using namespace AppInstaller::Settings;

namespace TestCommon
{
    namespace
    {
        // TODO: Use final path
        const static HKEY s_PolicyKeyRoot = HKEY_CURRENT_USER;
        const static std::wstring s_PolicyKeyName = L"test\\policy";
    }

    void ResetGroupPolicy()
    {
        AppInstaller::Settings::GroupPolicy::ResetInstance();
        RegDeleteKey(s_PolicyKeyRoot, s_PolicyKeyName.c_str());
    }

    void SetGroupPolicy(const std::wstring& name, bool value)
    {
        auto policiesKey = RegCreateVolatileSubKey(s_PolicyKeyRoot, s_PolicyKeyName);
        SetRegistryValue(policiesKey.get(), name, value ? 1 : 0);
    }

    void SetGroupPolicy(const std::wstring& name, DWORD value)
    {
        auto policiesKey = RegCreateVolatileSubKey(s_PolicyKeyRoot, s_PolicyKeyName);
        SetRegistryValue(policiesKey.get(), name, value);
    }

    void SetGroupPolicy(const std::wstring& name, const std::wstring& value)
    {
        auto policiesKey = RegCreateVolatileSubKey(s_PolicyKeyRoot, s_PolicyKeyName);
        SetRegistryValue(policiesKey.get(), name, value);
    }

    void DeleteUserSettingsFiles()
    {
        auto settingsPath = UserSettings::SettingsFilePath();
        if (std::filesystem::exists(settingsPath))
        {
            std::filesystem::remove(settingsPath);
        }

        auto settingsBackupPath = GetPathTo(Streams::BackupUserSettings);
        if (std::filesystem::exists(settingsBackupPath))
        {
            std::filesystem::remove(settingsBackupPath);
        }
    }
}