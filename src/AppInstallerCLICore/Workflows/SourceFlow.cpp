// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Resources.h"
#include "SourceFlow.h"
#include "PromptFlow.h"
#include "TableOutput.h"
#include "WorkflowBase.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::Settings;
    using namespace AppInstaller::Utility::literals;

    void GetSourceList(Execution::Context& context)
    {
        context.Add<Execution::Data::SourceList>(Repository::Source::GetCurrentSources());
    }

    void GetSourceListWithFilter(Execution::Context& context)
    {
        auto currentSources = Repository::Source::GetCurrentSources();
        if (context.Args.Contains(Args::Type::SourceName))
        {
            auto name = Utility::LocIndString{ context.Args.GetArg(Args::Type::SourceName) };

            for (auto const& source : currentSources)
            {
                if (Utility::ICUCaseInsensitiveEquals(source.Name, name))
                {
                    std::vector<Repository::SourceDetails> sources;
                    sources.emplace_back(source);
                    context.Add<Execution::Data::SourceList>(std::move(sources));
                    return;
                }
            }

            context.Reporter.Error() << Resource::String::SourceListNoneFound(name) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST);
        }
        else
        {
            context.Add<Execution::Data::SourceList>(std::move(currentSources));
        }
    }

    void CheckSourceListAgainstAdd(Execution::Context& context)
    {
        auto sourceList = context.Get<Execution::Data::SourceList>();
        std::string_view name = context.Args.GetArg(Args::Type::SourceName);
        std::string_view arg = context.Args.GetArg(Args::Type::SourceArg);
        std::string_view type = context.Args.GetArg(Args::Type::SourceType);

        // In the absence of a specified type, the default is Microsoft.PreIndexed.Package for comparison.
        // The default type assignment to the source takes place during the add operation (Source::Add in Repository.cpp).
        // This is necessary for the comparison to function correctly; otherwise, it would allow the addition of multiple
        // sources with different names but the same argument for all default type cases.
        // For example, the following commands would be allowed, but they acts as different alias to same source:
        //      winget source add "mysource1" "https:\\mysource" --trust - level trusted
        //      winget source add "mysource2" "https:\\mysource" --trust - level trusted
        if (type.empty())
        {
            type = Repository::Source::GetDefaultSourceType();
        }

        for (const auto& details : sourceList)
        {
            if (Utility::ICUCaseInsensitiveEquals(details.Name, name))
            {
                if (details.Arg == arg)
                {
                    // Name and arg match, indicate this to the user and bail.
                    context.Reporter.Info() << Resource::String::SourceAddAlreadyExistsMatch << std::endl <<
                        "  "_liv << details.Name << " -> "_liv << details.Arg << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS);
                }
                else
                {
                    context.Reporter.Error() << Resource::String::SourceAddAlreadyExistsDifferentArg << std::endl <<
                        "  "_liv << details.Name << " -> "_liv << details.Arg << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS);
                }
            }

            if (!details.Arg.empty() && details.Arg == arg && details.Type == type)
            {
                context.Reporter.Error() << Resource::String::SourceAddAlreadyExistsDifferentName << std::endl <<
                    "  "_liv << details.Name << " -> "_liv << details.Arg << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_ARG_ALREADY_EXISTS);
            }
        }
    }

    void AddSource(Execution::Context& context)
    {
        auto& sourceToAdd = context.Get<Execution::Data::Source>();
        auto details = sourceToAdd.GetDetails();

        context.Reporter.Info() <<
            Resource::String::SourceAddBegin << std::endl <<
            "  "_liv << details.Name << " -> "_liv << details.Arg << std::endl;

        auto addFunction = [&](IProgressCallback& progress)->bool { return sourceToAdd.Add(progress); };
        if (!context.Reporter.ExecuteWithProgress(addFunction))
        {
            context.Reporter.Info() << Resource::String::Cancelled << std::endl;
        }
        else
        {
            context.Reporter.Info() << Resource::String::Done << std::endl;
        }
    }

    void CreateSourceForSourceAdd(Execution::Context& context)
    {
        try
        {
            std::string_view name = context.Args.GetArg(Args::Type::SourceName);
            std::string_view arg = context.Args.GetArg(Args::Type::SourceArg);
            std::string_view type = context.Args.GetArg(Args::Type::SourceType);
            bool isExplicit = context.Args.Contains(Args::Type::SourceExplicit);

            Repository::SourceTrustLevel trustLevel = Repository::SourceTrustLevel::None;
            if (context.Args.Contains(Execution::Args::Type::SourceTrustLevel))
            {
                std::vector<std::string> trustLevelArgs = Utility::Split(std::string{ context.Args.GetArg(Execution::Args::Type::SourceTrustLevel) }, '|', true);
                trustLevel = Repository::ConvertToSourceTrustLevelFlag(trustLevelArgs);
            }

            Repository::Source sourceToAdd{ name, arg, type, trustLevel, isExplicit};

            if (context.Args.Contains(Execution::Args::Type::CustomHeader))
            {
                std::string customHeader{ context.Args.GetArg(Execution::Args::Type::CustomHeader) };
                if (!sourceToAdd.SetCustomHeader(customHeader))
                {
                    context.Reporter.Warn() << Resource::String::HeaderArgumentNotApplicableForNonRestSourceWarning << std::endl;
                }
            }

            if (sourceToAdd.GetInformation().Authentication.Type == Authentication::AuthenticationType::Unknown)
            {
                context.Reporter.Error() << Resource::String::SourceAddFailedAuthenticationNotSupported << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED);
            }

            context << Workflow::HandleSourceAgreements(sourceToAdd);
            if (context.IsTerminated())
            {
                return;
            }

            context.Add<Execution::Data::Source>(std::move(sourceToAdd));
        }
        catch (...)
        {
            context.Reporter.Error() << Resource::String::SourceAddOpenSourceFailed << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_OPEN_FAILED);
        }
    }

    void ListSources(Execution::Context& context)
    {
        const std::vector<Repository::SourceDetails>& sources = context.Get<Data::SourceList>();

        if (context.Args.Contains(Args::Type::SourceName))
        {
            // If a source name was specified, list full details of the one and only source.
            const Repository::SourceDetails& source = sources[0];

            Execution::TableOutput<2> table(context.Reporter, { Resource::String::SourceListField, Resource::String::SourceListValue });

            table.OutputLine({ Resource::LocString(Resource::String::SourceListName), source.Name });
            table.OutputLine({ Resource::LocString(Resource::String::SourceListType), source.Type });
            table.OutputLine({ Resource::LocString(Resource::String::SourceListArg), source.Arg });
            table.OutputLine({ Resource::LocString(Resource::String::SourceListData), source.Data });
            table.OutputLine({ Resource::LocString(Resource::String::SourceListIdentifier), source.Identifier });
            table.OutputLine({ Resource::LocString(Resource::String::SourceListTrustLevel), Repository::GetSourceTrustLevelForDisplay(source.TrustLevel)});
            table.OutputLine({ Resource::LocString(Resource::String::SourceListExplicit), std::string{ Utility::ConvertBoolToString(source.Explicit) }});

            if (source.LastUpdateTime == Utility::ConvertUnixEpochToSystemClock(0))
            {
                table.OutputLine({
                    Resource::LocString(Resource::String::SourceListUpdated),
                    Resource::LocString(Resource::String::SourceListUpdatedNever)
                    });
            }
            else
            {
                std::ostringstream strstr;
                strstr << source.LastUpdateTime;
                table.OutputLine({ Resource::LocString(Resource::String::SourceListUpdated), strstr.str() });
            }

            table.Complete();
        }
        else
        {
            if (sources.empty())
            {
                context.Reporter.Info() << Resource::String::SourceListNoSources << std::endl;
            }
            else
            {
                Execution::TableOutput<3> table(context.Reporter, { Resource::String::SourceListName, Resource::String::SourceListArg, Resource::String::SourceListExplicit });
                for (const auto& source : sources)
                {
                    table.OutputLine({ source.Name, source.Arg, std::string{ Utility::ConvertBoolToString(source.Explicit) }});
                }
                table.Complete();
            }
        }
    }

    void UpdateSources(Execution::Context& context)
    {
        if (!context.Args.Contains(Args::Type::SourceName))
        {
            context.Reporter.Info() << Resource::String::SourceUpdateAll << std::endl;
        }

        const std::vector<Repository::SourceDetails>& sources = context.Get<Data::SourceList>();

        for (const auto& sd : sources)
        {
            Repository::Source source{ sd.Name };
            context.Reporter.Info() << Resource::String::SourceUpdateOne(Utility::LocIndView{ sd.Name }) << std::endl;
            auto updateFunction = [&](IProgressCallback& progress)->std::vector<Repository::SourceDetails> { return source.Update(progress); };
            auto sourceDetails = context.Reporter.ExecuteWithProgress(updateFunction);
            if (!sourceDetails.empty())
            {
                if (std::chrono::system_clock::now() < sourceDetails[0].DoNotUpdateBefore)
                {
                    context.Reporter.Warn() << Resource::String::Unavailable << std::endl;
                }
                else
                {
                    context.Reporter.Info() << Resource::String::Cancelled << std::endl;
                }
            }
            else
            {
                context.Reporter.Info() << Resource::String::Done << std::endl;
            }
        }
    }

    void RemoveSources(Execution::Context& context)
    {
        // TODO: We currently only allow removing a single source. If that changes,
        //       we need to check all sources with the Group Policy before removing any of them.
        if (!context.Args.Contains(Args::Type::SourceName))
        {
            context.Reporter.Info() << Resource::String::SourceRemoveAll << std::endl;
        }

        const std::vector<Repository::SourceDetails>& sources = context.Get<Data::SourceList>();
        for (const auto& sd : sources)
        {
            Repository::Source source{ sd.Name };
            context.Reporter.Info() << Resource::String::SourceRemoveOne(Utility::LocIndView{ sd.Name }) << std::endl;
            auto removeFunction = [&](IProgressCallback& progress)->bool { return source.Remove(progress); };
            if (context.Reporter.ExecuteWithProgress(removeFunction))
            {
                context.Reporter.Info() << Resource::String::Done << std::endl;
            }
            else
            {
                context.Reporter.Info() << Resource::String::Cancelled << std::endl;
            }
        }
    }

    void QueryUserForSourceReset(Execution::Context& context)
    {
        if (!context.Args.Contains(Execution::Args::Type::ForceSourceReset))
        {
            context << GetSourceListWithFilter;
            const std::vector<Repository::SourceDetails>& sources = context.Get<Data::SourceList>();

            if (!sources.empty())
            {
                context.Reporter.Info() << Resource::String::SourceResetListAndOverridePreamble << std::endl;

                context << ListSources;
                AICLI_TERMINATE_CONTEXT(E_ABORT);
            }
        }
    }

    void ResetSourceList(Execution::Context& context)
    {
        const std::vector<Repository::SourceDetails>& sources = context.Get<Data::SourceList>();

        for (const auto& source : sources)
        {
            context.Reporter.Info() << Resource::String::SourceResetOne(Utility::LocIndView{ source.Name });
            Repository::Source::DropSource(source.Name);
            context.Reporter.Info() << Resource::String::Done << std::endl;
        }
    }

    void ResetAllSources(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::SourceResetAll;
        Repository::Source::DropSource({});
        context.Reporter.Info() << Resource::String::Done << std::endl;
    }

    void ExportSourceList(Execution::Context& context)
    {
        const std::vector<Repository::SourceDetails>& sources = context.Get<Data::SourceList>();

        if (sources.empty())
        {
            context.Reporter.Info() << Resource::String::SourceListNoSources << std::endl;
        }
        else
        {
            for (const auto& source : sources)
            {
                SourceFromPolicy s;
                s.Name = source.Name;
                s.Type = source.Type;
                s.Arg = source.Arg;
                s.Data = source.Data;
                s.Identifier = source.Identifier;

                std::vector<std::string_view> sourceTrustLevels = Repository::SourceTrustLevelFlagToList(source.TrustLevel);
                s.TrustLevel = std::vector<std::string>(sourceTrustLevels.begin(), sourceTrustLevels.end());
                s.Explicit = source.Explicit;
                context.Reporter.Info() << s.ToJsonString() << std::endl;
            }
        }
    }

    void ForceInstalledCacheUpdate(Execution::Context&)
    {
        // Creating this object is currently sufficient to mark the cache as needing an update for the next time it is opened.
        Repository::Source ignore{ Repository::PredefinedSource::InstalledForceCacheUpdate };
    }
}
