// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/UserSettings.h>
#include <wil/resource.h>
#include <string>

namespace TestCommon
{
    void DeleteUserSettingsFiles();

    struct UserSettingsTest : AppInstaller::Settings::UserSettings
    {
    };

    void ResetGroupPolicy();
    void SetGroupPolicy(const std::wstring& name, bool value);
    void SetGroupPolicy(const std::wstring& name, DWORD value);
    void SetGroupPolicy(const std::wstring& name, const std::wstring& value);

    inline auto PrepareGroupPolicyForTest()
    {
        ResetGroupPolicy();
        return wil::scope_exit([]() { ResetGroupPolicy(); });
    }

}