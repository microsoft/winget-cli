// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <string>

namespace AppInstaller::Settings
{
    // Enum of admin settings.
    // When a new one is added, it should be also added to SettingsCommand::ExportAdminSettings
    enum class AdminSetting
    {
        Unknown,
        LocalManifestFiles,
        BypassCertificatePinningForMicrosoftStore,
    };

    AdminSetting StringToAdminSetting(std::string_view in);

    std::string_view AdminSettingToString(AdminSetting setting);

    bool EnableAdminSetting(AdminSetting setting);

    bool DisableAdminSetting(AdminSetting setting);

    bool IsAdminSettingEnabled(AdminSetting setting);
}
