// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "ExecutionContext.h"
#include "WorkflowBase.h"

namespace AppInstaller::CLI::Workflow
{
    class ShowFlow : public SingleManifestWorkflow
    {
    public:
        ShowFlow(AppInstaller::CLI::Execution::Context& context) : SingleManifestWorkflow(context) {}

        void Execute();

    protected:

        void ShowAppInfo();
        void ShowAppVersion();
    };
}