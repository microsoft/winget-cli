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
    // Outputs: None
    void ShellExecuteInstallImpl(Execution::Context& context);

    // Gets the installer args from the context.
    // Required Args: None
    // Inputs: Manifest?, Installer, InstallerPath
    // Outputs: InstallerArgs
    void GetInstallerArgs(Execution::Context& context);

    // This method appends appropriate extension to the downloaded installer.
    // ShellExecute uses file extension to launch the installer appropriately.
    // Required Args: None
    // Inputs: Installer, InstallerPath
    // Modifies: InstallerPath
    // Outputs: None
    void RenameDownloadedInstaller(Execution::Context& context);
}