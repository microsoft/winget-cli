// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "ExecutionContext.h"
#include "WorkflowBase.h"

namespace AppInstaller::Workflow
{
    class ShowFlow : public WorkflowBase
    {
    public:
        ShowFlow(AppInstaller::CLI::ExecutionContext& context) : WorkflowBase(context) {}

        void Execute();;

    protected:

        void ShowAppInfo();
        void ShowAppVersion();
    };
}