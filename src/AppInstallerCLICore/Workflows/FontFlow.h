// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Reports the installed font families as a table.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void ReportInstalledFontFamiliesResult(Execution::Context& context);
}
