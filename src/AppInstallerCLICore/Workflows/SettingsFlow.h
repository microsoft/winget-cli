// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
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

    // Opens the user settings.
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
