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

    // Verifies that the portable install operation is supported, including elevation checks for user scope.
    // Required Args: None
    // Inputs: Installer, InstalledPackageVersion?, UninstalledPackageMetadata?
    // Outputs: None
    void EnsureSupportForPortableInstall(Execution::Context& context);

    // Initializes the portable installer after elevation checks.
    // Required Args: None
    // Inputs: Architecture, Manifest, Installer, InstalledPackageVersion?, UninstalledPackageMetadata?
    // Outputs: None
    void InitializePortableInstaller(Execution::Context& context);

    // Verifies that the package and source identifiers match the loaded ARP entry.
    // Required Args: None
    // Inputs: Manifest, PackageVersion, PortableInstaller
    // Outputs: None
    void VerifyPackageAndSourceMatch(Execution::Context& context);
}