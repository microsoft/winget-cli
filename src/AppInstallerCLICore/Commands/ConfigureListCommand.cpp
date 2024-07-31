// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureListCommand.h"
#include "Workflows/ConfigurationFlow.h"
#include "ConfigurationCommon.h"

using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    std::vector<Argument> ConfigureListCommand::GetArguments() const
    {
        return {
            Argument{ Execution::Args::Type::ConfigurationHistoryItem, Resource::String::ConfigurationHistoryItemArgumentDescription, ArgumentType::Standard },
            Argument{ Execution::Args::Type::OutputFile, Resource::String::OutputFileArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help },
            Argument{ Execution::Args::Type::ConfigurationHistoryRemove, Resource::String::ConfigurationHistoryRemoveArgumentDescription, ArgumentType::Flag, Argument::Visibility::Help },
            Argument{ Execution::Args::Type::ConfigurationStatusWatch, Resource::String::ConfigurationStatusWatchArgumentDescription, ArgumentType::Flag, Argument::Visibility::Hidden },
        };
    }

    Resource::LocString ConfigureListCommand::ShortDescription() const
    {
        return { Resource::String::ConfigureListCommandShortDescription };
    }

    Resource::LocString ConfigureListCommand::LongDescription() const
    {
        return { Resource::String::ConfigureListCommandLongDescription };
    }

    Utility::LocIndView ConfigureListCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-configure#list"_liv;
    }

    void ConfigureListCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            VerifyIsFullPackage <<
            CreateConfigurationProcessorWithoutFactory <<
            GetConfigurationSetHistory;

        if (context.Args.Contains(Execution::Args::Type::ConfigurationHistoryItem))
        {
            context << SelectSetFromHistory;

            if (context.Args.Contains(Execution::Args::Type::OutputFile))
            {
                context << SerializeConfigurationSetHistory;
            }

            if (context.Args.Contains(Execution::Args::Type::ConfigurationHistoryRemove))
            {
                context << RemoveConfigurationSetHistory;
            }
            else
            {
                context << ShowSingleConfigurationSetHistory;
            }
        }
        else if (context.Args.Contains(Execution::Args::Type::ConfigurationStatusWatch))
        {
            context << MonitorConfigurationStatus;
        }
        else
        {
            context << ShowConfigurationSetHistory;
        }
    }

    void ConfigureListCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if ((execArgs.Contains(Execution::Args::Type::ConfigurationHistoryRemove) ||
             execArgs.Contains(Execution::Args::Type::OutputFile)) &&
            !execArgs.Contains(Execution::Args::Type::ConfigurationHistoryItem))
        {
            throw CommandException(Resource::String::RequiredArgError(ArgumentCommon::ForType(Execution::Args::Type::ConfigurationHistoryItem).Name));
        }
    }

    void ConfigureListCommand::Complete(Execution::Context& context, Execution::Args::Type argType) const
    {
        if (argType == Execution::Args::Type::ConfigurationHistoryItem)
        {
            context << CompleteConfigurationHistoryItem;
        }
    }
}
