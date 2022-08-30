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

    void EnsureSupportForPortableInstall(Execution::Context& context);

    void EnsureSupportForPortableUninstall(Execution::Context& context);

    void EnsureNonPortableTypeForArchiveInstall(Execution::Context& context);

    // Gets the portable install info from the context.
    // Required Args: None
    // Inputs: Manifest?, Installer, InstallerPath
    // Outputs: InstallerArgs
    void GetPortableInstallInfo(Execution::Context& context);

    void VerifyPortableRegistryMatch(Execution::Context& context);

    std::filesystem::path GetPortableTargetDirectory(Execution::Context& context);
}