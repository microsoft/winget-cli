// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // 
    // Required Args: 
    // Inputs: 
    // Outputs: 
    void Export(Execution::Context& context);

    // Required Args: ImportFile
    // Outputs: PackageRequests
    void ReadImportFile(Execution::Context& context);

    // Inputs: PackageRequests
    // Outputs: PackagesToInstall
    void SearchPackagesForImport(Execution::Context& context);
}
