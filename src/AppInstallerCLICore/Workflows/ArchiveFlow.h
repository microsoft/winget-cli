// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    void ExtractInstallerFromArchive(Execution::Context& context);

    void VerifyAndSetNestedInstaller(Execution::Context& context);
}