// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Verifies windows supports stub packages.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void VerifyStubSupport(Execution::Context& context);

    // Sets the stub package option to stub and installs stub package if needed.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void AppInstallerStubPreferred(Execution::Context& context);

    // Sets the stub package option to full and installs full package if needed.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void AppInstallerFullPreferred(Execution::Context& context);
}
