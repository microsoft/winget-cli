// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/UserSettings.h>
#include <wil/resource.h>
#include <string>

namespace TestCommon
{
    const std::wstring WinGetPolicyValueName = L"EnableWindowsPackageManager";
    const std::wstring SettingsCommandPolicyValueName = L"EnableSettingsCommand";
    const std::wstring ExperimentalFeaturesPolicyValueName = L"EnableExperimentalFeatures";
    const std::wstring LocalManifestsPolicyValueName = L"EnableLocalManifestFiles";

    const std::wstring SourceUpdateIntervalPolicyValueName = L"SourceAutoUpdateIntervalInMinutes";

    void DeleteUserSettingsFiles();

    struct UserSettingsTest : AppInstaller::Settings::UserSettings
    {
    };

    struct GroupPolicyTestOverride : AppInstaller::Settings::GroupPolicy
    {
        GroupPolicyTestOverride(const AppInstaller::Registry::Key& key);
        ~GroupPolicyTestOverride();
    };
}