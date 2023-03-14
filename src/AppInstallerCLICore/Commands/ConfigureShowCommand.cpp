// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureShowCommand.h"
#include "Workflows/ConfigurationFlow.h"

using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    std::vector<Argument> ConfigureShowCommand::GetArguments() const
    {
        return {
            // Required for now, make exclusive when history implemented
            Argument{ Execution::Args::Type::ConfigurationFile, Resource::String::ConfigurationFileArgumentDescription, ArgumentType::Positional, true },
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
        // TODO: Make this exist
        return "https://aka.ms/winget-command-configure#show"_liv;
    }

    void ConfigureShowCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            CreateConfigurationProcessor <<
            OpenConfigurationSet <<
            ShowConfigurationSet;
    }
}
