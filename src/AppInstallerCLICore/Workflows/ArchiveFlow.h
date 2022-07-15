// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Extracts the files from an archive
    // Required Args: None
    // Inputs: InstallerPath
    // Outputs: None
    void ExtractFilesFromArchive(Execution::Context& context);

    // Verifies that the NestedInstaller exists and sets the InstallerPath
    // Required Args: None
    // Inputs: Installer, InstallerPath
    // Outputs: None
    void VerifyAndSetNestedInstaller(Execution::Context& context);
}