// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "SourceFlow.h"
#include "TableOutput.h"
#include "WorkflowBase.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::CLI::Execution;

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
                context.Reporter.Error() << "Did not find a source named: " << name << std::endl;
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
                context.Reporter.Info() << "A source with the given name already exists and refers to the same location: " << std::endl <<
                    "  " << source->Name << " -> " << source->Arg << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS);
            }
            else
            {
                context.Reporter.Error() << "A source with the given name already exists and refers to a different location: " << std::endl <<
                    "  " << source->Name << " -> " << source->Arg << std::endl;
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
                context.Reporter.Error() << "A source with a different name already refers to this location: " << std::endl <<
                    "  " << details.Name << " -> " << details.Arg << std::endl;
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
            "Adding source:" << std::endl <<
            "  " << name << " -> " << arg << std::endl;

        context.Reporter.ExecuteWithProgress(std::bind(Repository::AddSource, std::move(name), std::move(type), std::move(arg), std::placeholders::_1));

        context.Reporter.Info() << "Done";
    }

    void ListSources(Execution::Context& context)
    {
        const std::vector<Repository::SourceDetails>& sources = context.Get<Data::SourceList>();

        if (context.Args.Contains(Args::Type::SourceName))
        {
            // If a source name was specified, list full details of the one and only source.
            const Repository::SourceDetails& source = sources[0];

            context.Reporter.Info() <<
                "Name   : " + source.Name << std::endl <<
                "Type   : " + source.Type << std::endl <<
                "Arg    : " + source.Arg << std::endl <<
                "Data   : " + source.Data << std::endl <<
                "Updated: ";

            if (source.LastUpdateTime == Utility::ConvertUnixEpochToSystemClock(0))
            {
                context.Reporter.Info() << "<never>" << std::endl;
            }
            else
            {
                context.Reporter.Info() << source.LastUpdateTime << std::endl;
            }
        }
        else
        {

            if (sources.empty())
            {
                context.Reporter.Info() << "There are no sources configured." << std::endl;
            }
            else
            {
                Execution::TableOutput<2> table(context.Reporter, { "Name", "Arg" });
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
            context.Reporter.Info() << "Updating all sources..." << std::endl;
        }

        const std::vector<Repository::SourceDetails>& sources = context.Get<Data::SourceList>();
        for (const auto& sd : sources)
        {
            context.Reporter.Info() << "Updating source: " << sd.Name << "..." << std::endl;
            context.Reporter.ExecuteWithProgress(std::bind(Repository::UpdateSource, sd.Name, std::placeholders::_1));
            context.Reporter.Info() << "Done." << std::endl;
        }
    }

    void RemoveSources(Execution::Context& context)
    {
        if (!context.Args.Contains(Args::Type::SourceName))
        {
            context.Reporter.Info() << "Removing all sources..." << std::endl;
        }

        const std::vector<Repository::SourceDetails>& sources = context.Get<Data::SourceList>();
        for (const auto& sd : sources)
        {
            context.Reporter.Info() << "Removing source: " << sd.Name << "..." << std::endl;
            context.Reporter.ExecuteWithProgress(std::bind(Repository::RemoveSource, sd.Name, std::placeholders::_1));
            context.Reporter.Info() << "Done." << std::endl;
        }
    }

    void QueryUserForSourceReset(Execution::Context& context)
    {
        if (!context.Args.Contains(Execution::Args::Type::Force))
        {
            context << GetSourceListWithFilter;
            const std::vector<Repository::SourceDetails>& sources = context.Get<Data::SourceList>();

            if (!sources.empty())
            {
                context.Reporter.Info() << "The following sources will be reset:" << std::endl;

                context << ListSources;

                if (!context.Reporter.PromptForBoolResponse("Do you wish to continue?"))
                {
                    AICLI_TERMINATE_CONTEXT(E_ABORT);
                }
            }
        }
    }

    void ResetSourceList(Execution::Context& context)
    {
        const std::vector<Repository::SourceDetails>& sources = context.Get<Data::SourceList>();

        for (const auto& source : sources)
        {
            context.Reporter.Info() << "Resetting source: " << source.Name << " ...";
            Repository::DropSource(source.Name);
            context.Reporter.Info() << " Done." << std::endl;
        }
    }

    void ResetAllSources(Execution::Context& context)
    {
        context.Reporter.Info() << "Resetting all sources ...";
        Repository::DropSource({});
        context.Reporter.Info() << " Done." << std::endl;
    }
}
