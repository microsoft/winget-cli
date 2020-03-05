// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Command.h"
#include "Localization.h"

namespace AppInstaller::CLI
{
    void Command::OutputIntroHeader(ExecutionReporter& reporter) const
    {
        reporter.ShowMsg("AppInstaller Command Line");
        reporter.ShowMsg("Copyright (c) Microsoft Corporation");
    }

    void Command::OutputHelp(ExecutionReporter& reporter, const CommandException* exception) const
    {
        OutputIntroHeader(reporter);
        reporter.EmptyLine();

        if (exception)
        {
            reporter.ShowMsg(exception->Message() + " : '" + std::string(exception->Param()) + '\'', ExecutionReporter::Level::Error);
            reporter.EmptyLine();
        }

        for (const auto& line : GetLongDescription())
        {
            reporter.ShowMsg(line);
        }
        reporter.EmptyLine();

        auto commands = GetCommands();
        if (!commands.empty())
        {
            reporter.ShowMsg(LOCME("The following commands are available:"));
            reporter.EmptyLine();

            for (const auto& command : commands)
            {
                reporter.ShowMsg("  " + std::string(command->Name()));
                reporter.ShowMsg("    " + command->ShortDescription());
            }

            reporter.EmptyLine();
            reporter.ShowMsg(std::string(LOCME("For more details on a specific command, pass it the help argument.")) + " [" + APPINSTALLER_CLI_HELP_ARGUMENT + "]");
        }
        else
        {
            reporter.ShowMsg(LOCME("The following arguments are available:"));
            reporter.EmptyLine();

            for (const auto& arg : GetArguments())
            {
                reporter.ShowMsg("  " + std::string(arg.Name()));
                reporter.ShowMsg("    " + arg.Description());
            }
        }
    }

    std::unique_ptr<Command> Command::FindInvokedCommand(Invocation& inv) const
    {
        auto itr = inv.begin();
        if (itr == inv.end() || (*itr)[0] == APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR)
        {
            // No more command arguments to check, so no command to find
            return {};
        }

        auto commands = GetCommands();
        if (commands.empty())
        {
            // No more subcommands
            return {};
        }

        for (auto& command : commands)
        {
            if (*itr == command->Name())
            {
                AICLI_LOG(CLI, Info, << "Found subcommand: " << *itr);
                inv.consume(itr);
                std::unique_ptr<Command> subcommand = command->FindInvokedCommand(inv);
                // If we found a subcommand, return it.  Otherwise, this is the one.
                return (subcommand ? std::move(subcommand) : std::move(command));
            }
        }

        throw CommandException(LOCME("Unrecognized command"), *itr);
    }

    void Command::ParseArguments(Invocation& inv) const
    {
        auto definedArgs = GetArguments();
        auto positionalSearchItr = definedArgs.begin();

        for (auto incomingArgsItr = inv.begin(); incomingArgsItr != inv.end(); ++incomingArgsItr)
        {
            if ((*incomingArgsItr)[0] != APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR)
            {
                // Positional argument, find the next appropriate one if the current itr isn't one or has hit its limit.
                if (positionalSearchItr != definedArgs.end() &&
                    (positionalSearchItr->Type() != ArgumentType::Positional || inv.GetCount(positionalSearchItr->Name()) == positionalSearchItr->Limit()))
                {
                    for (++positionalSearchItr; positionalSearchItr != definedArgs.end() && positionalSearchItr->Type() != ArgumentType::Positional; ++positionalSearchItr);
                }

                if (positionalSearchItr == definedArgs.end())
                {
                    throw CommandException(LOCME("Found a positional argument when none was expected"), *incomingArgsItr);
                }

                inv.AddArg(positionalSearchItr->Name(), *incomingArgsItr);
            }
            else
            {
                // This is an arg name, find it and process its value if needed.
                // Skip the name identifier char.
                std::string argName = incomingArgsItr->substr(1);
                bool argFound = false;

                for (const auto& arg : definedArgs)
                {
                    if (argName == arg.Name())
                    {
                        if (arg.Type() == ArgumentType::Flag)
                        {
                            inv.AddArg(arg.Name());
                        }
                        else
                        {
                            ++incomingArgsItr;
                            if (incomingArgsItr == inv.end())
                            {
                                throw CommandException(LOCME("Argument value required, but none found"), *incomingArgsItr);
                            }
                            inv.AddArg(arg.Name(), *incomingArgsItr);
                        }
                        argFound = true;
                        break;
                    }
                }

                if (argName == APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_STRING)
                {
                    inv.AddArg(APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_STRING);
                }
                else if (!argFound)
                {
                    throw CommandException(LOCME("Argument name was not recognized for the current command"), *incomingArgsItr);
                }
            }
        }
    }

    void Command::ValidateArguments(Invocation& inv) const
    {
        // If help is asked for, don't bother validating anything else
        if (inv.Contains(APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_STRING))
        {
            return;
        }

        for (const auto& arg : GetArguments())
        {
            if (arg.Required() && !inv.Contains(arg.Name()))
            {
                throw CommandException(LOCME("Required argument not provided"), arg.Name());
            }

            if (arg.Limit() < inv.GetCount(arg.Name()))
            {
                throw CommandException(LOCME("Argument provided more times than allowed"), arg.Name());
            }
        }
    }

    void Command::PopulateExecutionArgs(const Invocation& inv, ExecutionArgs& args) const
    {
        auto invArgs = inv.GetAllArgs();

        for (const auto& invArg : *invArgs)
        {
            if (invArg.first == APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_STRING)
            {
                args.AddArg(ExecutionArgs::ExecutionArgType::Help);
                continue;
            }

            auto execArgType = GetExecutionArgType(invArg.first);
            if (invArg.second.empty())
            {
                // Flag
                args.AddArg(execArgType);
            }
            else
            {
                for (const auto& argValue : invArg.second)
                {
                    args.AddArg(execArgType, argValue);
                }
            }
        }
    }

    void Command::Execute(ExecutionContext& context) const
    {
        AICLI_LOG(CLI, Info, << "Executing command: " << Name());
        if (context.Args.Contains(ExecutionArgs::ExecutionArgType::Help))
        {
            OutputHelp(context.Reporter);
        }
        else
        {
            ExecuteInternal(context);
        }
    }

    void Command::ExecuteInternal(ExecutionContext& context) const
    {
        context.Reporter.ShowMsg(LOCME("Oops, we forgot to do this..."), ExecutionReporter::Level::Error);
        THROW_HR(E_NOTIMPL);
    }
}
