// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowBase.h"
#include "ExecutionContext.h"
#include "ManifestComparator.h"
#include "TableOutput.h"
#include <winget/ManifestYamlParser.h>


namespace AppInstaller::CLI::Workflow
{
    using namespace std::string_literals;
    using namespace AppInstaller::Utility::literals;
    using namespace AppInstaller::Repository;

    namespace
    {
        std::string GetMatchCriteriaDescriptor(const ResultMatch& match)
        {
            if (match.MatchCriteria.Field != PackageMatchField::Id && match.MatchCriteria.Field != PackageMatchField::Name)
            {
                std::string result{ PackageMatchFieldToString(match.MatchCriteria.Field) };
                result += ": ";
                result += match.MatchCriteria.Value;
                return result;
            }
            else
            {
                return {};
            }
        }

        void ReportIdentity(Execution::Context& context, std::string_view name, std::string_view id)
        {
            context.Reporter.Info() << Resource::String::ReportIdentityFound << ' ' << Execution::NameEmphasis << name << " [" << Execution::IdEmphasis << id << ']' << std::endl;
        }

        void SearchSourceApplyFilters(Execution::Context& context, SearchRequest& searchRequest, MatchType matchType)
        {
            const auto& args = context.Args;

            if (args.Contains(Execution::Args::Type::Id))
            {
                searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Id, matchType, args.GetArg(Execution::Args::Type::Id)));
            }

