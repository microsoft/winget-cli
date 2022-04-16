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
    void PortableInstall(Execution::Context& context);
}