// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Ensures that there is an applicable installer.
    // Required Args: None
    // Inputs: InstallerArgs, Installer, InstallerPath, Manifest
    // Outputs: OperationReturnCode
    void PortableInstallImpl(Execution::Context& context);
}