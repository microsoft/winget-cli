// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Uninstalls a single package. This also does the reporting, user interaction, and recording
    // for single-package uninstallation.
    // RequiredArgs: None
    // Inputs: InstalledPackageVersion
    // Outputs: None
    void UninstallSinglePackage(Execution::Context& context);

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

    // Removes the MSIX.
    // Required Args: None
    // Inputs: PackageFamilyNames
    // Outputs: None
    void MsixUninstall(Execution::Context& context);

    // Records the uninstall to the tracking catalog.
    // Required Args: None
    // Inputs: Package
    // Outputs: None
    void RecordUninstall(Execution::Context& context);
}
