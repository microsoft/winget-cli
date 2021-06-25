// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace std::string_view_literals;

    // Ensures that there is an applicable installer.
    // Required Args: None
    // Inputs: InstallerArgs, Installer, InstallerPath, Manifest
    // Outputs: None
    void DirectMSIInstallImpl(Execution::Context& context);
}
