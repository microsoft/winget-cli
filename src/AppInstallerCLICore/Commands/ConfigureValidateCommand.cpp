// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureValidateCommand.h"
#include "Workflows/ConfigurationFlow.h"
#include <AppInstallerRuntime.h>

using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    std::vector<Argument> ConfigureValidateCommand::GetArguments() const
    {
        return {
            Argument{ Execution::Args::Type::ConfigurationFile, Resource::String::ConfigurationFileArgumentDescription, ArgumentType::Positional, true },
            Argument{ Execution::Args::Type::ConfigurationCustomLocationPath, Resource::String::ConfigurationCustomLocationPath, ArgumentType::Positional },
            Argument{ Execution::Args::Type::ConfigurationAllUsersLocation, Resource::String::ConfigurationAllUsers, ArgumentType::Flag },
            Argument{ Execution::Args::Type::ConfigurationCurrentUserLocation, Resource::String::ConfigurationCurrentUser, ArgumentType::Flag },
            Argument{ Execution::Args::Type::ConfigurationWinGetLocation, Resource::String::ConfigurationWinGetLocation, ArgumentType::Flag },
        };
    }

    Resource::LocString ConfigureValidateCommand::ShortDescription() const
    {
        return { Resource::String::ConfigureValidateCommandShortDescription };
    }

    Resource::LocString ConfigureValidateCommand::LongDescription() const
    {
        return { Resource::String::ConfigureValidateCommandLongDescription };
    }

    Utility::LocIndView ConfigureValidateCommand::HelpLink() const
    {
        // TODO: Make this exist
        return "https://aka.ms/winget-command-configure#validate"_liv;
    }

    void ConfigureValidateCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            VerifyIsFullPackage <<
            VerifyFile(Execution::Args::Type::ConfigurationFile) <<
            CreateConfigurationProcessor <<
            OpenConfigurationSet <<
            ValidateConfigurationSetSemantics <<
            ValidateConfigurationSetUnitProcessors <<
            ValidateAllGoodMessage;
    }

    void ConfigureValidateCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::ConfigurationAllUsersLocation) && !Runtime::IsRunningAsAdmin())
        {
            throw CommandException(Resource::String::ConfigurationAllUsersElevated);
        }
    }
}
