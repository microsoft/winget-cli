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
        void DeleteUserSettingsFilesInternal()
        {
            auto settingsPath = UserSettings::SettingsFilePath();
            if (std::filesystem::exists(settingsPath))
            {
                std::filesystem::remove(settingsPath);
            }

            auto settingsBackupPath = GetPathTo(Stream::BackupUserSettings);
            if (std::filesystem::exists(settingsBackupPath))
            {
                std::filesystem::remove(settingsBackupPath);
            }
        }
    }

    void SetSetting(const AppInstaller::Settings::StreamDefinition& stream, std::string_view value)
    {
        REQUIRE(Stream{ stream }.Set(value));
    }

    void RemoveSetting(const AppInstaller::Settings::StreamDefinition& stream)
    {
        Stream{ stream }.Remove();
    }

    std::filesystem::path GetPathTo(const AppInstaller::Settings::StreamDefinition& stream)
    {
        return Stream{ stream }.GetPath();
    }

    UserSettingsFileGuard::UserSettingsFileGuard()
    {
        DeleteUserSettingsFilesInternal();
    }

    UserSettingsFileGuard::~UserSettingsFileGuard()
    {
        DeleteUserSettingsFilesInternal();
    }

    [[nodiscard]] UserSettingsFileGuard DeleteUserSettingsFiles()
    {
        return {};
    }

    GroupPolicyTestOverride::GroupPolicyTestOverride(const AppInstaller::Registry::Key& key) : GroupPolicy(key)
    {
        GroupPolicy::OverrideInstance(this);
    }

    GroupPolicyTestOverride::~GroupPolicyTestOverride()
    {
        GroupPolicy::ResetInstance();
    }

    void GroupPolicyTestOverride::SetState(TogglePolicy::Policy policy, PolicyState state)
    {
        m_toggles[policy] = state;
    }
}