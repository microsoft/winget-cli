// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "Invocation.h"
#include "WorkflowBase.h"

namespace AppInstaller::Workflow
{
    class ShowFlow : public WorkflowBase
    {
    public:
        ShowFlow(const AppInstaller::CLI::Invocation& args, std::ostream& outStream, std::istream& inStream) :
            WorkflowBase(args, outStream, inStream) {}

        void Execute();;

    protected:

        void ShowAppInfo();
        void ShowAppVersion();
    };
}