// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Required Args: None
    // Inputs: InstalledPackageVersion
    // Output: None
    void GetUninstallInfo(Execution::Context& context);

    // Required Args: None
    // Inputs:
    // Output: None
    void ExecuteUninstaller(Execution::Context& context);

    // Runs the uninstaller via ShellExecute.
    // Required Args: None
    // Inputs: UninstallerCommand
    // Outputs: None
    void ExecuteUninstallString(Execution::Context& context);

    // Removes the MSIX.
    // Required Args: None
    // Inputs: InstalledPackageVersion
    // Outputs: None
    void MsixUninstall(Execution::Context& context);
}