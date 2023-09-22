// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerProgress.h>
#include "ExecutionContext.h"

#include <filesystem>
#include <optional>

// ShellExecuteInstallerHandler handles installers run through ShellExecute.
// Exe, Wix, Nullsoft, Msi and Inno should be handled by this installer handler.
namespace AppInstaller::CLI::Workflow
{
    // Install is done through invoking ShellExecute on downloaded installer.
    // Required Args: None
    // Inputs: Manifest?, InstallerPath, InstallerArgs
    // Outputs: OperationReturnCode
    void ShellExecuteInstallImpl(Execution::Context& context);

    // Uninstall is done through invoking ShellExecute on uninstall string.
    // Required Args: None
    // Inputs: UninstallString
    // Outputs: OperationReturnCode
    void ShellExecuteUninstallImpl(Execution::Context& context);

    // Removes the MSI
    // Required Args: None
    // Inputs: ProductCodes
    // Output: None
    void ShellExecuteMsiExecUninstall(Execution::Context& context);

    // Gets the installer args from the context.
    // Required Args: None
    // Inputs: Manifest?, Installer, InstallerPath
    // Outputs: InstallerArgs
    void GetInstallerArgs(Execution::Context& context);

    // Enables the Windows Features dependencies by invoking ShellExecute on the dism executable.
    // Required Args: None
    // Inputs: Dependencies
    // Output: None
    void ShellExecuteEnableWindowsFeatures(Execution::Context& context);
}