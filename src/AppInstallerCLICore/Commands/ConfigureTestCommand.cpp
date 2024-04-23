// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureTestCommand.h"
#include "Workflows/ConfigurationFlow.h"
#include "ConfigurationCommon.h"

using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    std::vector<Argument> ConfigureTestCommand::GetArguments() const
    {
        return {
            Argument{ Execution::Args::Type::ConfigurationFile, Resource::String::ConfigurationFileArgumentDescription, ArgumentType::Positional, true },
            Argument{ Execution::Args::Type::ConfigurationModulePath, Resource::String::ConfigurationModulePath, ArgumentType::Positional },
            Argument{ Execution::Args::Type::ConfigurationAcceptWarning, Resource::String::ConfigurationAcceptWarningArgumentDescription, ArgumentType::Flag },
        };
    }

    Resource::LocString ConfigureTestCommand::ShortDescription() const
    {
        return { Resource::String::ConfigureTestCommandShortDescription };
    }

    Resource::LocString ConfigureTestCommand::LongDescription() const
    {
        return { Resource::String::ConfigureTestCommandLongDescription };
    }

    Utility::LocIndView ConfigureTestCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-configure#test"_liv;
    }

    void ConfigureTestCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            VerifyIsFullPackage <<
            VerifyFileOrUri(Execution::Args::Type::ConfigurationFile) <<
            CreateConfigurationProcessor <<
            OpenConfigurationSet <<
            ShowConfigurationSet <<
            ShowConfigurationSetConflicts <<
            ConfirmConfigurationProcessing(false) <<
            TestConfigurationSet;
    }

    void ConfigureTestCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        Configuration::ValidateCommonArguments(execArgs);
    }
}
