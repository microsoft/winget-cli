// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Reports the installed fonts as a table.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void ReportInstalledFonts(Execution::Context& context);
}
