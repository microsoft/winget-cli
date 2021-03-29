// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include <winget/Settings.h>

using namespace AppInstaller::Settings;

namespace TestCommon
{
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

    GroupPolicyTestOverride::GroupPolicyTestOverride(const AppInstaller::Registry::Key& key) : GroupPolicy(key)
    {
        AppInstaller::Settings::GroupPolicy::OverrideInstance(this);
    }

    GroupPolicyTestOverride::~GroupPolicyTestOverride()
    {
        AppInstaller::Settings::GroupPolicy::ResetInstance();
    }
}