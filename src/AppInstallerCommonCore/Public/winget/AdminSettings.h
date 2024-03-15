// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <string>

#ifndef AICLI_DISABLE_TEST_HOOKS
#include "winget/GroupPolicy.h"
#endif

namespace AppInstaller::Settings
{
    // Enum of admin settings.
    enum class BoolAdminSetting : size_t
    {
        Unknown = 0,
        LocalManifestFiles,
        BypassCertificatePinningForMicrosoftStore,
        InstallerHashOverride,
        LocalArchiveMalwareScanOverride,
        ProxyCommandLineOptions,
        Max,
    };

    enum class StringAdminSetting : size_t
    {
        Unknown = 0,
        DefaultProxy,
        Max,
    };

    BoolAdminSetting StringToBoolAdminSetting(std::string_view in);
    StringAdminSetting StringToStringAdminSetting(std::string_view in);

    Utility::LocIndView AdminSettingToString(BoolAdminSetting setting);
    Utility::LocIndView AdminSettingToString(StringAdminSetting setting);

    // Returns true if the value is set.
    // Group policy overriding the setting can prevent the value from being set
    bool EnableAdminSetting(BoolAdminSetting setting);
    bool DisableAdminSetting(BoolAdminSetting setting);
    bool SetAdminSetting(StringAdminSetting setting, std::string_view value);
    bool ResetAdminSetting(StringAdminSetting setting);

    bool IsAdminSettingEnabled(BoolAdminSetting setting);
    std::optional<std::string> GetAdminSetting(StringAdminSetting setting);

    std::vector<BoolAdminSetting> GetAllBoolAdminSettings();
    std::vector<StringAdminSetting> GetAllStringAdminSettings();

#ifndef AICLI_DISABLE_TEST_HOOKS
    // Only exposed for tests to validate that all admin settings have a corresponding policy
    TogglePolicy::Policy GetAdminSettingPolicy(BoolAdminSetting setting);
#endif
}
