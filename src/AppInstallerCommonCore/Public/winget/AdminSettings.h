// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <string>

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
        Max,
    };

    AdminSetting StringToAdminSetting(std::string_view in);

    Utility::LocIndView AdminSettingToString(AdminSetting setting);

    bool EnableAdminSetting(AdminSetting setting);

    bool DisableAdminSetting(AdminSetting setting);

    bool IsAdminSettingEnabled(AdminSetting setting);

    std::vector<AdminSetting> GetAllAdminSettings();
}
