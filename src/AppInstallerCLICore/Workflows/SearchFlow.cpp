// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "SearchFlow.h"

using namespace AppInstaller::Repository;

namespace AppInstaller::Workflow
{
    void SearchFlow::Execute()
    {
        WorkflowBase::IndexSearch();

        ProcessSearchResult();
    }

    void SearchFlow::ProcessSearchResult()
    {
        if (m_searchResult.Matches.size() == 0)
        {
            AICLI_LOG(CLI, Info, << "No app found matching input criteria");
            m_reporter.ShowMsg(WorkflowReporter::Level::Info, "No app found matching input criteria.");
        }
        else
        {
            WorkflowBase::ReportSearchResult();
        }
    }
}