            if (args.Contains(Execution::Args::Type::Name))
            {
                searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Name, matchType, args.GetArg(Execution::Args::Type::Name)));
            }

            if (args.Contains(Execution::Args::Type::Moniker))
            {
                searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Moniker, matchType, args.GetArg(Execution::Args::Type::Moniker)));
            }

            if (args.Contains(Execution::Args::Type::Tag))
            {
                searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Tag, matchType, args.GetArg(Execution::Args::Type::Tag)));
            }

            if (args.Contains(Execution::Args::Type::Command))
            {
                searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Command, matchType, args.GetArg(Execution::Args::Type::Command)));
            }

            if (args.Contains(Execution::Args::Type::Count))
            {
                searchRequest.MaximumResults = std::stoi(std::string(args.GetArg(Execution::Args::Type::Count)));
            }
        }
    }

    bool WorkflowTask::operator==(const WorkflowTask& other) const
    {
        if (m_isFunc && other.m_isFunc)
        {
            return m_func == other.m_func;
        }
        else if (!m_isFunc && !other.m_isFunc)
        {
            return m_name == other.m_name;
        }
        else
        {
            return false;
        }
    }

    void WorkflowTask::operator()(Execution::Context& context) const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !m_isFunc);
        m_func(context);
    }

    void OpenSource(Execution::Context& context)
    {
        std::string_view sourceName;
        if (context.Args.Contains(Execution::Args::Type::Source))
        {
            sourceName = context.Args.GetArg(Execution::Args::Type::Source);
        }

        std::shared_ptr<Repository::ISource> source;
        try
        {
            auto result = context.Reporter.ExecuteWithProgress(std::bind(Repository::OpenSource, sourceName, std::placeholders::_1), true);
            source = result.Source;

            // We'll only report the source update failure as warning and continue
            for (const auto& s : result.SourcesWithUpdateFailure)
            {
                context.Reporter.Warn() << Resource::String::SourceOpenWithFailedUpdate << ' ' << s.Name << std::endl;
            }
        }
        catch (...)
        {
            context.Reporter.Error() << Resource::String::SourceOpenFailedSuggestion << std::endl;
            throw;
        }

        if (!source)
        {
            std::vector<SourceDetails> sources = GetSources();

            if (context.Args.Contains(Execution::Args::Type::Source) && !sources.empty())
            {
                // A bad name was given, try to help.
                context.Reporter.Error() << Resource::String::OpenSourceFailedNoMatch << ' ' << context.Args.GetArg(Execution::Args::Type::Source) << std::endl;
                context.Reporter.Info() << Resource::String::OpenSourceFailedNoMatchHelp << std::endl;
                for (const auto& details : sources)
                {
                    context.Reporter.Info() << "  "_liv << details.Name << std::endl;
                }

                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST);
            }
            else
            {
                // Even if a name was given, there are no sources
                context.Reporter.Error() << Resource::String::OpenSourceFailedNoSourceDefined << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_SOURCES_DEFINED);
            }
        }
        else
        {
            context.Add<Execution::Data::Source>(std::move(source));
        }
    }

    void OpenPredefinedSource::operator()(Execution::Context& context) const
    {
        std::shared_ptr<Repository::ISource> source;
        try
        {
            source = context.Reporter.ExecuteWithProgress(std::bind(Repository::OpenPredefinedSource, m_predefinedSource, std::placeholders::_1), true);
        }
        catch (...)
        {
            context.Reporter.Error() << Resource::String::SourceOpenPredefinedFailedSuggestion << std::endl;
            throw;
        }

        // A well known predefined source should return a value.
        THROW_HR_IF(E_UNEXPECTED, !source);

        context.Add<Execution::Data::Source>(std::move(source));
    }

    void OpenCompositeSource::operator()(Execution::Context& context) const
    {
        // Get the already open source for use as the available.
        std::shared_ptr<Repository::ISource> availableSource = context.Get<Execution::Data::Source>();

        // Open the predefined source.
        context << OpenPredefinedSource(m_predefinedSource);

        // Create the composite source from the two.
        std::shared_ptr<Repository::ISource> compositeSource = Repository::CreateCompositeSource(context.Get<Execution::Data::Source>(), availableSource);

        // Overwrite the source with the composite.
        context.Add<Execution::Data::Source>(std::move(compositeSource));
    }

    void SearchSourceForMany(Execution::Context& context)
    {
        const auto& args = context.Args;

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

        SearchSourceApplyFilters(context, searchRequest, matchType);

        Logging::Telemetry().LogSearchRequest(
            "many",
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

    void SearchSourceForSingle(Execution::Context& context)
    {
        const auto& args = context.Args;

        MatchType matchType = MatchType::CaseInsensitive;
        if (args.Contains(Execution::Args::Type::Exact))
        {
            matchType = MatchType::Exact;
        }

        SearchRequest searchRequest;
        if (args.Contains(Execution::Args::Type::Query))
        {
            std::string_view query = args.GetArg(Execution::Args::Type::Query);

            // Regardless of match type, always use an exact match for the system reference strings.
            searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::PackageFamilyName, MatchType::Exact, query));
            searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, query));

            searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, matchType, query));
            searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Name, matchType, query));
            searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Moniker, matchType, query));
        }

        SearchSourceApplyFilters(context, searchRequest, matchType);

        Logging::Telemetry().LogSearchRequest(
            "single",
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

    void SearchSourceForManyCompletion(Execution::Context& context)
    {
        MatchType matchType = MatchType::StartsWith;

        SearchRequest searchRequest;
        std::string_view query = context.Get<Execution::Data::CompletionData>().Word();
        searchRequest.Query.emplace(RequestMatch(matchType, query));

        SearchSourceApplyFilters(context, searchRequest, matchType);

        context.Add<Execution::Data::SearchResult>(context.Get<Execution::Data::Source>()->Search(searchRequest));
    }

    void SearchSourceForSingleCompletion(Execution::Context& context)
    {
        MatchType matchType = MatchType::StartsWith;

        SearchRequest searchRequest;
        std::string_view query = context.Get<Execution::Data::CompletionData>().Word();
        searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, matchType, query));
        searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Name, matchType, query));
        searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Moniker, matchType, query));

        SearchSourceApplyFilters(context, searchRequest, matchType);

        context.Add<Execution::Data::SearchResult>(context.Get<Execution::Data::Source>()->Search(searchRequest));
    }

    void SearchSourceForCompletionField::operator()(Execution::Context& context) const
    {
        const std::string& word = context.Get<Execution::Data::CompletionData>().Word();

        SearchRequest searchRequest;
        searchRequest.Inclusions.emplace_back(PackageMatchFilter(m_field, MatchType::StartsWith, word));

        // If filters are provided, be generous with the search no matter the intended result.
        SearchSourceApplyFilters(context, searchRequest, MatchType::Substring);

        context.Add<Execution::Data::SearchResult>(context.Get<Execution::Data::Source>()->Search(searchRequest));
    }

    void ReportSearchResult(Execution::Context& context)
    {
        auto& searchResult = context.Get<Execution::Data::SearchResult>();
        Logging::Telemetry().LogSearchResultCount(searchResult.Matches.size());

        bool sourceIsComposite = context.Get<Execution::Data::Source>()->IsComposite();
        Execution::TableOutput<5> table(context.Reporter,
            {
                Resource::String::SearchName,
                Resource::String::SearchId,
                Resource::String::SearchVersion,
                Resource::String::SearchMatch,
                Resource::String::SearchSource
            });

        for (size_t i = 0; i < searchResult.Matches.size(); ++i)
        {
            auto latestVersion = searchResult.Matches[i].Package->GetLatestAvailableVersion();

            table.OutputLine({
                latestVersion->GetProperty(PackageVersionProperty::Name),
                latestVersion->GetProperty(PackageVersionProperty::Id),
                latestVersion->GetProperty(PackageVersionProperty::Version),
                GetMatchCriteriaDescriptor(searchResult.Matches[i]),
                sourceIsComposite ? static_cast<std::string>(latestVersion->GetProperty(PackageVersionProperty::SourceName)) : ""s
            });
        }

        table.Complete();

        if (searchResult.Truncated)
        {
            context.Reporter.Info() << '<' << Resource::String::SearchTruncated << '>' << std::endl;
        }
    }

    void ReportMultiplePackageFoundResult(Execution::Context& context)
    {
        auto& searchResult = context.Get<Execution::Data::SearchResult>();

        Execution::TableOutput<5> table(context.Reporter,
            {
                Resource::String::SearchName,
                Resource::String::SearchId
            });

        for (size_t i = 0; i < searchResult.Matches.size(); ++i)
        {
            auto package = searchResult.Matches[i].Package;

            table.OutputLine({
                package->GetProperty(PackageProperty::Name),
                package->GetProperty(PackageProperty::Id)
                });
        }

        table.Complete();

        if (searchResult.Truncated)
        {
            context.Reporter.Info() << '<' << Resource::String::SearchTruncated << '>' << std::endl;
        }
    }

    void ReportListResult::operator()(Execution::Context& context) const
    {
        auto& searchResult = context.Get<Execution::Data::SearchResult>();

        Execution::TableOutput<5> table(context.Reporter,
            {
                Resource::String::SearchName,
                Resource::String::SearchId,
                Resource::String::SearchVersion,
                Resource::String::AvailableHeader,
                Resource::String::SearchSource
            });

        for (const auto& match : searchResult.Matches)
        {
            auto installedVersion = match.Package->GetInstalledVersion();

            if (installedVersion)
            {
                auto latestVersion = match.Package->GetLatestAvailableVersion();
                bool updateAvailable = match.Package->IsUpdateAvailable();

                // The only time we don't want to output a line is when filtering and no update is available.
                if (updateAvailable || !m_onlyShowUpgrades)
                {
                    Utility::LocIndString availableVersion, sourceName;

                    if (updateAvailable)
                    {
                        availableVersion = latestVersion->GetProperty(PackageVersionProperty::Version);
                        sourceName = latestVersion->GetProperty(PackageVersionProperty::SourceName);
                    }

                    table.OutputLine({
                        match.Package->GetProperty(PackageProperty::Name),
                        match.Package->GetProperty(PackageProperty::Id),
                        installedVersion->GetProperty(PackageVersionProperty::Version),
                        availableVersion,
                        sourceName
                        });
                }
            }
        }

        table.Complete();

        if (table.IsEmpty())
        {
            context.Reporter.Info() << Resource::String::NoInstalledPackageFound << std::endl;
        }

        if (searchResult.Truncated)
        {
            context.Reporter.Info() << '<' << Resource::String::SearchTruncated << '>' << std::endl;
        }
    }

    void EnsureMatchesFromSearchResult::operator()(Execution::Context& context) const
    {
        auto& searchResult = context.Get<Execution::Data::SearchResult>();

        if (searchResult.Matches.size() == 0)
        {
            Logging::Telemetry().LogNoAppMatch();

            if (m_isFromInstalledSource)
            {
                context.Reporter.Info() << Resource::String::NoInstalledPackageFound << std::endl;
            }
            else
            {
                context.Reporter.Info() << Resource::String::NoPackageFound << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
        }
    }

    void EnsureOneMatchFromSearchResult::operator()(Execution::Context& context) const
    {
        context <<
            EnsureMatchesFromSearchResult(m_isFromInstalledSource);

        if (!context.IsTerminated())
        {
            auto& searchResult = context.Get<Execution::Data::SearchResult>();

            if (searchResult.Matches.size() > 1)
            {
                Logging::Telemetry().LogMultiAppMatch();

                if (m_isFromInstalledSource)
                {
                    context.Reporter.Warn() << Resource::String::MultipleInstalledPackagesFound << std::endl;
                }
                else
                {
                    context.Reporter.Warn() << Resource::String::MultiplePackagesFound << std::endl;
                }

                context << ReportMultiplePackageFoundResult;

                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND);
            }

            std::shared_ptr<IPackage> package = searchResult.Matches.at(0).Package;
            Logging::Telemetry().LogAppFound(package->GetProperty(PackageProperty::Name), package->GetProperty(PackageProperty::Id));

            context.Add<Execution::Data::Package>(std::move(package));
        };
    }

    void GetManifestFromPackage(Execution::Context& context)
    {
        std::string_view version = context.Args.GetArg(Execution::Args::Type::Version);
        std::string_view channel = context.Args.GetArg(Execution::Args::Type::Channel);

        PackageVersionKey key("", version, channel);
        auto requestedVersion = context.Get<Execution::Data::Package>()->GetAvailableVersion(key);

        std::optional<Manifest::Manifest> manifest;
        if (requestedVersion)
        {
            manifest = requestedVersion->GetManifest();
        }

        if (!manifest)
        {
            context.Reporter.Error() << Resource::String::GetManifestResultVersionNotFound << ' ';
            if (!version.empty())
            {
                context.Reporter.Error() << version;
            }
            if (!channel.empty())
            {
                context.Reporter.Error() << '[' << channel << ']';
            }

            context.Reporter.Error() << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_MANIFEST_FOUND);
        }

        Logging::Telemetry().LogManifestFields(manifest->Id, manifest->Name, manifest->Version);
        context.Add<Execution::Data::Manifest>(std::move(manifest.value()));
        context.Add<Execution::Data::PackageVersion>(std::move(requestedVersion));
    }

    void VerifyFile::operator()(Execution::Context& context) const
    {
        std::filesystem::path path = Utility::ConvertToUTF16(context.Args.GetArg(m_arg));

        if (!std::filesystem::exists(path))
        {
            context.Reporter.Error() << Resource::String::VerifyFileFailedNotExist << ' ' << path.u8string() << std::endl;
            AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
        }

        if (std::filesystem::is_directory(path))
        {
            context.Reporter.Error() << Resource::String::VerifyFileFailedIsDirectory << ' ' << path.u8string() << std::endl;
            AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_DIRECTORY_NOT_SUPPORTED));
        }
    }

    void GetManifestFromArg(Execution::Context& context)
    {
        Logging::Telemetry().LogIsManifestLocal(true);

        context <<
            VerifyFile(Execution::Args::Type::Manifest) <<
            [](Execution::Context& context)
        {
            Manifest::Manifest manifest = Manifest::YamlParser::CreateFromPath(Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::Manifest)));
            Logging::Telemetry().LogManifestFields(manifest.Id, manifest.Name, manifest.Version);
            context.Add<Execution::Data::Manifest>(std::move(manifest));
        };
    }

    void ReportPackageIdentity(Execution::Context& context)
    {
        auto package = context.Get<Execution::Data::Package>();
        ReportIdentity(context, package->GetProperty(PackageProperty::Name), package->GetProperty(PackageProperty::Id));
    }

    void ReportManifestIdentity(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        ReportIdentity(context, manifest.Name, manifest.Id);
    }

    void GetManifest(Execution::Context& context)
    {
        if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            context <<
                GetManifestFromArg <<
                ReportManifestIdentity;
        }
        else
        {
            context <<
                OpenSource <<
                SearchSourceForSingle <<
                EnsureOneMatchFromSearchResult(false) <<
                ReportPackageIdentity <<
                GetManifestFromPackage;
        }
    }

    void SelectInstaller(Execution::Context& context)
    {
        bool isUpdate = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate);

        IPackageVersion::Metadata installationMetadata;
        if (isUpdate)
        {
            installationMetadata = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata();
        }

        ManifestComparator manifestComparator(context.Args, std::move(installationMetadata));
        context.Add<Execution::Data::Installer>(manifestComparator.GetPreferredInstaller(context.Get<Execution::Data::Manifest>()));
    }

    void EnsureRunningAsAdmin(Execution::Context& context)
    {
        if (!Runtime::IsRunningAsAdmin())
        {
            context.Reporter.Error() << Resource::String::CommandRequiresAdmin;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_COMMAND_REQUIRES_ADMIN);
        }
    }

    void EnsureFeatureEnabled::operator()(Execution::Context& context) const
    {
        if (!Settings::ExperimentalFeature::IsEnabled(m_feature))
        {
            context.Reporter.Error() << Resource::String::FeatureDisabledMessage << " : '" <<
                Settings::ExperimentalFeature::GetFeature(m_feature).JsonName() << '\'' << std::endl;
            AICLI_LOG(CLI, Error, << Settings::ExperimentalFeature::GetFeature(m_feature).Name() << " feature is disabled. Execution cancelled.");
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_EXPERIMENTAL_FEATURE_DISABLED);
        }
    }

    void SearchSourceUsingManifest(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        auto source = context.Get<Execution::Data::Source>();

        // First try search using ProductId or PackageFamilyName
        for (const auto& installer : manifest.Installers)
        {
            SearchRequest searchRequest;
            if (!installer.PackageFamilyName.empty())
            {
                searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::PackageFamilyName, MatchType::Exact, installer.PackageFamilyName));
            }
            else if (!installer.ProductCode.empty())
            {
                searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, installer.ProductCode));
            }

            if (!searchRequest.Inclusions.empty())
            {
                auto searchResult = source->Search(searchRequest);

                if (!searchResult.Matches.empty())
                {
                    context.Add<Execution::Data::SearchResult>(std::move(searchResult));
                    return;
                }
            }
        }

        // If we cannot find a package using PackageFamilyName or ProductId, try manifest Id and Name pair
        SearchRequest searchRequest;
        searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::CaseInsensitive, manifest.Id));
        // In case there're same Ids from different sources, filter the result using package name
        searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Name, MatchType::CaseInsensitive, manifest.Name));

        context.Add<Execution::Data::SearchResult>(source->Search(searchRequest));
    }

    void GetInstalledPackageVersion(Execution::Context& context)
    {
        context.Add<Execution::Data::InstalledPackageVersion>(context.Get<Execution::Data::Package>()->GetInstalledVersion());
    }

    void ReportExecutionStage::operator()(Execution::Context& context) const
    {
        if (!context.Contains(Execution::Data::ExecutionStage))
        {
            context.Add<Execution::Data::ExecutionStage>(m_stage);
        }
        else if (context.Get<Execution::Data::ExecutionStage>() == m_stage)
        {
            return;
        }
        else if (context.Get<Execution::Data::ExecutionStage>() < m_stage || m_allowBackward)
        {
            context.Get<Execution::Data::ExecutionStage>() = m_stage;
        }
        else
        {
            THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), "Reporting ExecutionStage to an earlier Stage without allowBackward as true");
        }

        Logging::SetExecutionStage(static_cast<uint32_t>(context.Get<Execution::Data::ExecutionStage>()));
    }
}

AppInstaller::CLI::Execution::Context& operator<<(AppInstaller::CLI::Execution::Context& context, AppInstaller::CLI::Workflow::WorkflowTask::Func f)
{
    return (context << AppInstaller::CLI::Workflow::WorkflowTask(f));
}

AppInstaller::CLI::Execution::Context& operator<<(AppInstaller::CLI::Execution::Context& context, const AppInstaller::CLI::Workflow::WorkflowTask& task)
{
    if (!context.IsTerminated())
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (context.ShouldExecuteWorkflowTask(task))
#endif
        {
            task(context);
        }
    }
    return context;
}
