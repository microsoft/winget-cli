// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "Invocation.h"
#include "WorkflowBase.h"

namespace AppInstaller::Workflow
{
    class SearchFlow : public WorkflowBase
    {
    public:
        SearchFlow(const AppInstaller::CLI::Invocation& args, std::ostream& outStream, std::istream& inStream) :
            WorkflowBase(args, outStream, inStream) {}

        void Execute();;

    protected:

        void ProcessSearchResult();
    };
}