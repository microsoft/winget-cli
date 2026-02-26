// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Installs the portable package.
    // Required Args: None
    // Inputs: Manifest, Scope, Rename, Location
    // Outputs: None
    void PortableInstallImpl(Execution::Context& context);

    // Uninstalls the portable package.
    // Required Args: None
    // Inputs: ProductCode, Scope, Architecture
    // Outputs: None
    void PortableUninstallImpl(Execution::Context& context);

    // Verifies that the portable install operation is supported.
    // Required Args: None
    // Inputs: Scope, Rename
    // Outputs: None
    void EnsureSupportForPortableInstall(Execution::Context& context);

    // Initializes the portable installer.
    // Required Args: None
    // Inputs: Scope, Architecture, Manifest, Installer
    // Outputs: None
    void InitializePortableInstaller(Execution::Context& context);

    // Verifies that the package identifier and the source identifier match the ARP entry.
    // Required Args: None
    // Inputs: Manifest, PackageVersion, PortableInstaller
    // Outputs: None
    void VerifyPackageAndSourceMatch(Execution::Context& context);
}