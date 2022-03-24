// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "ExecutionContext.h"
#include <filesystem>

// PortableInstallHelper provides functionality specific to installing portable executables.
namespace AppInstaller::CLI::Workflow
{
    void PortableInstallImpl(Execution::Context& context);
}