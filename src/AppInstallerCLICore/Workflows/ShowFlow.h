// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Shows information on an application.
    // Required Args: None
    // Inputs: Manifest, Installer
    // Outputs: None
    void ShowManifestInfo(Execution::Context& context);

    // Shows information on a package; this is only the information common to all installers.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void ShowPackageInfo(Execution::Context& context);

    // Shows information on an installer
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void ShowInstallerInfo(Execution::Context& context);

    // Shows the version for the specific manifest.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void ShowManifestVersion(Execution::Context& context);
}