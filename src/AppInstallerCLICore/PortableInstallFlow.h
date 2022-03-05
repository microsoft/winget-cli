// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Install for portable exes which involves copying the exe and writing to the App Paths registry.
    // Required Args: None
    // Inputs: InstallerArgs, Installer, InstallerPath, Manifest
    // Outputs: OperationReturnCode
    void PortableInstallImpl(Execution::Context& context);
}