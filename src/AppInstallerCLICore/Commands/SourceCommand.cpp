// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SourceCommand.h"
#include "Localization.h"
#include "Workflows/SourceFlow.h"
#include "Workflows/WorkflowBase.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;

    std::vector<std::unique_ptr<Command>> SourceCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<SourceAddCommand>(FullName()),
            std::make_unique<SourceListCommand>(FullName()),
            std::make_unique<SourceUpdateCommand>(FullName()),
            std::make_unique<SourceRemoveCommand>(FullName()),
            std::make_unique<SourceResetCommand>(FullName()),
            });
    }

    std::string SourceCommand::ShortDescription() const
    {
        return LOCME("Manage sources of applications");
    }

    std::string SourceCommand::GetLongDescription() const
    {
        return LOCME("Manage sources with the sub-commands. A source provides the data for you to discover and install applications. Only add a new source if you trust it as a secure location.");
    }

    void SourceCommand::ExecuteInternal(Context& context) const
    {
        OutputHelp(context.Reporter);
    }

    std::vector<Argument> SourceAddCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::SourceName).SetRequired(true),
            Argument::ForType(Args::Type::SourceArg),
            Argument::ForType(Args::Type::SourceType),
        };
    }

    std::string SourceAddCommand::ShortDescription() const
    {
        return LOCME("Add a new source");
    }

    std::string SourceAddCommand::GetLongDescription() const
    {
        return LOCME("Add a new source. A source provides the data for you to discover and install applications. Only add a new source if you trust it as a secure location.");
    }

    void SourceAddCommand::ExecuteInternal(Context& context) const
    {
        context <<
            Workflow::GetSourceList <<
            Workflow::CheckSourceListAgainstAdd <<
            Workflow::AddSource;
    }

    std::vector<Argument> SourceListCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::SourceName),
        };
    }

    std::string SourceListCommand::ShortDescription() const
    {
        return LOCME("List current sources");
    }

    std::string SourceListCommand::GetLongDescription() const
    {
        return LOCME("List all current sources, or full details of a specific source.");
    }

    void SourceListCommand::ExecuteInternal(Context& context) const
    {
        context <<
            Workflow::GetSourceListWithFilter <<
            Workflow::ListSources;
    }

    std::vector<Argument> SourceUpdateCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::SourceName),
        };
    }

    std::string SourceUpdateCommand::ShortDescription() const
    {
        return LOCME("Update current sources");
    }

    std::string SourceUpdateCommand::GetLongDescription() const
    {
        return LOCME("Update all sources, or only a specific source.");
    }

    void SourceUpdateCommand::ExecuteInternal(Context& context) const
    {
        context <<
            Workflow::GetSourceListWithFilter <<
            Workflow::UpdateSources;
    }

    std::vector<Argument> SourceRemoveCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::SourceName).SetRequired(true),
        };
    }

    std::string SourceRemoveCommand::ShortDescription() const
    {
        return LOCME("Remove current sources");
    }

    std::string SourceRemoveCommand::GetLongDescription() const
    {
        return LOCME("Remove a specific source.");
    }

    void SourceRemoveCommand::ExecuteInternal(Context& context) const
    {
        context <<
            Workflow::GetSourceListWithFilter <<
            Workflow::RemoveSources;
    }

    std::vector<Argument> SourceResetCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::SourceName),
            Argument::ForType(Args::Type::Force),
        };
    }

    std::string SourceResetCommand::ShortDescription() const
    {
        return LOCME("Reset sources");
    }

    std::string SourceResetCommand::GetLongDescription() const
    {
        return LOCME("This command drops existing sources, potentially leaving any local data behind. Without any argument, it will drop all sources and add the defaults. If a named source is provided, only that source will be dropped.");
    }

    void SourceResetCommand::ExecuteInternal(Context& context) const
    {
        if (context.Args.Contains(Args::Type::SourceName))
        {
            context <<
                Workflow::GetSourceListWithFilter <<
                Workflow::ResetSourceList;
        }
        else
        {
            context <<
                Workflow::QueryUserForSourceReset <<
                Workflow::ResetAllSources <<
                Workflow::AddDefaultSources;
        }
    }
}
