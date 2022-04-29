// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    void GetPortableARPEntryForInstall(Execution::Context& context);

    void GetPortableARPEntryForUninstall(Execution::Context& context);

    void EnsureSupportForPortableInstall(Execution::Context& context);

    void WritePortableEntryToUninstallRegistry(Execution::Context& context);

    void MovePortableExe(Execution::Context& context);

    void RemovePortableExe(Execution::Context& context);

    void CreatePortableSymlink(Execution::Context& context);

    void RemovePortableSymlink(Execution::Context& context);
}