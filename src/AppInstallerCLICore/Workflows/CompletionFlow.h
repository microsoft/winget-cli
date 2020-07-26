// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Outputs completion possibilities for the source name argument.
    // Required Args: None
    // Inputs: CompletionData
    // Outputs: None
    void CompleteSourceName(Execution::Context& context);
}
