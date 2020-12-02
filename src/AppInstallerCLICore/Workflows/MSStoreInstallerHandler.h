// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"


// ShellExecuteInstallerHandler handles installers run through ShellExecute.
// Exe, Wix, Nullsoft, Msi and Inno should be handled by this installer handler.
namespace AppInstaller::CLI::Workflow
{
    // Deploys the Store app.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void MSStoreInstall(Execution::Context& context);

    // Updates the Store app if applicable.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void MSStoreUpdate(Execution::Context& context);

    // Ensure the Store app is not blocked by policy.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void EnsureStorePolicySatisfied(Execution::Context& context);
}