// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "WorkflowBase.h"
#include "Public/AppInstallerRepositorySearch.h"
#include "Public/AppInstallerRepositorySource.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Repository;

namespace AppInstaller::Workflow
{
    void WorkflowBase::OpenIndexSource()
    {
        std::string sourceName;
        if (m_argsRef.Contains(ExecutionArgs::Type::Source))
        {
            sourceName = *m_argsRef.GetArg(ExecutionArgs::Type::Source);
        }

        m_source = m_reporterRef.ExecuteWithProgress(std::bind(OpenSource, sourceName, std::placeholders::_1));
    }

    void WorkflowBase::IndexSearch()
    {
        OpenIndexSource();

        // Construct query
        MatchType matchType = MatchType::Substring;
        if (m_argsRef.Contains(ExecutionArgs::Type::Exact))
        {
            matchType = MatchType::Exact;
        }

        SearchRequest searchRequest;
        if (m_argsRef.Contains(ExecutionArgs::Type::Query))
        {
            searchRequest.Query.emplace(RequestMatch(matchType, *m_argsRef.GetArg(ExecutionArgs::Type::Query)));
        }

        if (m_argsRef.Contains(ExecutionArgs::Type::Id))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Id, matchType, *m_argsRef.GetArg(ExecutionArgs::Type::Id)));
        }

        if (m_argsRef.Contains(ExecutionArgs::Type::Name))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Name, matchType, *m_argsRef.GetArg(ExecutionArgs::Type::Name)));
        }

        if (m_argsRef.Contains(ExecutionArgs::Type::Moniker))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Moniker, matchType, *m_argsRef.GetArg(ExecutionArgs::Type::Moniker)));
        }

        if (m_argsRef.Contains(ExecutionArgs::Type::Tag))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Tag, matchType, *m_argsRef.GetArg(ExecutionArgs::Type::Tag)));
        }

        if (m_argsRef.Contains(ExecutionArgs::Type::Command))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Command, matchType, *m_argsRef.GetArg(ExecutionArgs::Type::Command)));
        }

        if (m_argsRef.Contains(ExecutionArgs::Type::Count))
        {
            searchRequest.MaximumResults = std::stoi(*m_argsRef.GetArg(ExecutionArgs::Type::Count));
        }

        m_searchResult = m_source->Search(searchRequest);
    }

    bool WorkflowBase::EnsureOneMatchFromSearchResult()
    {
        if (m_searchResult.Matches.size() == 0)
        {
            AICLI_LOG(CLI, Info, << "No app found matching input criteria");
            m_reporterRef.ShowMsg("No app found matching input criteria.");
            return false;
        }

        if (m_searchResult.Matches.size() > 1)
        {
            AICLI_LOG(CLI, Info, << "Multiple apps found matching input criteria");
            m_reporterRef.ShowMsg("Multiple apps found matching input criteria. Please refine the input.");
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

            m_reporterRef.ShowMsg(msg);
        }
    }
}