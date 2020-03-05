// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionReporter.h"
#include "ExecutionArgs.h"

namespace AppInstaller::CLI
{
    struct ExecutionContext
    {
        ExecutionReporter Reporter;
        ExecutionArgs Args;

        ExecutionContext(std::ostream& out, std::istream& in) : Reporter(out, in) {}
    };
}