// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Ensures that the arguments provided supports a resume.
    // Required Args: None
    // Inputs: ResumeGuid
    // Outputs: None
    void EnsureSupportForResume(Execution::Context& context);
}
