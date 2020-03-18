// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowBase.h"
#include "ManifestComparator.h"

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

    bool WorkflowBase::IndexSearch()
    {
        OpenIndexSource();
        if (!m_source)
        {
            bool noSources = true;

            if (m_argsRef.Contains(ExecutionArgs::Type::Source))
            {
                // A bad name was given, try to help.
                std::vector<SourceDetails> sources = GetSources();
                if (!sources.empty())
                {
                    noSources = false;

                    m_reporterRef.ShowMsg("No sources match the given value '" + *m_argsRef.GetArg(ExecutionArgs::Type::Source) + "'",
                        ExecutionReporter::Level::Warning);
                    m_reporterRef.ShowMsg("The configured sources are:");
                    for (const auto& details : sources)
                    {
                        m_reporterRef.ShowMsg("  " + details.Name);
                    }
                }
            }

            if (noSources)
            {
                m_reporterRef.ShowMsg("No sources defined; add one with 'source add'",
                    ExecutionReporter::Level::Warning);
            }

            return false;
        }

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
            std::string msg = app->GetId() + ", " + app->GetName() + ", " + allVersions.at(0).GetVersion().ToString();

            if (match.MatchCriteria.Field != ApplicationMatchField::Id && match.MatchCriteria.Field != ApplicationMatchField::Name)
            {
                msg += ", [";
                msg += ApplicationMatchFieldToString(match.MatchCriteria.Field);
                msg += ": " + match.MatchCriteria.Value + "]";
            }

            Logging::Telemetry().LogSearchResultCount(m_searchResult.Matches.size());
            m_reporterRef.ShowMsg(msg);
        }
    }

    bool SingleManifestWorkflow::EnsureOneMatchFromSearchResult()
    {
        if (m_searchResult.Matches.size() == 0)
        {
            Logging::Telemetry().LogNoAppMatch();
            m_reporterRef.ShowMsg("No app found matching input criteria.");
            return false;
        }

        if (m_searchResult.Matches.size() > 1)
        {
            Logging::Telemetry().LogMultiAppMatch();
            m_reporterRef.ShowMsg("Multiple apps found matching input criteria. Please refine the input.");
            ReportSearchResult();
            return false;
        }

        return true;
    }

    bool SingleManifestWorkflow::GetManifest()
    {
        auto app = m_searchResult.Matches.at(0).Application.get();

        Logging::Telemetry().LogManifestFields(app->GetName(), app->GetId());
        m_reporterRef.ShowMsg("Found app: " + app->GetName());

        std::string_view version = (m_argsRef.Contains(ExecutionArgs::Type::Version) ? *m_argsRef.GetArg(ExecutionArgs::Type::Version) : std::string_view{});
        std::string_view channel = (m_argsRef.Contains(ExecutionArgs::Type::Channel) ? *m_argsRef.GetArg(ExecutionArgs::Type::Channel) : std::string_view{});

        std::optional<Manifest::Manifest> manifest = app->GetManifest(version, channel);

        if (!manifest)
        {
            std::string message = "No version found matching ";
            if (!version.empty())
            {
                message += version;
            }
            if (!channel.empty())
            {
                message += '[';
                message += channel;
                message += ']';
            }

            m_reporterRef.ShowMsg(message, ExecutionReporter::Level::Warning);
            return false;
        }

        m_manifest = std::move(manifest.value());

        return true;
    }

    void SingleManifestWorkflow::SelectInstaller()
    {
        ManifestComparator manifestComparator(m_manifest, m_reporterRef);
        m_selectedInstaller = manifestComparator.GetPreferredInstaller(m_argsRef);
    }
}