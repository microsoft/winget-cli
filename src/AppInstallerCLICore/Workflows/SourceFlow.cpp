// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Resources.h"
#include "SourceFlow.h"
#include "TableOutput.h"
#include "WorkflowBase.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::Utility::literals;

    void GetSourceList(Execution::Context& context)
    {
        context.Add<Execution::Data::SourceList>(Repository::GetSources());
    }

    void GetSourceListWithFilter(Execution::Context& context)
    {
        if (context.Args.Contains(Args::Type::SourceName))
        {
            std::string_view name = context.Args.GetArg(Args::Type::SourceName);
            std::optional<Repository::SourceDetails> source = Repository::GetSource(name);

            if (!source)
            {
                context.Reporter.Error() << Resource::String::SourceListNoneFound << ' ' << name << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST);
            }

            std::vector<Repository::SourceDetails> sources;
            sources.emplace_back(std::move(source.value()));
            context.Add<Execution::Data::SourceList>(std::move(sources));
        }
        else
        {
            context.Add<Execution::Data::SourceList>(Repository::GetSources());
        }
    }

    void CheckSourceListAgainstAdd(Execution::Context& context)
    {
        std::string_view name = context.Args.GetArg(Args::Type::SourceName);
        std::string_view arg = context.Args.GetArg(Args::Type::SourceArg);

        // First check if this is going to be a name conflict
        std::optional<Repository::SourceDetails> source = Repository::GetSource(name);
        if (source)
        {
            if (source->Arg == arg)
            {
                // Name and arg match, indicate this to the user and bail.
                context.Reporter.Info() << Resource::String::SourceAddAlreadyExistsMatch << std::endl <<
                    "  "_liv << source->Name << " -> "_liv << source->Arg << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS);
            }
            else
            {
                context.Reporter.Error() << Resource::String::SourceAddAlreadyExistsDifferentArg << std::endl <<
                    "  "_liv << source->Name << " -> "_liv << source->Arg << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS);
            }
        }

        // Now check if the URL is already in use under a different name
        auto sourceList = context.Get<Execution::Data::SourceList>();
        std::string_view type = context.Args.GetArg(Args::Type::SourceType);

        for (const auto& details : sourceList)
        {
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
        std::string name(context.Args.GetArg(Args::Type::SourceName));
        std::string arg(context.Args.GetArg(Args::Type::SourceArg));
        std::string type;
        if (context.Args.Contains(Args::Type::SourceType))
        {
            type = context.Args.GetArg(Args::Type::SourceType);
        }

        context.Reporter.Info() <<
            Resource::String::SourceAddBegin << std::endl <<
            "  "_liv << name << " -> "_liv << arg << std::endl;

        context.Reporter.ExecuteWithProgress(std::bind(Repository::AddSource, std::move(name), std::move(type), std::move(arg), std::placeholders::_1));

        context.Reporter.Info() << Resource::String::Done;
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
            context.Reporter.Info() << Resource::String::SourceUpdateOne << ' ' << sd.Name << "..."_liv << std::endl;
            context.Reporter.ExecuteWithProgress(std::bind(Repository::UpdateSource, sd.Name, std::placeholders::_1));
            context.Reporter.Info() << Resource::String::Done << std::endl;
        }
    }

    void RemoveSources(Execution::Context& context)
    {
        if (!context.Args.Contains(Args::Type::SourceName))
        {
            context.Reporter.Info() << Resource::String::SourceRemoveAll << std::endl;
        }

        const std::vector<Repository::SourceDetails>& sources = context.Get<Data::SourceList>();
        for (const auto& sd : sources)
        {
            context.Reporter.Info() << Resource::String::SourceRemoveOne << ' ' << sd.Name << "..."_liv << std::endl;
            context.Reporter.ExecuteWithProgress(std::bind(Repository::RemoveSource, sd.Name, std::placeholders::_1));
            context.Reporter.Info() << Resource::String::Done << std::endl;
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
            context.Reporter.Info() << Resource::String::SourceResetOne << ' ' << source.Name << "..."_liv;
            Repository::DropSource(source.Name);
            context.Reporter.Info() << Resource::String::Done << std::endl;
        }
    }

    void ResetAllSources(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::SourceResetAll;
        Repository::DropSource({});
        context.Reporter.Info() << Resource::String::Done << std::endl;
    }
}
