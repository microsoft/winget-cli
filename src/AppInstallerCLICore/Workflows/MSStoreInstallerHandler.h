// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

// MSStoreInstallerHandler handles msstore installers.
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

    // Attempt to repair the installation of an Store app that is already installed
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void MSStoreRepair(Execution::Context& context);

    // Downloads the Store app installer.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void MSStoreDownload(Execution::Context& context);

    // Ensure the Store app is not blocked by policy.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void EnsureStorePolicySatisfied(Execution::Context& context);

    // Verifies the full package is installed.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void VerifyIsFullPackage(Execution::Context& context);

    // Change stub preference to full and installs full package if needed.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void EnableExtendedFeatures(Execution::Context& context);

    // Change stub preference to stub and installs stub package if needed.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void DisableExtendedFeatures(Execution::Context& context);
}
