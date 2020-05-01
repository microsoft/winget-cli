// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SourceCommand.h"
#include "Workflows/SourceFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

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
        return Resources::GetInstance().ResolveWingetString(L"SourceCommandShortDescription");
    }

    std::string SourceCommand::GetLongDescription() const
    {
        return Resources::GetInstance().ResolveWingetString(L"SourceCommandLongDescription");
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
        return Resources::GetInstance().ResolveWingetString(L"SourceAddCommandShortDescription");
    }

    std::string SourceAddCommand::GetLongDescription() const
    {
        return Resources::GetInstance().ResolveWingetString(L"SourceAddCommandLongDescription");
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
        return Resources::GetInstance().ResolveWingetString(L"SourceListCommandShortDescription");
    }

    std::string SourceListCommand::GetLongDescription() const
    {
        return Resources::GetInstance().ResolveWingetString(L"SourceListCommandLongDescription");
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
        return Resources::GetInstance().ResolveWingetString(L"SourceUpdateCommandShortDescription");
    }

    std::string SourceUpdateCommand::GetLongDescription() const
    {
        return Resources::GetInstance().ResolveWingetString(L"SourceUpdateCommandLongDescription");
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
        return Resources::GetInstance().ResolveWingetString(L"SourceRemoveCommandShortDescription");
    }

    std::string SourceRemoveCommand::GetLongDescription() const
    {
        return Resources::GetInstance().ResolveWingetString(L"SourceRemoveCommandLongDescription");
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
        return Resources::GetInstance().ResolveWingetString(L"SourceResetCommandShortDescription");
    }

    std::string SourceResetCommand::GetLongDescription() const
    {
        return Resources::GetInstance().ResolveWingetString(L"SourceResetCommandLongDescription");
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
