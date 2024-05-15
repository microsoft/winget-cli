// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureValidateCommand.h"
#include "Workflows/ConfigurationFlow.h"
#include "ConfigurationCommon.h"

using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    std::vector<Argument> ConfigureValidateCommand::GetArguments() const
    {
        return {
            Argument{ Execution::Args::Type::ConfigurationFile, Resource::String::ConfigurationFileArgumentDescription, ArgumentType::Positional, true },
            Argument{ Execution::Args::Type::ConfigurationModulePath, Resource::String::ConfigurationModulePath, ArgumentType::Positional },
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
        return "https://aka.ms/winget-command-configure#validate"_liv;
    }

    void ConfigureValidateCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            VerifyIsFullPackage <<
            VerifyFileOrUri(Execution::Args::Type::ConfigurationFile) <<
            CreateConfigurationProcessor <<
            OpenConfigurationSet <<
            ValidateConfigurationSetSemantics <<
            ValidateConfigurationSetUnitProcessors <<
            ValidateConfigurationSetUnitContents <<
            ValidateAllGoodMessage;
    }

    void ConfigureValidateCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        Configuration::ValidateCommonArguments(execArgs);
    }
}
