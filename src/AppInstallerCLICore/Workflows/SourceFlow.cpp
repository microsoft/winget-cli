// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Resources.h"
#include "SourceFlow.h"
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
        if (context.Args.Contains(Args::Type::SourceName))
        {
            std::string_view name = context.Args.GetArg(Args::Type::SourceName);
            Repository::Source source{ name };

            if (!source)
            {
                context.Reporter.Error() << Resource::String::SourceListNoneFound << ' ' << name << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST);
            }

            std::vector<Repository::SourceDetails> sources;
            sources.emplace_back(source.GetDetails());
            context.Add<Execution::Data::SourceList>(std::move(sources));
        }
        else
        {
            context.Add<Execution::Data::SourceList>(Repository::Source::GetCurrentSources());
        }
    }

    void CheckSourceListAgainstAdd(Execution::Context& context)
    {
        auto sourceList = context.Get<Execution::Data::SourceList>();
        std::string_view name = context.Args.GetArg(Args::Type::SourceName);
        std::string_view arg = context.Args.GetArg(Args::Type::SourceArg);
        std::string_view type = context.Args.GetArg(Args::Type::SourceType);

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

        if (!context.Reporter.ExecuteWithProgress(std::bind(&Repository::Source::Add, &sourceToAdd, std::placeholders::_1)))
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

            Repository::Source sourceToAdd{ name, arg, type };

            if (context.Args.Contains(Execution::Args::Type::CustomHeader))
            {
                std::string customHeader{ context.Args.GetArg(Execution::Args::Type::CustomHeader) };
                if (!sourceToAdd.SetCustomHeader(customHeader))
                {
                    context.Reporter.Warn() << Resource::String::HeaderArgumentNotApplicableForNonRestSourceWarning << std::endl;
                }
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

            table.OutputLine({ Resource::Loader::Instance().ResolveString(Resource::String::SourceListName), source.Name });
            table.OutputLine({ Resource::Loader::Instance().ResolveString(Resource::String::SourceListType), source.Type });
            table.OutputLine({ Resource::Loader::Instance().ResolveString(Resource::String::SourceListArg), source.Arg });
            table.OutputLine({ Resource::Loader::Instance().ResolveString(Resource::String::SourceListData), source.Data });
            table.OutputLine({ Resource::Loader::Instance().ResolveString(Resource::String::SourceListIdentifier), source.Identifier });

            if (source.LastUpdateTime == Utility::ConvertUnixEpochToSystemClock(0))
            {
                table.OutputLine({
                    Resource::Loader::Instance().ResolveString(Resource::String::SourceListUpdated),
                    Resource::Loader::Instance().ResolveString(Resource::String::SourceListUpdatedNever)
                    });
            }
            else
            {
                std::ostringstream strstr;
                strstr << source.LastUpdateTime;
                table.OutputLine({ Resource::Loader::Instance().ResolveString(Resource::String::SourceListUpdated), strstr.str() });
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
                Execution::TableOutput<2> table(context.Reporter, { Resource::String::SourceListName, Resource::String::SourceListArg });
                for (const auto& source : sources)
                {
                    table.OutputLine({ source.Name, source.Arg });
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
            context.Reporter.Info() << Resource::String::SourceUpdateOne << ' ' << sd.Name << "..."_liv << std::endl;
            if (!context.Reporter.ExecuteWithProgress(std::bind(&Repository::Source::Update, &source, std::placeholders::_1)).empty())
            {
                context.Reporter.Info() << Resource::String::Cancelled << std::endl;
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
            context.Reporter.Info() << Resource::String::SourceRemoveOne << ' ' << sd.Name << "..."_liv << std::endl;
            if (context.Reporter.ExecuteWithProgress(std::bind(&Repository::Source::Remove, &source, std::placeholders::_1)))
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

        for (const auto& sd : sources)
        {
            context.Reporter.Info() << Resource::String::SourceResetOne << ' ' << sd.Name << "..."_liv;
            Repository::Source source{ sd.Name };
            source.Drop();
            context.Reporter.Info() << Resource::String::Done << std::endl;
        }
    }

    void ResetAllSources(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::SourceResetAll;
        Repository::Source source{ ""sv };
        source.Drop();
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
                context.Reporter.Info() << s.ToJsonString() << std::endl;
            }
        }
    }
}
