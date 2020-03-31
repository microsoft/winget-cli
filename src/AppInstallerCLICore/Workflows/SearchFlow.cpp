// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "SearchFlow.h"

using namespace AppInstaller::Repository;

namespace AppInstaller::Workflow
{
    void SearchFlow::Execute()
    {
        if (WorkflowBase::IndexSearch())
        {
            ProcessSearchResult();
        }
    }

    void SearchFlow::ProcessSearchResult()
    {
        if (m_contextRef.Get<CLI::Execution::Data::SearchResult>().Matches.size() == 0)
        {
            Logging::Telemetry().LogNoAppMatch();
            m_reporterRef.ShowMsg("No app found matching input criteria.");
        }
        else
        {
            WorkflowBase::ReportSearchResult();
        }
    }
}