// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/UserSettings.h>
#include <wil/resource.h>
#include <string>

namespace TestCommon
{
    // Repeat the policy values here so we can catch unintended changes in the source.
    const std::wstring WinGetPolicyValueName = L"EnableWindowsPackageManager";
    const std::wstring WinGetSettingsPolicyValueName = L"EnableWindowsPackageManagerSettings";
    const std::wstring ExperimentalFeaturesPolicyValueName = L"EnableExperimentalFeatures";
    const std::wstring LocalManifestsPolicyValueName = L"EnableLocalManifestFiles";
    const std::wstring EnableHashOverridePolicyValueName = L"EnableHashOverride";
    const std::wstring DefaultSourcePolicyValueName = L"EnableDefaultSource";
    const std::wstring MSStoreSourcePolicyValueName = L"EnableMSStoreSource";
    const std::wstring AdditionalSourcesPolicyValueName = L"EnableAdditionalSources";
    const std::wstring AllowedSourcesPolicyValueName = L"EnableAllowedSources";

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