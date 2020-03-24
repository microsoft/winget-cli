// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionReporter.h"
#include "ExecutionArgs.h"

namespace AppInstaller::CLI::Execution
{
    // The context within which all commands execute.
    // Contains inout/output via Execution::Reporter and
    // arguments via Execution::Args.
    struct Context
    {
        // The path for console input/output for all functionality.
        Reporter Reporter;

        // The arguments given to execute with.
        Args Args;

        Context(std::ostream& out, std::istream& in) : Reporter(out, in) {}
    };
}