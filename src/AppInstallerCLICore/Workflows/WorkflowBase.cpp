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
        if (m_argsRef.Contains(Execution::Args::Type::Source))
        {
            sourceName = m_argsRef.GetArg(Execution::Args::Type::Source);
        }

        m_contextRef.Add<Execution::Data::Source>(m_reporterRef.ExecuteWithProgress(std::bind(OpenSource, sourceName, std::placeholders::_1)));
    }

    bool WorkflowBase::IndexSearch()
    {
        OpenIndexSource();
        auto& source = m_contextRef.Get<Execution::Data::Source>();
        if (!source)
        {
            bool noSources = true;

            if (m_argsRef.Contains(Execution::Args::Type::Source))
            {
                // A bad name was given, try to help.
                std::vector<SourceDetails> sources = GetSources();
                if (!sources.empty())
                {
                    noSources = false;

                    m_reporterRef.Warn() << "No sources match the given value '" << m_argsRef.GetArg(Execution::Args::Type::Source) << "'" << std::endl;
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
                    Execution::Reporter::Level::Warning);
            }

            return false;
        }

        // Construct query
        MatchType matchType = MatchType::Substring;
        if (m_argsRef.Contains(Execution::Args::Type::Exact))
        {
            matchType = MatchType::Exact;
        }

        SearchRequest searchRequest;
        if (m_argsRef.Contains(Execution::Args::Type::Query))
        {
            searchRequest.Query.emplace(RequestMatch(matchType, m_argsRef.GetArg(Execution::Args::Type::Query)));
        }

        if (m_argsRef.Contains(Execution::Args::Type::Id))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Id, matchType, m_argsRef.GetArg(Execution::Args::Type::Id)));
        }

        if (m_argsRef.Contains(Execution::Args::Type::Name))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Name, matchType, m_argsRef.GetArg(Execution::Args::Type::Name)));
        }

        if (m_argsRef.Contains(Execution::Args::Type::Moniker))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Moniker, matchType, m_argsRef.GetArg(Execution::Args::Type::Moniker)));
        }

        if (m_argsRef.Contains(Execution::Args::Type::Tag))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Tag, matchType, m_argsRef.GetArg(Execution::Args::Type::Tag)));
        }

        if (m_argsRef.Contains(Execution::Args::Type::Command))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Command, matchType, m_argsRef.GetArg(Execution::Args::Type::Command)));
        }

        if (m_argsRef.Contains(Execution::Args::Type::Count))
        {
            searchRequest.MaximumResults = std::stoi(std::string(m_argsRef.GetArg(Execution::Args::Type::Count)));
        }

        Logging::Telemetry().LogSearchRequest(
            m_argsRef.GetArg(Execution::Args::Type::Query),
            m_argsRef.GetArg(Execution::Args::Type::Id),
            m_argsRef.GetArg(Execution::Args::Type::Name),
            m_argsRef.GetArg(Execution::Args::Type::Moniker),
            m_argsRef.GetArg(Execution::Args::Type::Tag),
            m_argsRef.GetArg(Execution::Args::Type::Command),
            searchRequest.MaximumResults,
            searchRequest.ToString());
        m_contextRef.Add<Execution::Data::SearchResult>(source->Search(searchRequest));

        return true;
    }

    void WorkflowBase::ReportSearchResult()
    {
        auto& searchResult = m_contextRef.Get<Execution::Data::SearchResult>();
        for (auto& match : searchResult.Matches)
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

            Logging::Telemetry().LogSearchResultCount(searchResult.Matches.size());
            m_reporterRef.ShowMsg(msg);
        }
    }

    bool SingleManifestWorkflow::EnsureOneMatchFromSearchResult()
    {
        auto& searchResult = m_contextRef.Get<Execution::Data::SearchResult>();
        if (searchResult.Matches.size() == 0)
        {
            Logging::Telemetry().LogNoAppMatch();
            m_reporterRef.ShowMsg("No app found matching input criteria.");
            return false;
        }

        if (searchResult.Matches.size() > 1)
        {
            Logging::Telemetry().LogMultiAppMatch();
            m_reporterRef.ShowMsg("Multiple apps found matching input criteria. Please refine the input.");
            ReportSearchResult();
            return false;
        }

        auto app = searchResult.Matches.at(0).Application.get();
        Logging::Telemetry().LogAppFound(app->GetName(), app->GetId());
        return true;
    }

    bool SingleManifestWorkflow::GetManifest()
    {
        auto app = m_contextRef.Get<Execution::Data::SearchResult>().Matches.at(0).Application.get();

        std::string_view version = m_argsRef.GetArg(Execution::Args::Type::Version);
        std::string_view channel = m_argsRef.GetArg(Execution::Args::Type::Channel);

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

            m_reporterRef.ShowMsg(message, Execution::Reporter::Level::Warning);
            return false;
        }

        m_manifest = std::move(manifest.value());
        Logging::Telemetry().LogManifestFields(m_manifest.Id, m_manifest.Name, m_manifest.Version);

        return true;
    }

    void SingleManifestWorkflow::SelectInstaller()
    {
        ManifestComparator manifestComparator(m_manifest, m_reporterRef);
        m_selectedInstaller = manifestComparator.GetPreferredInstaller(m_argsRef);
    }
}