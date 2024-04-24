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
            // TODO: Resource
            Argument{ Execution::Args::Type::OutputFile, Resource::String::ConfigurationModulePath, true },
            Argument{ Execution::Args::Type::ConfigurationExportPackageId, Resource::String::ConfigurationModulePath },
            Argument{ Execution::Args::Type::ConfigurationExportModule, Resource::String::ConfigurationModulePath },
            Argument{ Execution::Args::Type::ConfigurationExportResource, Resource::String::ConfigurationModulePath },
            Argument{ Execution::Args::Type::ConfigurationModulePath, Resource::String::ConfigurationModulePath },
        };
    }

    Resource::LocString ConfigureExportCommand::ShortDescription() const
    {
        // TODO
        return { Resource::String::ConfigureShowCommandShortDescription };
    }

    Resource::LocString ConfigureExportCommand::LongDescription() const
    {
        // TODO
        return { Resource::String::ConfigureShowCommandLongDescription };
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
        if ((execArgs.Contains(Execution::Args::Type::ConfigurationExportModule) && execArgs.Contains(Execution::Args::Type::ConfigurationExportResource)) ||
            execArgs.Contains(Execution::Args::Type::ConfigurationExportPackageId))
        {
            validInputArgs = true;
        }

        if (!validInputArgs)
        {
            // TODO: At least --packageId and/or --module with --resource must be used.
            throw CommandException(Resource::String::ConfigurationEnableArgumentError);
        }
    }
}
