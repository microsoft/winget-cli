// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "Invocation.h"
#include "WorkflowReporter.h"
#include "Public/AppInstallerRepositorySearch.h"

namespace AppInstaller::Workflow
{
    class WorkflowBase
    {
    protected:
        WorkflowBase(const AppInstaller::CLI::Invocation& args, std::ostream& outStream, std::istream& inStream) :
            m_reporter(outStream, inStream), m_argsRef(args) {}

        WorkflowReporter m_reporter;
        const AppInstaller::CLI::Invocation& m_argsRef;

        AppInstaller::Repository::SearchResult SearchIndex();
    };
}