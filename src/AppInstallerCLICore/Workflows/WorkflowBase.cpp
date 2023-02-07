// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowBase.h"
#include "ExecutionContext.h"
#include "ManifestComparator.h"
#include "PromptFlow.h"
#include "TableOutput.h"
#include <winget/ExperimentalFeature.h>
#include <winget/ManifestYamlParser.h>
#include <winget/Pin.h>

using namespace std::string_literals;
using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::Pinning;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        std::string GetMatchCriteriaDescriptor(const ResultMatch& match)
        {
            if (match.MatchCriteria.Field != PackageMatchField::Id && match.MatchCriteria.Field != PackageMatchField::Name)
            {
                std::string result{ ToString(match.MatchCriteria.Field) };
                result += ": ";
                result += match.MatchCriteria.Value;
                return result;
            }
            else
            {
                return {};
            }
        }

        void ReportIdentity(
            Execution::Context& context,
            Utility::LocIndView prefix,
            std::optional<Resource::StringId> label,
            std::string_view name,
            std::string_view id,
            std::string_view version = {},
            Execution::Reporter::Level level = Execution::Reporter::Level::Info)
        {
            auto out = context.Reporter.GetOutputStream(level);
            out << prefix;
            if (label)
            {
                out << *label << ' ';
            }
            out << Execution::NameEmphasis << name << " ["_liv << Execution::IdEmphasis << id << ']';

            if (!version.empty())
            {
                out << ' ' << Resource::String::ShowVersion << ' ' << version;
            }

            out << std::endl;
        }

        Repository::Source OpenNamedSource(Execution::Context& context, Utility::LocIndView sourceName)
        {
            Repository::Source source;

            try
            {
                source = Source{ sourceName };

                if (!source)
                {
                    std::vector<SourceDetails> sources = Source::GetCurrentSources();

                    if (!sourceName.empty() && !sources.empty())
                    {
                        // A bad name was given, try to help.
                        context.Reporter.Error() << Resource::String::OpenSourceFailedNoMatch(sourceName) << std::endl;
                        context.Reporter.Info() << Resource::String::OpenSourceFailedNoMatchHelp << std::endl;
                        for (const auto& details : sources)
                        {
                            context.Reporter.Info() << "  "_liv << details.Name << std::endl;
                        }

                        AICLI_TERMINATE_CONTEXT_RETURN(APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST, {});
                    }
                    else
                    {
                        // Even if a name was given, there are no sources
                        context.Reporter.Error() << Resource::String::OpenSourceFailedNoSourceDefined << std::endl;
                        AICLI_TERMINATE_CONTEXT_RETURN(APPINSTALLER_CLI_ERROR_NO_SOURCES_DEFINED, {});
                    }
                }

                if (context.Args.Contains(Execution::Args::Type::CustomHeader))
                {
                    std::string customHeader{ context.Args.GetArg(Execution::Args::Type::CustomHeader) };
                    if (!source.SetCustomHeader(customHeader))
                    {
                        context.Reporter.Warn() << Resource::String::HeaderArgumentNotApplicableForNonRestSourceWarning << std::endl;
                    }
                }

                auto openFunction = [&](IProgressCallback& progress)->std::vector<Repository::SourceDetails> { return source.Open(progress); };
                auto updateFailures = context.Reporter.ExecuteWithProgress(openFunction, true);

                // We'll only report the source update failure as warning and continue
                for (const auto& s : updateFailures)
                {
                    context.Reporter.Warn() << Resource::String::SourceOpenWithFailedUpdate(Utility::LocIndView{ s.Name }) << std::endl;
                }
            }
            catch (const wil::ResultException& re)
            {
                context.Reporter.Error() << Resource::String::SourceOpenFailedSuggestion << std::endl;
                if (re.GetErrorCode() == APPINSTALLER_CLI_ERROR_FAILED_TO_OPEN_ALL_SOURCES)
                {
                    // Since we know there must have been multiple errors here, just fail the context rather
                    // than trying to get one of the exceptions back out.
                    AICLI_TERMINATE_CONTEXT_RETURN(APPINSTALLER_CLI_ERROR_FAILED_TO_OPEN_ALL_SOURCES, {});
                }
                else
                {
                    throw;
                }
            }
            catch (...)
            {
                context.Reporter.Error() << Resource::String::SourceOpenFailedSuggestion << std::endl;
                throw;
            }

            return source;
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

            if (args.Contains(Execution::Args::Type::ProductCode))
            {
                searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, matchType, args.GetArg(Execution::Args::Type::ProductCode)));
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

        // Data shown on a line of a table displaying installed packages
        struct InstalledPackagesTableLine
        {
            InstalledPackagesTableLine(Utility::LocIndString name, Utility::LocIndString id, Utility::LocIndString installedVersion, Utility::LocIndString availableVersion, Utility::LocIndString source)
                : Name(name), Id(id), InstalledVersion(installedVersion), AvailableVersion(availableVersion), Source(source) {}

            Utility::LocIndString Name;
            Utility::LocIndString Id;
            Utility::LocIndString InstalledVersion;
            Utility::LocIndString AvailableVersion;
            Utility::LocIndString Source;
        };

        void OutputInstalledPackagesTable(Execution::Context& context, const std::vector<InstalledPackagesTableLine>& lines)
        {
            Execution::TableOutput<5> table(context.Reporter,
                {
                    Resource::String::SearchName,
                    Resource::String::SearchId,
                    Resource::String::SearchVersion,
                    Resource::String::AvailableHeader,
                    Resource::String::SearchSource
                });

            for (const auto& line : lines)
            {
                table.OutputLine({
                    line.Name,
                    line.Id,
                    line.InstalledVersion,
                    line.AvailableVersion,
                    line.Source
                    });
            }

            table.Complete();
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

    Repository::PredefinedSource DetermineInstalledSource(const Execution::Context& context)
    {
        Repository::PredefinedSource installedSource = Repository::PredefinedSource::Installed;
        Manifest::ScopeEnum scope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        if (scope == Manifest::ScopeEnum::Machine)
        {
            installedSource = Repository::PredefinedSource::InstalledMachine;
        }
        else if (scope == Manifest::ScopeEnum::User)
        {
            installedSource = Repository::PredefinedSource::InstalledUser;
        }

        return installedSource;
    }

    HRESULT HandleException(Execution::Context& context, std::exception_ptr exception)
    {
        try
        {
            std::rethrow_exception(exception);
        }
        // Exceptions that may occur in the process of executing an arbitrary command
        catch (const wil::ResultException& re)
        {
            // Even though they are logged at their source, log again here for completeness.
            Logging::Telemetry().LogException(Logging::FailureTypeEnum::ResultException, re.what());
            context.Reporter.Error() <<
                Resource::String::UnexpectedErrorExecutingCommand << ' ' << std::endl <<
                GetUserPresentableMessage(re) << std::endl;
            return re.GetErrorCode();
        }
        catch (const winrt::hresult_error& hre)
        {
            std::string message = GetUserPresentableMessage(hre);
            Logging::Telemetry().LogException(Logging::FailureTypeEnum::WinrtHResultError, message);
            context.Reporter.Error() <<
                Resource::String::UnexpectedErrorExecutingCommand << ' ' << std::endl <<
                message << std::endl;
            return hre.code();
        }
        catch (const Settings::GroupPolicyException& e)
        {
            auto policy = Settings::TogglePolicy::GetPolicy(e.Policy());
            auto policyNameId = policy.PolicyName();
            context.Reporter.Error() << Resource::String::DisabledByGroupPolicy(policyNameId) << std::endl;
            return APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY;
        }
        catch (const std::exception& e)
        {
            Logging::Telemetry().LogException(Logging::FailureTypeEnum::StdException, e.what());
            context.Reporter.Error() <<
                Resource::String::UnexpectedErrorExecutingCommand << ' ' << std::endl <<
                GetUserPresentableMessage(e) << std::endl;
            return APPINSTALLER_CLI_ERROR_COMMAND_FAILED;
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            Logging::Telemetry().LogException(Logging::FailureTypeEnum::Unknown, {});
            context.Reporter.Error() <<
                Resource::String::UnexpectedErrorExecutingCommand << " ???"_liv << std::endl;
            return APPINSTALLER_CLI_ERROR_COMMAND_FAILED;
        }

        return E_UNEXPECTED;
    }

    void OpenSource::operator()(Execution::Context& context) const
    {
        std::string_view sourceName;
        if (m_forDependencies)
        {
            if (context.Args.Contains(Execution::Args::Type::DependencySource))
            {
                sourceName = context.Args.GetArg(Execution::Args::Type::DependencySource);
            }
        }
        else
        {
            if (context.Args.Contains(Execution::Args::Type::Source))
            {
                sourceName = context.Args.GetArg(Execution::Args::Type::Source);
            }
        }

        auto source = OpenNamedSource(context, Utility::LocIndView{ sourceName });
        if (context.IsTerminated())
        {
            return;
        }

        context << HandleSourceAgreements(source);
        if (context.IsTerminated())
        {
            return;
        }

        if (m_forDependencies)
        {
            context.Add<Execution::Data::DependencySource>(std::move(source));
        }
        else
        {
            context.Add<Execution::Data::Source>(std::move(source));
        }
    }

    void OpenNamedSourceForSources::operator()(Execution::Context& context) const
    {
        auto source = OpenNamedSource(context, m_sourceName);
        if (context.IsTerminated())
        {
            return;
        }

        context << HandleSourceAgreements(source);
        if (context.IsTerminated())
        {
            return;
        }

        if (!context.Contains(Execution::Data::Sources))
        {
            context.Add<Execution::Data::Sources>({ std::move(source) });
        }
        else
        {
            context.Get<Execution::Data::Sources>().emplace_back(std::move(source));
        }
    }

    void OpenPredefinedSource::operator()(Execution::Context& context) const
    {
        Repository::Source source;
        try
        {
            source = Source{ m_predefinedSource };

            // A well known predefined source should return a value.
            THROW_HR_IF(E_UNEXPECTED, !source);

            auto openFunction = [&](IProgressCallback& progress)->std::vector<Repository::SourceDetails> { return source.Open(progress); };
            context.Reporter.ExecuteWithProgress(openFunction, true);
        }
        catch (...)
        {
            context.Reporter.Error() << Resource::String::SourceOpenPredefinedFailedSuggestion << std::endl;
            throw;
        }

        if (m_forDependencies)
        {
            context.Add<Execution::Data::DependencySource>(std::move(source));
        }
        else 
        {
            context.Add<Execution::Data::Source>(std::move(source));
        }
    }

    void OpenCompositeSource::operator()(Execution::Context& context) const
    {
        // Get the already open source for use as the available.
        Repository::Source availableSource;
        if (m_forDependencies)
        {
            availableSource = context.Get<Execution::Data::DependencySource>();
        }
        else
        {
            availableSource = context.Get<Execution::Data::Source>();
        }

        // Open the predefined source.
        context << OpenPredefinedSource(m_predefinedSource, m_forDependencies);

        // Create the composite source from the two.
        Repository::Source source;
        if (m_forDependencies)
        {
            source = context.Get<Execution::Data::DependencySource>();
        }
        else
        {
            source = context.Get<Execution::Data::Source>();
        }

        Repository::Source compositeSource{ source, availableSource, m_searchBehavior };

        // Overwrite the source with the composite.
        if (m_forDependencies)
        {
            context.Add<Execution::Data::DependencySource>(std::move(compositeSource));
        }
        else
        {
            context.Add<Execution::Data::Source>(std::move(compositeSource));
        }
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

        context.Add<Execution::Data::SearchResult>(context.Get<Execution::Data::Source>().Search(searchRequest));
    }

    void GetSearchRequestForSingle(Execution::Context& context)
    {
        const auto& args = context.Args;

        MatchType matchType = MatchType::CaseInsensitive;
        if (args.Contains(Execution::Args::Type::Exact))
        {
            matchType = MatchType::Exact;
        }

        SearchRequest searchRequest;
        // Note: MultiQuery when we need search for single is handled with one sub-context per query.
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

        context.Add<Execution::Data::SearchRequest>(std::move(searchRequest));
    }

    void SearchSourceForSingle(Execution::Context& context)
    {
        const auto& args = context.Args;
        context << GetSearchRequestForSingle;
        if (!context.IsTerminated())
        {
            const auto& searchRequest = context.Get<Execution::Data::SearchRequest>();

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

            context.Add<Execution::Data::SearchResult>(context.Get<Execution::Data::Source>().Search(searchRequest));
        }
    }

    void SearchSourceForManyCompletion(Execution::Context& context)
    {
        MatchType matchType = MatchType::StartsWith;

        SearchRequest searchRequest;
        std::string_view query = context.Get<Execution::Data::CompletionData>().Word();
        searchRequest.Query.emplace(RequestMatch(matchType, query));

        SearchSourceApplyFilters(context, searchRequest, matchType);

        context.Add<Execution::Data::SearchResult>(context.Get<Execution::Data::Source>().Search(searchRequest));
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

        context.Add<Execution::Data::SearchResult>(context.Get<Execution::Data::Source>().Search(searchRequest));
    }

    void SearchSourceForCompletionField::operator()(Execution::Context& context) const
    {
        const std::string& word = context.Get<Execution::Data::CompletionData>().Word();

        SearchRequest searchRequest;
        searchRequest.Inclusions.emplace_back(PackageMatchFilter(m_field, MatchType::StartsWith, word));

        // If filters are provided, be generous with the search no matter the intended result.
        SearchSourceApplyFilters(context, searchRequest, MatchType::Substring);

        context.Add<Execution::Data::SearchResult>(context.Get<Execution::Data::Source>().Search(searchRequest));
    }

    void ReportSearchResult(Execution::Context& context)
    {
        auto& searchResult = context.Get<Execution::Data::SearchResult>();

        bool sourceIsComposite = context.Get<Execution::Data::Source>().IsComposite();
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
            auto latestVersion = searchResult.Matches[i].Package->GetLatestAvailableVersion(PinBehavior::IgnorePins);

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

    void HandleSearchResultFailures(Execution::Context& context)
    {
        const auto& searchResult = context.Get<Execution::Data::SearchResult>();

        if (!searchResult.Failures.empty())
        {
            if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::TreatSourceFailuresAsWarning))
            {
                auto warn = context.Reporter.Warn();
                for (const auto& failure : searchResult.Failures)
                {
                    warn << Resource::String::SearchFailureWarning(Utility::LocIndView{ failure.SourceName }) << std::endl;
                }
            }
            else
            {
                HRESULT overallHR = S_OK;
                auto error = context.Reporter.Error();
                for (const auto& failure : searchResult.Failures)
                {
                    error << Resource::String::SearchFailureError(Utility::LocIndView{ failure.SourceName }) << std::endl;
                    HRESULT failureHR = HandleException(context, failure.Exception);

                    // Just take first failure for now
                    if (overallHR == S_OK)
                    {
                        overallHR = failureHR;
                    }
                }

                if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::ShowSearchResultsOnPartialFailure))
                {
                    if (searchResult.Matches.empty())
                    {
                        context.Reporter.Info() << std::endl << Resource::String::SearchFailureErrorNoMatches << std::endl;
                    }
                    else
                    {
                        context.Reporter.Info() << std::endl << Resource::String::SearchFailureErrorListMatches << std::endl;
                        context << ReportMultiplePackageFoundResultWithSource;
                    }
                }

                context.SetTerminationHR(overallHR);
            }
        }
    }

    void ReportMultiplePackageFoundResult(Execution::Context& context)
    {
        auto& searchResult = context.Get<Execution::Data::SearchResult>();

        Execution::TableOutput<2> table(context.Reporter,
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

    void ReportMultiplePackageFoundResultWithSource(Execution::Context& context)
    {
        auto& searchResult = context.Get<Execution::Data::SearchResult>();

        Execution::TableOutput<3> table(context.Reporter,
            {
                Resource::String::SearchName,
                Resource::String::SearchId,
                Resource::String::SearchSource
            });

        for (size_t i = 0; i < searchResult.Matches.size(); ++i)
        {
            auto package = searchResult.Matches[i].Package;

            std::string sourceName;
            auto latest = package->GetLatestAvailableVersion(PinBehavior::IgnorePins);
            if (latest)
            {
                auto source = latest->GetSource();
                if (source)
                {
                    sourceName = source.GetDetails().Name;
                }
            }

            table.OutputLine({
                package->GetProperty(PackageProperty::Name),
                package->GetProperty(PackageProperty::Id),
                std::move(sourceName)
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

        std::vector<InstalledPackagesTableLine> lines;
        std::vector<InstalledPackagesTableLine> linesForExplicitUpgrade;

        int availableUpgradesCount = 0;
        int packagesWithUnknownVersionSkipped = 0;
        int packagesWithUserPinsSkipped = 0;
        auto &source = context.Get<Execution::Data::Source>();
        bool shouldShowSource = source.IsComposite() && source.GetAvailableSources().size() > 1;

        PinBehavior pinBehavior;
        if (m_onlyShowUpgrades && !context.Args.Contains(Execution::Args::Type::Force))
        {
            // For listing upgrades, show the version we would upgrade to with the given pins.
            pinBehavior = context.Args.Contains(Execution::Args::Type::IncludePinned) ? PinBehavior::IncludePinned : PinBehavior::ConsiderPins;
        }
        else
        {
            // For listing installed apps or if we are ignoring pins due to --force, show the latest available.
            pinBehavior = PinBehavior::IgnorePins;
        }

        for (const auto& match : searchResult.Matches)
        {
            auto installedVersion = match.Package->GetInstalledVersion();

            if (installedVersion)
            {
                auto latestVersion = match.Package->GetLatestAvailableVersion(pinBehavior);
                bool updateAvailable = match.Package->IsUpdateAvailable(pinBehavior);

                if (m_onlyShowUpgrades && !context.Args.Contains(Execution::Args::Type::IncludeUnknown) && Utility::Version(installedVersion->GetProperty(PackageVersionProperty::Version)).IsUnknown() && updateAvailable)
                {
                    // We are only showing upgrades, and the user did not request to include packages with unknown versions.
                    ++packagesWithUnknownVersionSkipped;
                    continue;
                }

                if (m_onlyShowUpgrades && !updateAvailable && ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::Pinning))
                {
                    bool updateAvailableWithoutPins = match.Package->IsUpdateAvailable(PinBehavior::IgnorePins);
                    if (updateAvailableWithoutPins)
                    {
                        ++packagesWithUserPinsSkipped;
                        continue;
                    }
                }

                // The only time we don't want to output a line is when filtering and no update is available.
                if (updateAvailable || !m_onlyShowUpgrades)
                {
                    Utility::LocIndString availableVersion, sourceName;

                    if (latestVersion)
                    {
                        // Always show the source for correlated packages
                        sourceName = latestVersion->GetProperty(PackageVersionProperty::SourceName);

                        if (updateAvailable)
                        {
                            availableVersion = latestVersion->GetProperty(PackageVersionProperty::Version);
                            availableUpgradesCount++;
                        }
                    }

                    // Output using the local PackageName instead of the name in the manifest, to prevent confusion for packages that add multiple
                    // Add/Remove Programs entries.
                    // TODO: De-duplicate this list, and only show (by default) one entry per matched package.
                    InstalledPackagesTableLine line(
                         installedVersion->GetProperty(PackageVersionProperty::Name),
                         match.Package->GetProperty(PackageProperty::Id),
                         installedVersion->GetProperty(PackageVersionProperty::Version),
                         availableVersion,
                         shouldShowSource ? sourceName : Utility::LocIndString()
                    );

                    auto pinnedState = ConvertToPinTypeEnum(installedVersion->GetMetadata()[PackageVersionMetadata::PinnedState]);
                    if (m_onlyShowUpgrades && pinnedState == PinType::PinnedByManifest)
                    {
                        linesForExplicitUpgrade.push_back(std::move(line));
                    }
                    else
                    {
                        lines.push_back(std::move(line));
                    }
                }
            }
        }

        OutputInstalledPackagesTable(context, lines);

        if (lines.empty())
        {
            context.Reporter.Info() << Resource::String::NoInstalledPackageFound << std::endl;
        }
        else
        {
            if (searchResult.Truncated)
            {
                context.Reporter.Info() << '<' << Resource::String::SearchTruncated << '>' << std::endl;
            }

            if (m_onlyShowUpgrades)
            {
                context.Reporter.Info() << Resource::String::AvailableUpgrades(availableUpgradesCount) << std::endl;
            }
        }

        if (!linesForExplicitUpgrade.empty())
        {
            context.Reporter.Info() << std::endl << Resource::String::UpgradeAvailableForPinned << std::endl;
            OutputInstalledPackagesTable(context, linesForExplicitUpgrade);
        }

        if (m_onlyShowUpgrades)
        {
            if (packagesWithUnknownVersionSkipped > 0)
            {
                AICLI_LOG(CLI, Info, << packagesWithUnknownVersionSkipped << " package(s) skipped due to unknown installed version");
                context.Reporter.Info() << Resource::String::UpgradeUnknownVersionCount(packagesWithUnknownVersionSkipped) << std::endl;
            }

            if (packagesWithUserPinsSkipped > 0)
            {
                AICLI_LOG(CLI, Info, << packagesWithUserPinsSkipped << " package(s) skipped due to user pins");
                context.Reporter.Info() << Resource::String::UpgradePinnedByUserCount(packagesWithUserPinsSkipped) << std::endl;
            }
        }
    }

    void EnsureMatchesFromSearchResult::operator()(Execution::Context& context) const
    {
        auto& searchResult = context.Get<Execution::Data::SearchResult>();

        Logging::Telemetry().LogSearchResultCount(searchResult.Matches.size());

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
                    context << ReportMultiplePackageFoundResult;
                }
                else
                {
                    context.Reporter.Warn() << Resource::String::MultiplePackagesFound << std::endl;
                    context << ReportMultiplePackageFoundResultWithSource;
                }

                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND);
            }

            std::shared_ptr<IPackage> package = searchResult.Matches.at(0).Package;
            Logging::Telemetry().LogAppFound(package->GetProperty(PackageProperty::Name), package->GetProperty(PackageProperty::Id));

            context.Add<Execution::Data::Package>(std::move(package));
        }
    }

    void GetManifestWithVersionFromPackage::operator()(Execution::Context& context) const
    {
        PackageVersionKey key("", m_version, m_channel);

        std::shared_ptr<IPackage> package = context.Get<Execution::Data::Package>();
        std::shared_ptr<IPackageVersion> requestedVersion;

        if (m_considerPins && ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::Pinning))
        {
            bool isPinned = false;

            // TODO: The logic here will probably have to get more difficult once we support channels
            if (Utility::IsEmptyOrWhitespace(m_version) && Utility::IsEmptyOrWhitespace(m_channel))
            {
                PinBehavior pinBehavior;
                if (context.Args.Contains(Execution::Args::Type::Force))
                {
                    // --force ignores any pins
                    pinBehavior = PinBehavior::IgnorePins;
                }
                else
                {
                    pinBehavior = context.Args.Contains(Execution::Args::Type::IncludePinned) ? PinBehavior::IncludePinned : PinBehavior::ConsiderPins;
                }

                requestedVersion = package->GetLatestAvailableVersion(pinBehavior);

                if (!requestedVersion)
                {
                    // Check whether we didn't find the latest version because it was pinned or because there wasn't one
                    auto latestVersion = package->GetLatestAvailableVersion(PinBehavior::IgnorePins);
                    if (latestVersion)
                    {
                        isPinned = true;
                    }
                }
            }
            else
            {
                auto requestedVersionAndPin = package->GetAvailableVersionAndPin(key);
                requestedVersion = requestedVersionAndPin.first;
                auto pin = requestedVersionAndPin.second;

                isPinned =
                    pin == Pinning::PinType::Blocking ||
                    pin == Pinning::PinType::Gating ||
                    (pin == Pinning::PinType::Pinning && !context.Args.Contains(Execution::Args::Type::IncludePinned));
            }

            if (isPinned)
            {
                if (context.Args.Contains(Execution::Args::Type::Force))
                {
                    AICLI_LOG(CLI, Info, << "Ignoring pin on package due to --force argument");
                }
                else
                {
                    AICLI_LOG(CLI, Error, << "The requested package version is unavailable because of a pin");
                    context.Reporter.Error() << Resource::String::PackageIsPinned << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PACKAGE_IS_PINNED);
                }
            }
        }
        else
        {
            // The simple case: Just look up the requested version
            requestedVersion = package->GetAvailableVersion(key);
        }

        std::optional<Manifest::Manifest> manifest;
        if (requestedVersion)
        {
            manifest = requestedVersion->GetManifest();
        }

        if (!manifest)
        {
            std::ostringstream ssVersionInfo;
            if (!m_version.empty())
            {
                ssVersionInfo << m_version;
            }
            if (!m_channel.empty())
            {
                ssVersionInfo << '[' << m_channel << ']';
            }

            context.Reporter.Error() << Resource::String::GetManifestResultVersionNotFound(Utility::LocIndView{ ssVersionInfo.str()}) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_MANIFEST_FOUND);
        }

        Logging::Telemetry().LogManifestFields(manifest->Id, manifest->DefaultLocalization.Get<Manifest::Localization::PackageName>(), manifest->Version);

        std::string targetLocale;
        if (context.Args.Contains(Execution::Args::Type::Locale))
        {
            targetLocale = context.Args.GetArg(Execution::Args::Type::Locale);
        }
        manifest->ApplyLocale(targetLocale);

        context.Add<Execution::Data::Manifest>(std::move(manifest.value()));
        context.Add<Execution::Data::PackageVersion>(std::move(requestedVersion));
    }

    void GetManifestFromPackage::operator()(Execution::Context& context) const
    {
        context << GetManifestWithVersionFromPackage(
            context.Args.GetArg(Execution::Args::Type::Version),
            context.Args.GetArg(Execution::Args::Type::Channel),
            m_considerPins);
    }

    void VerifyFile::operator()(Execution::Context& context) const
    {
        std::filesystem::path path = Utility::ConvertToUTF16(context.Args.GetArg(m_arg));

        if (!std::filesystem::exists(path))
        {
            context.Reporter.Error() << Resource::String::VerifyFileFailedNotExist(Utility::LocIndView{ path.u8string() }) << std::endl;
            AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
        }

        if (std::filesystem::is_directory(path))
        {
            context.Reporter.Error() << Resource::String::VerifyFileFailedIsDirectory(Utility::LocIndView{ path.u8string() }) << std::endl;
            AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_DIRECTORY_NOT_SUPPORTED));
        }
    }

    void VerifyPath::operator()(Execution::Context& context) const
    {
        std::filesystem::path path = Utility::ConvertToUTF16(context.Args.GetArg(m_arg));

        if (!std::filesystem::exists(path))
        {
            context.Reporter.Error() << Resource::String::VerifyPathFailedNotExist(Utility::LocIndView{ path.u8string() }) << std::endl;
            AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND));
        }
    }

    void GetManifestFromArg(Execution::Context& context)
    {
        Logging::Telemetry().LogIsManifestLocal(true);

        context <<
            VerifyPath(Execution::Args::Type::Manifest) <<
            [](Execution::Context& context)
        {
            Manifest::Manifest manifest = Manifest::YamlParser::CreateFromPath(Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::Manifest)));
            Logging::Telemetry().LogManifestFields(manifest.Id, manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>(), manifest.Version);

            std::string targetLocale;
            if (context.Args.Contains(Execution::Args::Type::Locale))
            {
                targetLocale = context.Args.GetArg(Execution::Args::Type::Locale);
            }
            manifest.ApplyLocale(targetLocale);

            context.Add<Execution::Data::Manifest>(std::move(manifest));
        };
    }

    void ReportPackageIdentity(Execution::Context& context)
    {
        auto package = context.Get<Execution::Data::Package>();
        ReportIdentity(context, {}, Resource::String::ReportIdentityFound, package->GetProperty(PackageProperty::Name), package->GetProperty(PackageProperty::Id));
    }

    void ReportManifestIdentity(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        ReportIdentity(context, {}, Resource::String::ReportIdentityFound, manifest.CurrentLocalization.Get<Manifest::Localization::PackageName>(), manifest.Id);
    }

    void ReportManifestIdentityWithVersion::operator()(Execution::Context& context) const
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        ReportIdentity(context, m_prefix, m_label, manifest.CurrentLocalization.Get<Manifest::Localization::PackageName>(), manifest.Id, manifest.Version, m_level);
    }

    void GetManifest::operator()(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            context <<
                GetManifestFromArg;
        }
        else
        {
            context <<
                OpenSource() <<
                SearchSourceForSingle <<
                HandleSearchResultFailures <<
                EnsureOneMatchFromSearchResult(false) <<
                GetManifestFromPackage(m_considerPins);
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


        ManifestComparator manifestComparator(context, installationMetadata);
        auto [installer, inapplicabilities] = manifestComparator.GetPreferredInstaller(context.Get<Execution::Data::Manifest>());

        if (!installer.has_value())
        {
            auto onlyInstalledType = std::find(inapplicabilities.begin(), inapplicabilities.end(), InapplicabilityFlags::InstalledType);
            if (onlyInstalledType != inapplicabilities.end())
            {
                context.Reporter.Info() << Resource::String::UpgradeDifferentInstallTechnology << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
            }
        }

        if (installer.has_value())
        {
            Logging::Telemetry().LogSelectedInstaller(
                static_cast<int>(installer->Arch),
                installer->Url,
                Manifest::InstallerTypeToString(installer->EffectiveInstallerType()),
                Manifest::ScopeToString(installer->Scope),
                installer->Locale);
        }

        context.Add<Execution::Data::Installer>(installer);
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
            context.Reporter.Error()
                << Resource::String::FeatureDisabledMessage(Utility::LocIndView{ Settings::ExperimentalFeature::GetFeature(m_feature).JsonName() })
                << std::endl;
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
            else if (installer.EffectiveInstallerType() == Manifest::InstallerTypeEnum::Portable)
            {
                const auto& productCode = Utility::MakeSuitablePathPart(manifest.Id + '_' + source.GetIdentifier());
                searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::CaseInsensitive, Utility::Normalize(productCode)));
            }

            if (!searchRequest.Inclusions.empty())
            {
                auto searchResult = source.Search(searchRequest);

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
        
        // In case there are same Ids from different sources, filter the result using package name
        for (const auto& localization : manifest.Localizations)
        {
            const auto& localizedPackageName = localization.Get<Manifest::Localization::PackageName>();
            if (!localizedPackageName.empty())
            {
                searchRequest.Filters.emplace_back(PackageMatchField::Name, MatchType::CaseInsensitive, localizedPackageName);
            }
        }

        searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Name, MatchType::CaseInsensitive, manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>()));

        context.Add<Execution::Data::SearchResult>(source.Search(searchRequest));
    }

    void GetInstalledPackageVersion(Execution::Context& context)
    {
        context.Add<Execution::Data::InstalledPackageVersion>(context.Get<Execution::Data::Package>()->GetInstalledVersion());
    }

    void ReportExecutionStage::operator()(Execution::Context& context) const
    {
        context.SetExecutionStage(m_stage);
    }

    void ShowAppVersions(Execution::Context& context)
    {
        auto versions = context.Get<Execution::Data::Package>()->GetAvailableVersionKeys();

        Execution::TableOutput<2> table(context.Reporter, { Resource::String::ShowVersion, Resource::String::ShowChannel });
        for (const auto& version : versions)
        {
            table.OutputLine({ version.Version, version.Channel });
        }
        table.Complete();
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
