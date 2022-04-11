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
    std::optional<HRESULT> PortableCopyExeInstall(Execution::Context& context, IProgressCallback& progress);

    void PortableRegistryInstall(Execution::Context& context);

    void CreatePortableSymlink(Execution::Context& context);

    std::filesystem::path GetPortableTargetFullPath(Execution::Context& context);
}