// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CompleteCommand.h"
#include "RootCommand.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace Execution;

    std::vector<Argument> CompleteCommand::GetArguments() const
    {
        return {
            Argument{ "word", Argument::NoAlias, Args::Type::Word, Resource::String::WordArgumentDescription, ArgumentType::Standard, Argument::Visibility::Example, true },
            Argument{ "commandline", Argument::NoAlias, Args::Type::CommandLine, Resource::String::CommandLineArgumentDescription, ArgumentType::Standard, Argument::Visibility::Example, true },
            Argument{ "position", Argument::NoAlias, Args::Type::Position, Resource::String::PositionArgumentDescription, ArgumentType::Standard, Argument::Visibility::Example, true },
        };
    }

    Resource::LocString CompleteCommand::ShortDescription() const
    {
        return { Resource::String::CompleteCommandShortDescription };
    }

    Resource::LocString CompleteCommand::LongDescription() const
    {
        return { Resource::String::CompleteCommandLongDescription };
    }

    std::string CompleteCommand::HelpLink() const
    {
        // TODO: Define me and point to the right location
        return "https://aka.ms/winget-command-complete";
    }

    void CompleteCommand::ExecuteInternal(Execution::Context& context) const
    {
        try
        {
            CompletionData data{
                context.Args.GetArg(Args::Type::Word),
                context.Args.GetArg(Args::Type::CommandLine),
                context.Args.GetArg(Args::Type::Position) };

            std::unique_ptr<Command> command = std::make_unique<RootCommand>();

            std::unique_ptr<Command> subCommand = command->FindSubCommand(data.BeforeWord());
            while (subCommand)
            {
                command = std::move(subCommand);
                subCommand = command->FindSubCommand(data.BeforeWord());
            }

            // Create a new Context to execute the Complete from
            auto subContextPtr = context.Clone();
            Context& subContext = *subContextPtr;
            subContext.Reporter.SetChannel(Execution::Reporter::Channel::Completion);
            subContext.Add<Data::CompletionData>(std::move(data));

            // Disable all telemetry while doing a completion
            Logging::DisableTelemetryScope disable;

            AICLI_LOG(CLI, Info, << "Complete handing off to command " << command->FullName());
            command->Complete(subContext);
        }
        catch (const CommandException& ce)
        {
            AICLI_LOG(CLI, Info, << "Error encountered during completion, ignoring: " << ce.Message());
        }
        catch (const Settings::GroupPolicyException& e)
        {
            AICLI_LOG(CLI, Info, << "Error encountered during completion, ignoring: Blocked by Group Policy " << Settings::TogglePolicy::GetPolicy(e.Policy()).RegValueName());
        }
        catch (...)
        {
            AICLI_LOG(CLI, Info, << "Error encountered during completion, ignoring...");
        }
    }
}
