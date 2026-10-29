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
    // Inputs: Installer, InstalledPackageVersion?
    // Outputs: None
    void EnsureSupportForPortableInstall(Execution::Context& context);

    // Initializes the portable installer, loading installed state only for an update.
    // Required Args: None
    // Inputs: Architecture, Manifest, Installer, InstalledPackageVersion?
    // Outputs: None
    void InitializePortableInstaller(Execution::Context& context);

    // For updates, verifies that the package and source identifiers match the loaded ARP entry.
    // Required Args: None
    // Inputs: Manifest, PackageVersion, PortableInstaller
    // Outputs: None
    void VerifyPackageAndSourceMatch(Execution::Context& context);
}