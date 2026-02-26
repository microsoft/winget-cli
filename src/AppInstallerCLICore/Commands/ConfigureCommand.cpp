// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureCommand.h"
#include "ConfigureListCommand.h"
#include "ConfigureShowCommand.h"
#include "ConfigureTestCommand.h"
#include "ConfigureValidateCommand.h"
#include "ConfigureExportCommand.h"
#include "Workflows/ConfigurationFlow.h"
#include "Workflows/MSStoreInstallerHandler.h"
#include "ConfigurationCommon.h"

using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    ConfigureCommand::ConfigureCommand(std::string_view parent) :
        Command("configure", { "configuration", "dsc"}, parent, Settings::TogglePolicy::Policy::Configuration)
    {
        SelectCurrentCommandIfUnrecognizedSubcommandFound(true);
    }

    std::vector<std::unique_ptr<Command>> ConfigureCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<ConfigureShowCommand>(FullName()),
            std::make_unique<ConfigureListCommand>(FullName()),
            std::make_unique<ConfigureTestCommand>(FullName()),
            std::make_unique<ConfigureValidateCommand>(FullName()),
            std::make_unique<ConfigureExportCommand>(FullName()),
        });
    }

    std::vector<Argument> ConfigureCommand::GetArguments() const
    {
        return {
            Argument{ Execution::Args::Type::ConfigurationFile, Resource::String::ConfigurationFileArgumentDescription, ArgumentType::Positional },
            Argument{ Execution::Args::Type::ConfigurationModulePath, Resource::String::ConfigurationModulePath, ArgumentType::Positional },
            Argument{ Execution::Args::Type::ConfigurationProcessorPath, Resource::String::ConfigurationProcessorPath, ArgumentType::Standard, Argument::Visibility::Help },
            Argument{ Execution::Args::Type::ConfigurationHistoryItem, Resource::String::ConfigurationHistoryItemArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help },
            Argument{ Execution::Args::Type::ConfigurationAcceptWarning, Resource::String::ConfigurationAcceptWarningArgumentDescription, ArgumentType::Flag },
            Argument{ Execution::Args::Type::ConfigurationSuppressPrologue, Resource::String::ConfigurationSuppressPrologueArgumentDescription, ArgumentType::Flag, Argument::Visibility::Help },
            Argument{ Execution::Args::Type::ExtendedFeaturesEnable, Resource::String::ExtendedFeaturesEnableMessage, ArgumentType::Flag, Argument::Visibility::Help },
            Argument{ Execution::Args::Type::ExtendedFeaturesDisable, Resource::String::ExtendedFeaturesDisableMessage, ArgumentType::Flag, Argument::Visibility::Help },
        };
    }

    Resource::LocString ConfigureCommand::ShortDescription() const
    {
        return { Resource::String::ConfigureCommandShortDescription };
    }

    Resource::LocString ConfigureCommand::LongDescription() const
    {
        return { Resource::String::ConfigureCommandLongDescription };
    }

    Utility::LocIndView ConfigureCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-configure"_liv;
    }

    void ConfigureCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::ExtendedFeaturesEnable))
        {
            context <<
                EnableExtendedFeatures;
        }
        else if (context.Args.Contains(Execution::Args::Type::ExtendedFeaturesDisable))
        {
            context <<
                DisableExtendedFeatures;
        }
        else
        {
            context <<
                VerifyIsFullPackage <<
                VerifyFileOrUri(Execution::Args::Type::ConfigurationFile) <<
                CreateConfigurationProcessorWithoutFactory <<
                OpenConfigurationSet <<
                CreateConfigurationProcessor <<
                ShowConfigurationSet <<
                ShowConfigurationSetConflicts <<
                ConfirmConfigurationProcessing(true) <<
                ApplyConfigurationSet;
        }
    }

    void ConfigureCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::ExtendedFeaturesEnable) ||
            execArgs.Contains(Execution::Args::Type::ExtendedFeaturesDisable))
        {
            if (execArgs.GetArgsCount() > 1)
            {
                throw CommandException(Resource::String::ExtendedFeaturesEnableArgumentError);
            }
        }
        else
        {
            Configuration::ValidateCommonArguments(execArgs, true);
        }
    }

    void ConfigureCommand::Complete(Execution::Context& context, Execution::Args::Type argType) const
    {
        if (argType == Execution::Args::Type::ConfigurationHistoryItem)
        {
            context << CompleteConfigurationHistoryItem;
        }
    }
}
