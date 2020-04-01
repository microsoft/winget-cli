// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowBase.h"
#include "ManifestComparator.h"


namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::Repository;

    void OpenSource(Execution::Context& context)
    {
        std::string sourceName;
        if (context.Args.Contains(Execution::Args::Type::Source))
        {
            sourceName = context.Args.GetArg(Execution::Args::Type::Source);
        }

        std::shared_ptr<Repository::ISource> source = context.Reporter.ExecuteWithProgress(std::bind(OpenSource, sourceName, std::placeholders::_1));

        if (!source)
        {
            bool noSources = true;

            if (context.Args.Contains(Execution::Args::Type::Source))
            {
                // A bad name was given, try to help.
                std::vector<SourceDetails> sources = GetSources();
                if (!sources.empty())
                {
                    noSources = false;

                    context.Reporter.Error() << "No sources match the given value: " << sourceName << std::endl;
                    context.Reporter.Info() << "The configured sources are:" << std::endl;
                    for (const auto& details : sources)
                    {
                        context.Reporter.Info() << "  " << details.Name << std::endl;
                    }
                }
            }

            if (noSources)
            {
                context.Reporter.Error() << "No sources defined; add one with 'source add'" << std::endl;
            }

            context.Terminate();
            return;
        }

        context.Add<Execution::Data::Source>(std::move(source));
    }

    void SourceSearch(Execution::Context& context)
    {
        auto& args = context.Args;

        // Construct query
        MatchType matchType = MatchType::Substring;
        if (args.Contains(Execution::Args::Type::Exact))
        {
            matchType = MatchType::Exact;
        }

        SearchRequest searchRequest;
        if (args.Contains(Execution::Args::Type::Query))
        {
            searchRequest.Query.emplace(RequestMatch(matchType, args.GetArg(Execution::Args::Type::Query)));
        }

        if (args.Contains(Execution::Args::Type::Id))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Id, matchType, args.GetArg(Execution::Args::Type::Id)));
        }

        if (args.Contains(Execution::Args::Type::Name))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Name, matchType, args.GetArg(Execution::Args::Type::Name)));
        }

        if (args.Contains(Execution::Args::Type::Moniker))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Moniker, matchType, args.GetArg(Execution::Args::Type::Moniker)));
        }

        if (args.Contains(Execution::Args::Type::Tag))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Tag, matchType, args.GetArg(Execution::Args::Type::Tag)));
        }

        if (args.Contains(Execution::Args::Type::Command))
        {
            searchRequest.Filters.emplace_back(ApplicationMatchFilter(ApplicationMatchField::Command, matchType, args.GetArg(Execution::Args::Type::Command)));
        }

        if (args.Contains(Execution::Args::Type::Count))
        {
            searchRequest.MaximumResults = std::stoi(std::string(args.GetArg(Execution::Args::Type::Count)));
        }

        Logging::Telemetry().LogSearchRequest(
            args.GetArg(Execution::Args::Type::Query),
            args.GetArg(Execution::Args::Type::Id),
            args.GetArg(Execution::Args::Type::Name),
            args.GetArg(Execution::Args::Type::Moniker),
            args.GetArg(Execution::Args::Type::Tag),
            args.GetArg(Execution::Args::Type::Command),
            searchRequest.MaximumResults,
            searchRequest.ToString());

        context.Add<Execution::Data::SearchResult>(context.Get<Execution::Data::Source>()->Search(searchRequest));
    }

    void ReportSearchResult(Execution::Context& context)
    {
        auto& searchResult = context.Get<Execution::Data::SearchResult>();
        Logging::Telemetry().LogSearchResultCount(searchResult.Matches.size());
        for (auto& match : searchResult.Matches)
        {
            auto app = match.Application.get();
            auto allVersions = app->GetVersions();

            // Assume versions are sorted when returned so we'll use the first one as the latest version
            context.Reporter.Info() << app->GetId() << ", " << app->GetName() << ", " << allVersions.at(0).GetVersion().ToString();

            if (match.MatchCriteria.Field != ApplicationMatchField::Id && match.MatchCriteria.Field != ApplicationMatchField::Name)
            {
                context.Reporter.Info() << ", [" << ApplicationMatchFieldToString(match.MatchCriteria.Field) << ": " << match.MatchCriteria.Value << "]";
            }

            context.Reporter.Info() << std::endl;
        }
    }

    void EnsureOneMatchFromSearchResult(Execution::Context& context)
    {
        auto& searchResult = context.Get<Execution::Data::SearchResult>();
        if (searchResult.Matches.size() == 0)
        {
            Logging::Telemetry().LogNoAppMatch();
            context.Reporter.Info() << "No app found matching input criteria." << std::endl;
            context.Terminate();
            return;
        }

        if (searchResult.Matches.size() > 1)
        {
            Logging::Telemetry().LogMultiAppMatch();
            context.Reporter.Warn() << "Multiple apps found matching input criteria. Please refine the input." << std::endl;
            context << ReportSearchResult;
            context.Terminate();
            return;
        }

        auto app = searchResult.Matches.at(0).Application.get();
        Logging::Telemetry().LogAppFound(app->GetName(), app->GetId());
    }

    void GetManifestFromSearchResult(Execution::Context& context)
    {
        auto app = context.Get<Execution::Data::SearchResult>().Matches.at(0).Application.get();

        std::string_view version = context.Args.GetArg(Execution::Args::Type::Version);
        std::string_view channel = context.Args.GetArg(Execution::Args::Type::Channel);

        std::optional<Manifest::Manifest> manifest = app->GetManifest(version, channel);

        if (!manifest)
        {
            context.Reporter.Error() << "No version found matching: ";
            if (!version.empty())
            {
                context.Reporter.Error() << version;
            }
            if (!channel.empty())
            {
                context.Reporter.Error() << '[' << channel << ']';
            }

            context.Reporter.Error() << std::endl;
            context.Terminate();
            return;
        }

        Logging::Telemetry().LogManifestFields(manifest->Id, manifest->Name, manifest->Version);
        context.Add<Execution::Data::Manifest>(std::move(manifest.value()));
    }

    void GetManifestFromArg(Execution::Context& context)
    {
        std::filesystem::path manifestPath = context.Args.GetArg(Execution::Args::Type::Manifest);

        if (!std::filesystem::exists(manifestPath))
        {
            context.Reporter.Error() << "File does not exist: " << manifestPath.u8string() << std::endl;
            context.Terminate();
            return;
        }

        if (std::filesystem::is_directory(manifestPath))
        {
            context.Reporter.Error() << "Path is a directory: " << manifestPath.u8string() << std::endl;
            context.Terminate();
            return;
        }

        Manifest::Manifest manifest = Manifest::Manifest::CreateFromPath(manifestPath);
        Logging::Telemetry().LogManifestFields(manifest.Id, manifest.Name, manifest.Version);
        context.Add<Execution::Data::Manifest>(std::move(manifest));
    }

    void GetManifest(Execution::Context& context)
    {
        if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            context << GetManifestFromArg;
        }
        else
        {
            context <<
                OpenSource <<
                SourceSearch <<
                EnsureOneMatchFromSearchResult <<
                GetManifestFromSearchResult;
        }
    }

    void SelectInstaller(Execution::Context& context)
    {
        ManifestComparator manifestComparator(m_manifest, m_reporterRef);
        m_selectedInstaller = manifestComparator.GetPreferredInstaller(m_argsRef);
    }
}