// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Ensures that the current OS version is greater than or equal to the one in the manifest.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void EnsureMinOSVersion(Execution::Context& context);

    // Ensures that there is an applicable installer.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void EnsureApplicableInstaller(Execution::Context& context);

    // Gets the source list, filtering it if SourceName is present.
    // Required Args: None
    // Inputs: Installer
    // Outputs: InstallerHandler
    void GetInstallerHandler(Execution::Context& context);

    // Downloads any files necessary via the InstallerHandler.
    // Required Args: None
    // Inputs: InstallerHandler
    // Outputs: None
    void DownloadInstaller(Execution::Context& context);

    // Executes the installer via the InstallerHandler.
    // Required Args: None
    // Inputs: InstallerHandler
    // Outputs: None
    void ExecuteInstaller(Execution::Context& context);
}
