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
}