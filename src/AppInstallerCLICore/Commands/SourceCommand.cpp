// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SourceCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/SourceFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace std::string_view_literals;

    static constexpr std::string_view s_SourceCommand_HelpLink = "https://aka.ms/winget-command-source"sv;

    std::vector<std::unique_ptr<Command>> SourceCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<SourceAddCommand>(FullName()),
            std::make_unique<SourceListCommand>(FullName()),
            std::make_unique<SourceUpdateCommand>(FullName()),
            std::make_unique<SourceRemoveCommand>(FullName()),
            std::make_unique<SourceResetCommand>(FullName()),
            std::make_unique<SourceExportCommand>(FullName()),
            });
    }

    Resource::LocString SourceCommand::ShortDescription() const
    {
        return { Resource::String::SourceCommandShortDescription };
    }

    Resource::LocString SourceCommand::LongDescription() const
    {
        return { Resource::String::SourceCommandLongDescription };
    }

    std::string SourceCommand::HelpLink() const
    {
        return std::string{ s_SourceCommand_HelpLink };
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

    Resource::LocString SourceAddCommand::ShortDescription() const
    {
        return { Resource::String::SourceAddCommandShortDescription };
    }

    Resource::LocString SourceAddCommand::LongDescription() const
    {
        return { Resource::String::SourceAddCommandLongDescription };
    }

    std::string SourceAddCommand::HelpLink() const
    {
        return std::string{ s_SourceCommand_HelpLink };
    }

    void SourceAddCommand::ExecuteInternal(Context& context) const
    {
        // Note: Group Policy for allowed sources is enforced at the RepositoryCore level
        //       as we need to validate the source data and handle sources that were already added.
        context <<
            Workflow::EnsureRunningAsAdmin <<
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

    Resource::LocString SourceListCommand::ShortDescription() const
    {
        return { Resource::String::SourceListCommandShortDescription };
    }

    Resource::LocString SourceListCommand::LongDescription() const
    {
        return { Resource::String::SourceListCommandLongDescription };
    }

    void SourceListCommand::Complete(Context& context, Args::Type valueType) const
    {
        if (valueType == Args::Type::SourceName)
        {
            context <<
                Workflow::CompleteSourceName;
        }
    }

    std::string SourceListCommand::HelpLink() const
    {
        return std::string{ s_SourceCommand_HelpLink };
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

    Resource::LocString SourceUpdateCommand::ShortDescription() const
    {
        return { Resource::String::SourceUpdateCommandShortDescription };
    }

    Resource::LocString SourceUpdateCommand::LongDescription() const
    {
        return { Resource::String::SourceUpdateCommandLongDescription };
    }

    void SourceUpdateCommand::Complete(Context& context, Args::Type valueType) const
    {
        if (valueType == Args::Type::SourceName)
        {
            context <<
                Workflow::CompleteSourceName;
        }
    }

    std::string SourceUpdateCommand::HelpLink() const
    {
        return std::string{ s_SourceCommand_HelpLink };
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

    Resource::LocString SourceRemoveCommand::ShortDescription() const
    {
        return { Resource::String::SourceRemoveCommandShortDescription };
    }

    Resource::LocString SourceRemoveCommand::LongDescription() const
    {
        return { Resource::String::SourceRemoveCommandLongDescription };
    }

    void SourceRemoveCommand::Complete(Context& context, Args::Type valueType) const
    {
        if (valueType == Args::Type::SourceName)
        {
            context <<
                Workflow::CompleteSourceName;
        }
    }

    std::string SourceRemoveCommand::HelpLink() const
    {
        return std::string{ s_SourceCommand_HelpLink };
    }

    void SourceRemoveCommand::ExecuteInternal(Context& context) const
    {
        // Note: Group Policy for unremovable sources is enforced at the RepositoryCore.
        context <<
            Workflow::EnsureRunningAsAdmin <<
            Workflow::GetSourceListWithFilter <<
            Workflow::RemoveSources;
    }

    std::vector<Argument> SourceResetCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::SourceName),
            Argument{ "force", Argument::NoAlias, Args::Type::ForceSourceReset, Resource::String::SourceResetForceArgumentDescription, ArgumentType::Flag },
        };
    }

    Resource::LocString SourceResetCommand::ShortDescription() const
    {
        return { Resource::String::SourceResetCommandShortDescription };
    }

    Resource::LocString SourceResetCommand::LongDescription() const
    {
        return { Resource::String::SourceResetCommandLongDescription };
    }

    void SourceResetCommand::Complete(Context& context, Args::Type valueType) const
    {
        if (valueType == Args::Type::SourceName)
        {
            context <<
                Workflow::CompleteSourceName;
        }
    }

    std::string SourceResetCommand::HelpLink() const
    {
        return std::string{ s_SourceCommand_HelpLink };
    }

    void SourceResetCommand::ExecuteInternal(Context& context) const
    {
        if (context.Args.Contains(Args::Type::SourceName))
        {
            context <<
                Workflow::EnsureRunningAsAdmin <<
                Workflow::GetSourceListWithFilter <<
                Workflow::ResetSourceList;
        }
        else
        {
            context <<
                Workflow::EnsureRunningAsAdmin <<
                Workflow::QueryUserForSourceReset <<
                Workflow::ResetAllSources;
        }
    }

    std::vector<Argument> SourceExportCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::SourceName),
        };
    }

    Resource::LocString SourceExportCommand::ShortDescription() const
    {
        return { Resource::String::SourceExportCommandShortDescription };
    }

    Resource::LocString SourceExportCommand::LongDescription() const
    {
        return { Resource::String::SourceExportCommandLongDescription };
    }

    void SourceExportCommand::Complete(Context& context, Args::Type valueType) const
    {
        if (valueType == Args::Type::SourceName)
        {
            context <<
                Workflow::CompleteSourceName;
        }
    }

    std::string SourceExportCommand::HelpLink() const
    {
        return std::string{ s_SourceCommand_HelpLink };
    }

    void SourceExportCommand::ExecuteInternal(Context& context) const
    {
        context <<
            Workflow::GetSourceListWithFilter <<
            Workflow::ExportSourceList;
    }
}
