// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Copies the portable to the appropriate install location and creates a symlink.
    // Required Args: None
    // Inputs: Manifest, InstallerPath, AppsAndFeaturesEntry
    // Outputs: OperationReturnCode
    std::optional<DWORD> InstallPortable(Execution::Context& context, IProgressCallback& progress);

    void AddPortableLinksDirToPathRegistry(Execution::Context& context);

    void AddPortableEntryToUninstallRegistry(Execution::Context& context);
}