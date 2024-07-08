// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureShowCommand.h"
#include "Workflows/ConfigurationFlow.h"
#include "ConfigurationCommon.h"

using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    std::vector<Argument> ConfigureShowCommand::GetArguments() const
    {
        return {
            // Required for now, make exclusive when history implemented
            Argument{ Execution::Args::Type::ConfigurationFile, Resource::String::ConfigurationFileArgumentDescription, ArgumentType::Positional },
            Argument{ Execution::Args::Type::ConfigurationModulePath, Resource::String::ConfigurationModulePath, ArgumentType::Positional },
            Argument{ Execution::Args::Type::ConfigurationHistoryItem, Resource::String::ConfigurationHistoryItemArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help },
        };
    }

    Resource::LocString ConfigureShowCommand::ShortDescription() const
    {
        return { Resource::String::ConfigureShowCommandShortDescription };
    }

    Resource::LocString ConfigureShowCommand::LongDescription() const
    {
        return { Resource::String::ConfigureShowCommandLongDescription };
    }

    Utility::LocIndView ConfigureShowCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-configure#show"_liv;
    }

    void ConfigureShowCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            VerifyIsFullPackage <<
            VerifyFileOrUri(Execution::Args::Type::ConfigurationFile) <<
            CreateConfigurationProcessor <<
            OpenConfigurationSet <<
            ShowConfigurationSet;
    }

    void ConfigureShowCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        Configuration::ValidateCommonArguments(execArgs, true);
    }

    void ConfigureShowCommand::Complete(Execution::Context& context, Execution::Args::Type argType) const
    {
        if (argType == Execution::Args::Type::ConfigurationHistoryItem)
        {
            context << CompleteConfigurationHistoryItem;
        }
    }
}
