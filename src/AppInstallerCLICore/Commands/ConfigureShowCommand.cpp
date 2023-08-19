// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureShowCommand.h"
#include "Workflows/ConfigurationFlow.h"
#include <AppInstallerRuntime.h>

using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    std::vector<Argument> ConfigureShowCommand::GetArguments() const
    {
        return {
            // Required for now, make exclusive when history implemented
            Argument{ Execution::Args::Type::ConfigurationFile, Resource::String::ConfigurationFileArgumentDescription, ArgumentType::Positional, true },
            Argument{ Execution::Args::Type::ConfigurationCustomLocationPath, Resource::String::ConfigurationCustomLocationPath, ArgumentType::Positional },
            Argument{ Execution::Args::Type::ConfigurationAllUsersLocation, Resource::String::ConfigurationAllUsers, ArgumentType::Flag },
            Argument{ Execution::Args::Type::ConfigurationCurrentUserLocation, Resource::String::ConfigurationCurrentUser, ArgumentType::Flag },
            Argument{ Execution::Args::Type::ConfigurationWinGetLocation, Resource::String::ConfigurationWinGetLocation, ArgumentType::Flag },
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
            VerifyFile(Execution::Args::Type::ConfigurationFile) <<
            CreateConfigurationProcessor <<
            OpenConfigurationSet <<
            ShowConfigurationSet;
    }

    void ConfigureShowCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::ConfigurationAllUsersLocation) && !Runtime::IsRunningAsAdmin())
        {
            throw CommandException(Resource::String::ConfigurationAllUsersElevated);
        }
    }
}
