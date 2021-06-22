// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Gathers dependencies from manifest.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void GetDependenciesFromManifest(Execution::Context& context);
}