// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Displays migration warnings, or non-migration warnings when requested.
    void ShowSettingsWarnings(Execution::Context& context, bool migrationOnly = true);

    // Creates the protected settings file without opening it or creating a backup.
    void CreateAdminSettingsFile(Execution::Context& context);

    // Enables an admin setting.
    // Required Args: AdminSettingEnable
    // Inputs: None
    // Outputs: None
    void EnableAdminSetting(Execution::Context& context);

    // Disables an admin setting.
    // Required Args: AdminSettingDisable
    // Inputs: None
    // Outputs: None
    void DisableAdminSetting(Execution::Context& context);

    // Sets the value of an admin setting.
    // Required Args: SettingName, SettingValue
    // Inputs: None
    // Outputs: None
    void SetAdminSetting(Execution::Context& context);

    // Resets an admin setting to the default.
    // Required Args: SettingName
    // Inputs: None
    // Outputs: None
    void ResetAdminSetting(Execution::Context& context);

    // Resets all admin settings to the default.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void ResetAllAdminSettings(Execution::Context& context);

    // Opens the ordinary or protected per-user settings file.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void OpenUserSetting(Execution::Context& context);

    // Lists the state of settings.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void ExportSettings(Execution::Context& context);
}
