// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "ExecutionContext.h"
#include "WorkflowBase.h"

namespace AppInstaller::Workflow
{
    class ShowFlow : public SingleManifestWorkflow
    {
    public:
        ShowFlow(AppInstaller::CLI::ExecutionContext& context) : SingleManifestWorkflow(context) {}

        void Execute();

    protected:

        void ShowAppInfo();
        void ShowAppVersion();
    };
}