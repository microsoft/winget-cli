// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Shows information on an application.
    // Required Args: None
    // Inputs: Manifest, Installer
    // Outputs: None
    void ShowManifestInfo(Execution::Context& context);

    // Shows the version for the specific manifest.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void ShowManifestVersion(Execution::Context& context);

    // Shows all versions for an application.
    // Required Args: None
    // Inputs: SearchResult [only operates on first match]
    // Outputs: None
    void ShowAppVersions(Execution::Context& context);
}