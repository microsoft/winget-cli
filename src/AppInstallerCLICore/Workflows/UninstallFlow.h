// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Required Args: None
    // Inputs: InstalledPackageVersion
    // Output: UninstallString?, PackageFamilyNames?
    void GetUninstallInfo(Execution::Context& context);

    // Required Args: None
    // Inputs: InstalledPackageVersion, UninstallString?, PackageFamilyNames?
    // Output: None
    void ExecuteUninstaller(Execution::Context& context);

    // Runs the uninstaller via CreateProcess.
    // Required Args: None
    // Inputs: UninstallString
    // Outputs: None
    void ExecuteUninstallString(Execution::Context& context);

    // Removes the MSIX.
    // Required Args: None
    // Inputs: PackageFamilyNames
    // Outputs: None
    void MsixUninstall(Execution::Context& context);
}