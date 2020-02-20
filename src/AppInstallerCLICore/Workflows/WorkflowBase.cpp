// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Commands/Common.h"
#include "WorkflowBase.h"
#include "Public/AppInstallerRepositorySearch.h"
#include "Public/AppInstallerRepositorySource.h"

using namespace AppInstaller::Repository;

namespace AppInstaller::Workflow
{
    void WorkflowBase::OpenIndexSource()
    {
        if (m_argsRef.Contains(CLI::ARG_SOURCE))
        {
            AICLI_LOG(Repo, Verbose, << "Open source: " << *m_argsRef.GetArg(CLI::ARG_SOURCE));
            m_source = OpenSource(*m_argsRef.GetArg(CLI::ARG_SOURCE));
        }
        else
        {
            AICLI_LOG(Repo, Verbose, << "Open source: all");
            m_source = OpenSource("");
        }
    }

    SearchResult WorkflowBase::IndexSearch()
    {
        OpenIndexSource();

        // Construct query
        MatchType matchType = MatchType::Fuzzy;
        if (m_argsRef.Contains(CLI::ARG_EXACT))
        {
            matchType = MatchType::Exact;
        }

        SearchRequest searchRequest;
        if (m_argsRef.Contains(CLI::ARG_QUERY))
        {
            AICLI_LOG(Repo, Verbose, << "SearchRequest query: " << *m_argsRef.GetArg(CLI::ARG_QUERY));
            searchRequest.Query.emplace(RequestMatch(matchType, *m_argsRef.GetArg(CLI::ARG_QUERY)));
        }

        if (m_argsRef.Contains(CLI::ARG_ID))
        {
            AICLI_LOG(Repo, Verbose, << "SearchRequest Id: " << *m_argsRef.GetArg(CLI::ARG_ID));
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Id, matchType, *m_argsRef.GetArg(CLI::ARG_ID)));
        }

        if (m_argsRef.Contains(CLI::ARG_NAME))
        {
            AICLI_LOG(Repo, Verbose, << "SearchRequest Name: " << *m_argsRef.GetArg(CLI::ARG_NAME));
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Name, matchType, *m_argsRef.GetArg(CLI::ARG_NAME)));
        }

        if (m_argsRef.Contains(CLI::ARG_MONIKER))
        {
            AICLI_LOG(Repo, Verbose, << "SearchRequest Moniker: " << *m_argsRef.GetArg(CLI::ARG_MONIKER));
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Moniker, matchType, *m_argsRef.GetArg(CLI::ARG_MONIKER)));
        }

        if (m_argsRef.Contains(CLI::ARG_TAG))
        {
            AICLI_LOG(Repo, Verbose, << "SearchRequest Tag: " << *m_argsRef.GetArg(CLI::ARG_TAG));
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Tag, matchType, *m_argsRef.GetArg(CLI::ARG_TAG)));
        }

        if (m_argsRef.Contains(CLI::ARG_COMMAND))
        {
            AICLI_LOG(Repo, Verbose, << "SearchRequest Command: " << *m_argsRef.GetArg(CLI::ARG_COMMAND));
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Command, matchType, *m_argsRef.GetArg(CLI::ARG_COMMAND)));
        }

        if (m_argsRef.Contains(CLI::ARG_COUNT))
        {
            AICLI_LOG(Repo, Verbose, << "SearchRequest Count: " << *m_argsRef.GetArg(CLI::ARG_COUNT));
            searchRequest.MaximumResults = std::stoi(*m_argsRef.GetArg(CLI::ARG_COUNT));
        }

        return m_source->Search(searchRequest);
    }
}