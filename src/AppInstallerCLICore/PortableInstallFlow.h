// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Installs a portable exe by copying the file, creating a symlink and writing to registry.
    // Required Args: None
    // Inputs: Manifest, InstallerPath, AppsAndFeaturesEntry
    // Outputs: OperationReturnCode
    void PortableInstallImpl(Execution::Context& context);
}