// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "McpCommand.h"
#include "Workflows/MSStoreInstallerHandler.h"
#include <AppInstallerRuntime.h>

using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    McpCommand::McpCommand(std::string_view parent) :
        Command("mcp", {}, parent, Settings::TogglePolicy::Policy::McpServer)
    {
    }

    std::vector<Argument> McpCommand::GetArguments() const
    {
        return {
            Argument{ Execution::Args::Type::ExtendedFeaturesEnable, Resource::String::ExtendedFeaturesEnableMessage, ArgumentType::Flag, Argument::Visibility::Help },
            Argument{ Execution::Args::Type::ExtendedFeaturesDisable, Resource::String::ExtendedFeaturesDisableMessage, ArgumentType::Flag, Argument::Visibility::Help },
        };
    }

    Resource::LocString McpCommand::ShortDescription() const
    {
        return { Resource::String::McpCommandShortDescription };
    }

    Resource::LocString McpCommand::LongDescription() const
    {
        return { Resource::String::McpCommandLongDescription };
    }

    Utility::LocIndView McpCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-mcp"_liv;
    }

    void McpCommand::ExecuteInternal(Execution::Context& context) const
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
                [](Execution::Context& context)
                {
                    std::filesystem::path exePath = Runtime::GetPathTo(Runtime::PathName::MCPExecutable);
                    std::string jsonCompatiblePath = exePath.u8string();
                    Utility::FindAndReplace(jsonCompatiblePath, "\\", "\\\\");

                    context.Reporter.Info() << Resource::String::McpConfigurationPreamble <<
                        R"(
    "winget-mcp": {
        "type": "stdio",
        "command": ")" << jsonCompatiblePath << R"("
    })" << std::endl;
                };
        }
    }

    void McpCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::ExtendedFeaturesEnable) ||
            execArgs.Contains(Execution::Args::Type::ExtendedFeaturesDisable))
        {
            if (execArgs.GetArgsCount() > 1)
            {
                throw CommandException(Resource::String::ExtendedFeaturesEnableArgumentError);
            }
        }
    }
}
