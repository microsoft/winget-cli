// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CompleteCommand.h"
#include "RootCommand.h"
#include "Resources.h"
#include <functional>

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace Execution;

    namespace
    {
        void WithCompletionCommand(const Args& args, const std::function<void(const Command&, CompletionData&&)>& action)
        {
            try
            {
                CompletionData data{
                    args.GetArg(Args::Type::Word),
                    args.GetArg(Args::Type::CommandLine),
                    args.GetArg(Args::Type::Position) };

                std::unique_ptr<Command> command = std::make_unique<RootCommand>();

                std::unique_ptr<Command> subCommand = command->FindSubCommand(data.BeforeWord());
                while (subCommand)
                {
                    command = std::move(subCommand);
                    subCommand = command->FindSubCommand(data.BeforeWord());
                }

                action(*command, std::move(data));
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

    std::vector<Argument> CompleteCommand::GetArguments() const
    {
        return {
            Argument{ Args::Type::Word, Resource::String::WordArgumentDescription, ArgumentType::Standard, Argument::Visibility::Example, true },
            Argument{ Args::Type::CommandLine, Resource::String::CommandLineArgumentDescription, ArgumentType::Standard, Argument::Visibility::Example, true },
            Argument{ Args::Type::Position, Resource::String::PositionArgumentDescription, ArgumentType::Standard, Argument::Visibility::Example, true },
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

    Utility::LocIndView CompleteCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-complete"_liv;
    }

    Args CompleteCommand::GetArgumentsForLog(const Args& args)
    {
        Args argsForLog = args;
        if (args.Contains(Args::Type::Word, Args::Type::CommandLine, Args::Type::Position))
        {
            WithCompletionCommand(args, [&](const Command& command, CompletionData&& data)
            {
                for (const auto& proxy : command.GetCompletionProxiesForLog(data))
                {
                    argsForLog.AddArg(Args::Type::Proxy, proxy.second);
                }
            });
        }
        return argsForLog;
    }

    void CompleteCommand::ExecuteInternal(Execution::Context& context) const
    {
        WithCompletionCommand(context.Args, [&](const Command& command, CompletionData&& data)
        {
            // Create a new Context to execute the Complete from
            auto subContextPtr = context.CreateSubContext();
            Context& subContext = *subContextPtr;
            auto previousThreadGlobals = subContext.SetForCurrentThread();

            subContext.Reporter.SetChannel(Execution::Reporter::Channel::Completion);
            subContext.Add<Data::CompletionData>(std::move(data));

            // Disable all telemetry while doing a completion
            Logging::DisableTelemetryScope disable;

            AICLI_LOG(CLI, Info, << "Complete handing off to command " << command.FullName());
            command.Complete(subContext);
        });
    }
}
