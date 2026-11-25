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
            for (const auto& stream : { Stream::PrimaryUserSettings, Stream::BackupUserSettings,
                Stream::PrimaryAdminSettingsFile, Stream::BackupAdminSettingsFile })
            {
                auto path = GetPathTo(stream);
                if (std::filesystem::exists(path))
                {
                    std::filesystem::remove(path);
                }
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
