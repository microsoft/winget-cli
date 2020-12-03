// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Gets the command string or package family names used to uninstall the package.
    // Required Args: None
    // Inputs: InstalledPackageVersion
    // Output: UninstallString?, PackageFamilyNames?
    void GetUninstallInfo(Execution::Context& context);

    // Uninstalls the package according to its type.
    // Required Args: None
    // Inputs: InstalledPackageVersion, UninstallString?, PackageFamilyNames?
    // Output: None
    void ExecuteUninstaller(Execution::Context& context);

    // Runs the uninstaller command via CreateProcess.
    // Required Args: None
    // Inputs: UninstallString
    // Outputs: None
    void ExecuteUninstallString(Execution::Context& context);

    // Removes the MSI
    // Required Args: None
    // Inputs: ProductCodes
    // Output: None
    void MsiUninstall(Execution::Context& context);

    // Removes the MSIX.
    // Required Args: None
    // Inputs: PackageFamilyNames
    // Outputs: None
    void MsixUninstall(Execution::Context& context);
}