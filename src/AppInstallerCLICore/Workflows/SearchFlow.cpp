// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "SearchFlow.h"

using namespace AppInstaller::Repository;

namespace AppInstaller::Workflow
{
    void SearchFlow::Execute()
    {
        m_searchResult = WorkflowBase::IndexSearch();

        ProcessSearchResult();
    }

    void SearchFlow::ProcessSearchResult()
    {
        if (m_searchResult.Matches.size() == 0)
        {
            AICLI_LOG(Repo, Info, << "No app found matching input criteria");
            m_reporter.ShowMsg(WorkflowReporter::Level::Info, "No app found matching input criteria.");
        }
        else
        {
            m_reporter.ShowSearchResult(m_searchResult);
        }
    }
}