// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureExportCommand.h"
#include "Workflows/ConfigurationFlow.h"
#include "ConfigurationCommon.h"

using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    std::vector<Argument> ConfigureExportCommand::GetArguments() const
    {
        return {
            Argument{ Execution::Args::Type::OutputFile, Resource::String::OutputFileArgumentDescription, true },
            Argument{ Execution::Args::Type::ConfigurationExportPackageId, Resource::String::ConfigureExportPackageId },
            Argument{ Execution::Args::Type::ConfigurationExportModule, Resource::String::ConfigureExportModule },
            Argument{ Execution::Args::Type::ConfigurationExportResource, Resource::String::ConfigureExportResource },
            Argument{ Execution::Args::Type::ConfigurationModulePath, Resource::String::ConfigurationModulePath },
        };
    }

    Resource::LocString ConfigureExportCommand::ShortDescription() const
    {
        return { Resource::String::ConfigureExportCommandShortDescription };
    }

    Resource::LocString ConfigureExportCommand::LongDescription() const
    {
        return { Resource::String::ConfigureExportCommandLongDescription };
    }

    Utility::LocIndView ConfigureExportCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-configure#export"_liv;
    }

    void ConfigureExportCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            VerifyIsFullPackage <<
            CreateConfigurationProcessor <<
            CreateOrOpenConfigurationSet <<
            AddWinGetPackageAndResource <<
            WriteConfigFile;
    }

    void ConfigureExportCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        Configuration::ValidateCommonArguments(execArgs);

        bool validInputArgs = false;
        if (execArgs.Contains(Execution::Args::Type::ConfigurationExportModule, Execution::Args::Type::ConfigurationExportResource) ||
            execArgs.Contains(Execution::Args::Type::ConfigurationExportPackageId))
        {
            validInputArgs = true;
        }

        if (!validInputArgs)
        {
            throw CommandException(Resource::String::ConfigureExportArgumentError);
        }
    }
}
