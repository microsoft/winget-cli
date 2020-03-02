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
            m_source = OpenSource(*m_argsRef.GetArg(CLI::ARG_SOURCE));
        }
        else
        {
            m_source = OpenSource();
        }
    }

    void WorkflowBase::IndexSearch()
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
            searchRequest.Query.emplace(RequestMatch(matchType, *m_argsRef.GetArg(CLI::ARG_QUERY)));
        }

        if (m_argsRef.Contains(CLI::ARG_ID))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Id, matchType, *m_argsRef.GetArg(CLI::ARG_ID)));
        }

        if (m_argsRef.Contains(CLI::ARG_NAME))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Name, matchType, *m_argsRef.GetArg(CLI::ARG_NAME)));
        }

        if (m_argsRef.Contains(CLI::ARG_MONIKER))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Moniker, matchType, *m_argsRef.GetArg(CLI::ARG_MONIKER)));
        }

        if (m_argsRef.Contains(CLI::ARG_TAG))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Tag, matchType, *m_argsRef.GetArg(CLI::ARG_TAG)));
        }

        if (m_argsRef.Contains(CLI::ARG_COMMAND))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Command, matchType, *m_argsRef.GetArg(CLI::ARG_COMMAND)));
        }

        if (m_argsRef.Contains(CLI::ARG_COUNT))
        {
            searchRequest.MaximumResults = std::stoi(*m_argsRef.GetArg(CLI::ARG_COUNT));
        }

        m_searchResult = m_source->Search(searchRequest);
    }

    bool WorkflowBase::EnsureOneMatchFromSearchResult()
    {
        if (m_searchResult.Matches.size() == 0)
        {
            AICLI_LOG(CLI, Info, << "No app found matching input criteria");
            m_reporter.ShowMsg(WorkflowReporter::Level::Info, "No app found matching input criteria.");
            return false;
        }

        if (m_searchResult.Matches.size() > 1)
        {
            AICLI_LOG(CLI, Info, << "Multiple apps found matching input criteria");
            m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Multiple apps found matching input criteria. Please refine the input.");
            ReportSearchResult();
            return false;
        }

        return true;
    }

    void WorkflowBase::ReportSearchResult()
    {
        for (auto& match : m_searchResult.Matches)
        {
            auto app = match.Application.get();
            auto allVersions = app->GetVersions();

            // Todo: Assume versions are sorted when returned so we'll use the first one as the latest version
            // Need to call sort if the above is not the case.
            std::string msg = app->GetId() + ", " + app->GetName() + ", " + allVersions.at(0).first;

            if (match.MatchCriteria.Field != ApplicationMatchField::Id && match.MatchCriteria.Field != ApplicationMatchField::Name)
            {
                msg += ", [";
                msg += ApplicationMatchFieldToString(match.MatchCriteria.Field);
                msg += ": " + match.MatchCriteria.Value + "]";
            }

            m_reporter.ShowMsg(WorkflowReporter::Level::Info, msg);
        }
    }
}