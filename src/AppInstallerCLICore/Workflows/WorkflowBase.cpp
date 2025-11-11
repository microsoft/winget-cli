// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowBase.h"
#include "ExecutionContext.h"
#include <winget/ManifestComparator.h>
#include "PromptFlow.h"
#include "Sixel.h"
#include "TableOutput.h"
#include <winget/FileCache.h>
#include <winget/ExperimentalFeature.h>
#include <winget/ManifestYamlParser.h>
#include <winget/Pin.h>
#include <winget/PinningData.h>
#include <AppInstallerSHA256.h>
#include <winget/Runtime.h>
#include <winget/PackageVersionSelection.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

using namespace std::string_literals;
using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::Pinning;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;
using namespace winrt::Windows::Foundation;

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

        // Determines icon fit given two options.
        // Targets an 80x80 icon as the best resolution for this use case.
        // TODO: Consider theme based on current background color.
        bool IsSecondIconBetter(const Manifest::Icon& current, const Manifest::Icon& alternative)
        {
            static constexpr std::array<uint8_t, ToIntegral(Manifest::IconResolutionEnum::Square256) + 1> s_iconResolutionOrder
            {
                9, // Unknown
                8, // Custom
                15, // Square16
                14, // Square20
                13, // Square24
                12, // Square30
                11, // Square32
                10, // Square36
                6, // Square40
                5, // Square48
                4, // Square60
                3, // Square64
                2, // Square72
                0, // Square80
                1, // Square96
                7, // Square256
            };

            return s_iconResolutionOrder.at(ToIntegral(alternative.Resolution)) < s_iconResolutionOrder.at(ToIntegral(current.Resolution));
        }

        void ShowManifestIcon(Execution::Context& context, const Manifest::Manifest& manifest) try
        {
            if (!context.Reporter.SixelsEnabled())
            {
                return;
            }

            auto icons = manifest.CurrentLocalization.Get<Manifest::Localization::Icons>();
            const Manifest::Icon* bestFitIcon = nullptr;

            for (const auto& icon : icons)
            {
                if (!bestFitIcon || IsSecondIconBetter(*bestFitIcon, icon))
                {
                    bestFitIcon = &icon;
                }
            }

            if (!bestFitIcon)
            {
                return;
            }

            // Use a cache to hold the icons
            auto splitUri = Utility::SplitFileNameFromURI(bestFitIcon->Url);
            Caching::FileCache fileCache{ Caching::FileCache::Type::Icon, Utility::SHA256::ConvertToString(bestFitIcon->Sha256), { splitUri.first } };
            auto iconStream = fileCache.GetFile(splitUri.second, bestFitIcon->Sha256);

            VirtualTerminal::Sixel::Image sixelIcon{ *iconStream, bestFitIcon->FileType };

            // Using a height of 4 arbitrarily; allow width up to the entire console.
            UINT imageHeightCells = 4;
            UINT imageWidthCells = static_cast<UINT>(Execution::GetConsoleWidth());

            sixelIcon.RenderSizeInCells(imageWidthCells, imageHeightCells);
            auto infoOut = context.Reporter.Info();
            sixelIcon.RenderTo(infoOut);

            // Force the final sixel line to not be overwritten
            infoOut << std::endl;
        }
        CATCH_LOG();

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

                auto openFunction = [&](IProgressCallback& progress)->std::vector<Repository::SourceDetails>
                {
                    source.SetCaller("winget-cli");
                    source.SetAuthenticationArguments(GetAuthenticationArguments(context));
                    source.SetThreadGlobals(context.GetSharedThreadGlobals());
                    return source.Open(progress);
                };
                auto updateFailures = context.Reporter.ExecuteWithProgress(openFunction, true);

                // We'll only report the source update failure as warning and continue
                for (const auto& s : updateFailures)
                {
                    context.Reporter.Warn() << Resource::String::SourceOpenWithFailedUpdate(Utility::LocIndView{ s.Name }) << std::endl;
                }

                // Report sources that may need authentication
                if (source.IsComposite())
                {
                    for (const auto& s : source.GetAvailableSources())
                    {
                        if (s.GetInformation().Authentication.Type != Authentication::AuthenticationType::None)
                        {
                            context.Reporter.Info() << Execution::AuthenticationEmphasis << Resource::String::SourceRequiresAuthentication(Utility::LocIndView{ s.GetDetails().Name }) << std::endl;
                        }
                    }
                }
                else if (source.GetInformation().Authentication.Type != Authentication::AuthenticationType::None)
                {
                    context.Reporter.Info() << Execution::AuthenticationEmphasis << Resource::String::SourceRequiresAuthentication(Utility::LocIndView{ source.GetDetails().Name }) << std::endl;
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

    void WorkflowTask::Log() const
    {
        if (m_isFunc)
        {
            // Using `00000001`80000000` as base address default when loading dll into windbg as dump file.
            AICLI_LOG(Workflow, Verbose, << "Running task: 0x" << m_func << " [ln 00000001`80000000+" << std::hex << (reinterpret_cast<char*>(m_func) - reinterpret_cast<char*>(&__ImageBase)) << "]");
        }
        else
        {
            AICLI_LOG(Workflow, Verbose, << "Running task: " << m_name);
        }
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

    Authentication::AuthenticationArguments GetAuthenticationArguments(const Execution::Context& context)
    {
        AppInstaller::Authentication::AuthenticationArguments authArgs;

        if (context.Args.Contains(Execution::Args::Type::AuthenticationMode))
        {
            authArgs.Mode = Authentication::ConvertToAuthenticationMode(context.Args.GetArg(Execution::Args::Type::AuthenticationMode));
        }
        else
        {
            // If user did not specify authentication mode, determine based on if disable interactivity flag exists.
            authArgs.Mode = context.Args.Contains(Execution::Args::Type::DisableInteractivity) ? Authentication::AuthenticationMode::Silent : Authentication::AuthenticationMode::SilentPreferred;
        }

        if (context.Args.Contains(Execution::Args::Type::AuthenticationAccount))
        {
            authArgs.AuthenticationAccount = context.Args.GetArg(Execution::Args::Type::AuthenticationAccount);
        }

        AICLI_LOG(CLI, Info, << "Created authentication arguments. Mode: " << Authentication::AuthenticationModeToString(authArgs.Mode) << ", Account: " << authArgs.AuthenticationAccount);

        return authArgs;
    }

    HRESULT HandleException(Execution::Context* context, std::exception_ptr exception)
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
            if (context)
            {
                context->Reporter.Error() <<
                    Resource::String::UnexpectedErrorExecutingCommand << ' ' << std::endl <<
                    GetUserPresentableMessage(re) << std::endl;
            }
            return re.GetErrorCode();
        }
        catch (const winrt::hresult_error& hre)
        {
            std::string message = GetUserPresentableMessage(hre);
            Logging::Telemetry().LogException(Logging::FailureTypeEnum::WinrtHResultError, message);
            if (context)
            {
                context->Reporter.Error() <<
                    Resource::String::UnexpectedErrorExecutingCommand << ' ' << std::endl <<
                    message << std::endl;
            }
            return hre.code();
        }
        catch (const Settings::GroupPolicyException& e)
        {
            if (context)
            {
                auto policy = Settings::TogglePolicy::GetPolicy(e.Policy());
                auto policyNameId = policy.PolicyName();
                context->Reporter.Error() << Resource::String::DisabledByGroupPolicy(policyNameId) << std::endl;
            }
            return APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY;
        }
        catch (const std::exception& e)
        {
            Logging::Telemetry().LogException(Logging::FailureTypeEnum::StdException, e.what());
            if (context)
            {
                context->Reporter.Error() <<
                    Resource::String::UnexpectedErrorExecutingCommand << ' ' << std::endl <<
                    GetUserPresentableMessage(e) << std::endl;
            }
            return APPINSTALLER_CLI_ERROR_COMMAND_FAILED;
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            Logging::Telemetry().LogException(Logging::FailureTypeEnum::Unknown, {});
            if (context)
            {
                context->Reporter.Error() <<
                    Resource::String::UnexpectedErrorExecutingCommand << " ???"_liv << std::endl;
            }
            return APPINSTALLER_CLI_ERROR_COMMAND_FAILED;
        }

        return E_UNEXPECTED;
    }

    HRESULT HandleException(Execution::Context& context, std::exception_ptr exception)
    {
        return HandleException(&context, exception);
    }

    AppInstaller::Manifest::ManifestComparator::Options GetManifestComparatorOptions(const Execution::Context& context, const IPackageVersion::Metadata& metadata)
    {
        AppInstaller::Manifest::ManifestComparator::Options options;
        bool getAllowedArchitecturesFromMetadata = false;

        if (context.Contains(Execution::Data::AllowedArchitectures))
        {
            // Com caller can directly set allowed architectures
            options.AllowedArchitectures = context.Get<Execution::Data::AllowedArchitectures>();
        }
        else if (context.Args.Contains(Execution::Args::Type::InstallArchitecture))
        {
            // Arguments provided in command line
            options.AllowedArchitectures.emplace_back(Utility::ConvertToArchitectureEnum(context.Args.GetArg(Execution::Args::Type::InstallArchitecture)));
        }
        else if (context.Args.Contains(Execution::Args::Type::InstallerArchitecture))
        {
            // Arguments provided in command line. Also skips applicability check.
            options.AllowedArchitectures.emplace_back(Utility::ConvertToArchitectureEnum(context.Args.GetArg(Execution::Args::Type::InstallerArchitecture)));
            options.SkipApplicabilityCheck = true;
        }
        else
        {
            getAllowedArchitecturesFromMetadata = true;
        }

        if (context.Args.Contains(Execution::Args::Type::InstallerType))
        {
            options.RequestedInstallerType = Manifest::ConvertToInstallerTypeEnum(std::string(context.Args.GetArg(Execution::Args::Type::InstallerType)));
        }

        if (context.Args.Contains(Execution::Args::Type::InstallScope))
        {
            options.RequestedInstallerScope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        }

        if (context.Contains(Execution::Data::AllowUnknownScope))
        {
            options.AllowUnknownScope = context.Get<Execution::Data::AllowUnknownScope>();
        }

        if (context.Args.Contains(Execution::Args::Type::Locale))
        {
            options.RequestedInstallerLocale = context.Args.GetArg(Execution::Args::Type::Locale);
        }

        Repository::GetManifestComparatorOptionsFromMetadata(options, metadata, getAllowedArchitecturesFromMetadata);

        return options;
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

            auto openFunction = [&](IProgressCallback& progress)->std::vector<Repository::SourceDetails>
            {
                return source.Open(progress);
            };
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
            auto latestVersion = GetAllAvailableVersions(searchResult.Matches[i].Package)->GetLatestVersion();

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
            auto available = package->GetAvailable();
            if (!available.empty())
            {
                auto source = available[0]->GetSource();
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
        std::vector<InstalledPackagesTableLine> linesForPins;

        int availableUpgradesCount = 0;

        // We will show a line with a summary for skipped and pinned packages at the end.
        // The strings suggest using a --include-unknown/pinned argument, so we should
        // ensure that the count is 0 when using the arguments.
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

        PinningData pinningData{ PinningData::Disposition::ReadOnly };

        for (const auto& match : searchResult.Matches)
        {
            auto installedPackage = match.Package->GetInstalled();
            if (!installedPackage)
            {
                continue;
            }

            // We only want to evaluate update availability for the latest version.
            bool isFirstInstalledVersion = true;

            for (const auto& installedVersionKey : installedPackage->GetVersionKeys())
            {
                bool isFirstInstalledVersionLocal = isFirstInstalledVersion;
                isFirstInstalledVersion = false;

                auto installedVersion = installedPackage->GetVersion(installedVersionKey);

                auto evaluator = pinningData.CreatePinStateEvaluator(pinBehavior, installedVersion);
                auto availableVersions = GetAvailableVersionsForInstalledVersion(match.Package, installedVersion);

                auto latestVersion = evaluator.GetLatestAvailableVersionForPins(availableVersions);
                bool updateAvailable = isFirstInstalledVersionLocal && evaluator.IsUpdate(latestVersion);
                bool updateIsPinned = false;

                if (m_onlyShowUpgrades && !context.Args.Contains(Execution::Args::Type::IncludeUnknown) && Utility::Version(installedVersion->GetProperty(PackageVersionProperty::Version)).IsUnknown() && updateAvailable)
                {
                    // We are only showing upgrades, and the user did not request to include packages with unknown versions.
                    ++packagesWithUnknownVersionSkipped;
                    continue;
                }

                if (m_onlyShowUpgrades && !updateAvailable && isFirstInstalledVersionLocal)
                {
                    // Reuse the evaluator to check if there is an update outside of the pinning
                    auto unpinnedLatestVersion = availableVersions->GetLatestVersion();
                    bool updateAvailableWithoutPins = evaluator.IsUpdate(unpinnedLatestVersion);

                    if (updateAvailableWithoutPins)
                    {
                        // When given the --include-pinned argument, report blocking and gating pins in a separate table.
                        // Otherwise, simply show a count of them
                        if (context.Args.Contains(Execution::Args::Type::IncludePinned))
                        {
                            updateIsPinned = true;

                            // Override these so we generate the table line below.
                            latestVersion = std::move(unpinnedLatestVersion);
                            updateAvailable = true;
                        }
                        else
                        {
                            ++packagesWithUserPinsSkipped;
                            continue;
                        }
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
                    if (updateIsPinned)
                    {
                        linesForPins.push_back(std::move(line));
                    }
                    else if (m_onlyShowUpgrades && pinnedState == PinType::PinnedByManifest)
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

        if (!linesForPins.empty())
        {
            context.Reporter.Info() << std::endl << Resource::String::UpgradeBlockedByPinCount(linesForPins.size()) << std::endl;
            OutputInstalledPackagesTable(context, linesForPins);
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
            
            switch (m_operationType)
            {
                // These search purposes require a package to be found in the Installed Packages
                case OperationType::Export:
                case OperationType::List:
                case OperationType::Uninstall:
                case OperationType::Pin:
                case OperationType::Upgrade:
                case OperationType::Repair:
                    context.Reporter.Info() << Resource::String::NoInstalledPackageFound << std::endl;
                    break;
                case OperationType::Completion:
                case OperationType::Install:
                case OperationType::Search:
                case OperationType::Show:
                case OperationType::Download:
                default:
                    context.Reporter.Info() << Resource::String::NoPackageFound << std::endl;
                    break;
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
        }
    }

    void EnsureOneMatchFromSearchResult::operator()(Execution::Context& context) const
    {
        context <<
            EnsureMatchesFromSearchResult(m_operationType);

        if (!context.IsTerminated())
        {
            auto& searchResult = context.Get<Execution::Data::SearchResult>();

            if (searchResult.Matches.size() > 1)
            {
                Logging::Telemetry().LogMultiAppMatch();

                if (m_operationType == OperationType::Upgrade || m_operationType == OperationType::Uninstall || m_operationType == OperationType::Repair || m_operationType == OperationType::Export)
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

            std::shared_ptr<ICompositePackage> package = searchResult.Matches.at(0).Package;
            Logging::Telemetry().LogAppFound(package->GetProperty(PackageProperty::Name), package->GetProperty(PackageProperty::Id));

            context.Add<Execution::Data::Package>(std::move(package));
        }
    }

    void GetManifestWithVersionFromPackage::operator()(Execution::Context& context) const
    {
        PackageVersionKey key("", m_version, m_channel);

        std::shared_ptr<ICompositePackage> package = context.Get<Execution::Data::Package>();
        std::shared_ptr<IPackageVersion> requestedVersion;
        auto availableVersions = GetAvailableVersionsForInstalledVersion(package);

        if (m_considerPins)
        {
            bool isPinned = false;

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

            PinningData pinningData{ PinningData::Disposition::ReadOnly };
            auto evaluator = pinningData.CreatePinStateEvaluator(pinBehavior, GetInstalledVersion(package));

            // TODO: The logic here will probably have to get more difficult once we support channels
            if (Utility::IsEmptyOrWhitespace(m_version) && Utility::IsEmptyOrWhitespace(m_channel))
            {
                requestedVersion = evaluator.GetLatestAvailableVersionForPins(availableVersions);

                if (!requestedVersion)
                {
                    // Check whether we didn't find the latest version because it was pinned or because there wasn't one
                    auto latestVersion = availableVersions->GetLatestVersion();
                    if (latestVersion)
                    {
                        isPinned = true;
                    }
                }
            }
            else
            {
                requestedVersion = availableVersions->GetVersion(key);
                isPinned = evaluator.EvaluatePinType(requestedVersion) != PinType::Unknown;
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
            requestedVersion = availableVersions->GetVersion(key);
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

    void VerifyFileOrUri::operator()(Execution::Context& context) const
    {
        // Argument requirement is handled elsewhere.
        if (!context.Args.Contains(m_arg))
        {
            return;
        }

        auto path = context.Args.GetArg(m_arg);

        // try uri first
        Uri pathAsUri = nullptr;
        try
        {
            pathAsUri = Uri{ Utility::ConvertToUTF16(path) };
        }
        catch (...) {}

        if (pathAsUri)
        {
            if (pathAsUri.Suspicious())
            {
                context.Reporter.Error() << Resource::String::UriNotWellFormed(Utility::LocIndView{ path }) << std::endl;
                AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
            }
            // SchemeName() always returns lower case
            else if (L"file" == pathAsUri.SchemeName() && !Utility::CaseInsensitiveStartsWith(path, "file:"))
            {
                // Uri constructor is smart enough to parse an absolute local file path to file uri.
                // In this case, we should continue with VerifyFile.
                context << VerifyFile(m_arg);
            }
            else if (std::find(m_supportedSchemes.begin(), m_supportedSchemes.end(), pathAsUri.SchemeName()) != m_supportedSchemes.end())
            {
                // Scheme supported.
                return;
            }
            else
            {
                context.Reporter.Error() << Resource::String::UriSchemeNotSupported(Utility::LocIndView{ path }) << std::endl;
                AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
            }
        }
        else
        {
            context << VerifyFile(m_arg);
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

    void ReportInstalledPackageVersionIdentity(Execution::Context& context)
    {
        auto package = context.Get<Execution::Data::Package>();
        auto version = context.Get<Execution::Data::InstalledPackageVersion>();
        ReportIdentity(context, {}, Resource::String::ReportIdentityFound, version->GetProperty(PackageVersionProperty::Name), package ? package->GetProperty(PackageProperty::Id) : version->GetProperty(PackageVersionProperty::Id));
    }

    void ReportManifestIdentity(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        ReportIdentity(context, {}, Resource::String::ReportIdentityFound, manifest.CurrentLocalization.Get<Manifest::Localization::PackageName>(), manifest.Id);
        ShowManifestIcon(context, manifest);
    }

    void ReportManifestIdentityWithVersion::operator()(Execution::Context& context) const
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        ReportIdentity(context, m_prefix, m_label, manifest.CurrentLocalization.Get<Manifest::Localization::PackageName>(), manifest.Id, manifest.Version, m_level);
        ShowManifestIcon(context, manifest);
    }

    void SelectInstaller(Execution::Context& context)
    {
        bool isUpdate = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate);
        bool isRepair = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseRepair);

        IPackageVersion::Metadata installationMetadata;

        if (isUpdate || isRepair)
        {
            installationMetadata = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata();
        }

        Manifest::ManifestComparator manifestComparator(GetManifestComparatorOptions(context, installationMetadata));
        auto [installer, inapplicabilities] = manifestComparator.GetPreferredInstaller(context.Get<Execution::Data::Manifest>());

        if (!installer.has_value())
        {
            auto onlyInstalledType = std::find(inapplicabilities.begin(), inapplicabilities.end(), Manifest::InapplicabilityFlags::InstalledType);
            if (onlyInstalledType != inapplicabilities.end())
            {
                if (isRepair)
                {
                    context.Reporter.Info() << Resource::String::RepairDifferentInstallTechnology << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_REPAIR_NOT_APPLICABLE);
                }
                else
                {
                    context.Reporter.Info() << Resource::String::UpgradeDifferentInstallTechnology << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
                }
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
            else if (installer.EffectiveInstallerType() == Manifest::InstallerTypeEnum::Font)
            {
                // Font Packages match by Package Id first.
                searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::CaseInsensitive, manifest.Id));
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
        std::shared_ptr<IPackage> installed = context.Get<Execution::Data::Package>()->GetInstalled();

        if (installed)
        {
            // TODO: This may need to be expanded dramatically to enable targeting across a variety of dimensions (architecture, etc.)
            //       Alternatively, if we make it easier to see the fully unique package identifiers, we may avoid that need.
            if (context.Args.Contains(Execution::Args::Type::TargetVersion))
            {
                Repository::PackageVersionKey versionKey{ "", context.Args.GetArg(Execution::Args::Type::TargetVersion) , "" };
                std::shared_ptr<IPackageVersion> installedVersion = installed->GetVersion(versionKey);

                if (!installedVersion)
                {
                    context.Reporter.Error() << Resource::String::GetManifestResultVersionNotFound(Utility::LocIndView{ versionKey.Version }) << std::endl;
                    // This error maintains consistency with passing an available version to commands
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_MANIFEST_FOUND);
                }

                context.Add<Execution::Data::InstalledPackageVersion>(std::move(installedVersion));
            }
            else
            {
                context.Add<Execution::Data::InstalledPackageVersion>(installed->GetLatestVersion());
            }
        }
        else
        {
            context.Add<Execution::Data::InstalledPackageVersion>(nullptr);
        }
    }

    void ReportExecutionStage::operator()(Execution::Context& context) const
    {
        context.SetExecutionStage(m_stage);
    }

    void ShowAppVersions(Execution::Context& context)
    {
        auto versions = GetAllAvailableVersions(context.Get<Execution::Data::Package>())->GetVersionKeys();

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
    if (!context.IsTerminated() || task.ExecuteAlways())
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (context.ShouldExecuteWorkflowTask(task))
#endif
        {
            task.Log();
            task(context);
        }
    }
    return context;
}
