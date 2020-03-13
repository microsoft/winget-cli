// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "ExecutionContext.h"
#include "WorkflowBase.h"

namespace AppInstaller::Workflow
{
    class SearchFlow : public WorkflowBase
    {
    public:
        SearchFlow(AppInstaller::CLI::ExecutionContext& context) : WorkflowBase(context) {}

        void Execute();;

    protected:

        void ProcessSearchResult();
    };
}