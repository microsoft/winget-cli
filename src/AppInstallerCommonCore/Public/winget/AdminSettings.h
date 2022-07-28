// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <string>

namespace AppInstaller::Settings
{
    // Enum of admin settings.
    enum class AdminSetting
    {
        Unknown,
        LocalManifestFiles,
        BypassCertificatePinningForMicrosoftStore,
    };

    AdminSetting StringToAdminSetting(std::string_view in);

    std::string_view AdminSettingToString(AdminSetting setting);

    void EnableAdminSetting(AdminSetting setting);

    void DisableAdminSetting(AdminSetting setting);

    bool IsAdminSettingEnabled(AdminSetting setting);
}
