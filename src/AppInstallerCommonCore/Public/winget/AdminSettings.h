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
    enum class AdminSetting
    {
        Unknown = 0,
        LocalManifestFiles,
        BypassCertificatePinningForMicrosoftStore,
        InstallerHashOverride,
        LocalArchiveMalwareScanOverride,
        ProxyCommandLineOptions,
        Max,
    };

    AdminSetting StringToAdminSetting(std::string_view in);

    Utility::LocIndView AdminSettingToString(AdminSetting setting);

    bool EnableAdminSetting(AdminSetting setting);

    bool DisableAdminSetting(AdminSetting setting);

    bool IsAdminSettingEnabled(AdminSetting setting);

    std::vector<AdminSetting> GetAllAdminSettings();

#ifndef AICLI_DISABLE_TEST_HOOKS
    // Only used in tests to validate that all admin settings have a corresponding policy
    TogglePolicy::Policy GetAdminSettingPolicy(AdminSetting setting);
#endif
}
