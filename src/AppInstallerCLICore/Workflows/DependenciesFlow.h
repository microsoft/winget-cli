// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Shows information about dependencies.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void ReportDependencies(Execution::Context& context);
}