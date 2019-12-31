// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Command.h"
#include "Localization.h"

namespace AppInstaller::CLI
{
    void Command::OutputIntroHeader(std::ostream& out) const
    {
        out << "AppInstaller Command Line" << std::endl;
        out << "(c) 2019 Microsoft Corporation" << std::endl;
    }

    void Command::OutputHelp(std::ostream& out, const CommandException* exception) const
    {
        OutputIntroHeader(out);
        out << std::endl;

        if (exception)
        {
            out << exception->Message() << " : '" << exception->Param() << '\'' << std::endl;
            out << std::endl;
        }

        for (const auto& line : GetLongDescription())
        {
            out << line << std::endl;
        }
        out << std::endl;

        auto commands = GetCommands();
        if (!commands.empty())
        {
            out << LOCME("The following commands are available:") << std::endl;
            out << std::endl;

            for (const auto& command : commands)
            {
                out << "  " << command->Name() << std::endl;
                out << "    " << command->ShortDescription() << std::endl;
            }

            out << std::endl;
            out << LOCME("For more details on a specific command, pass it the help argument.") << " [" << APPINSTALLER_CLI_HELP_ARGUMENT << "]" << std::endl;
        }
        else
        {
            out << LOCME("The following arguments are available:") << std::endl;
            out << std::endl;

            for (const auto& arg : GetArguments())
            {
                out << "  " << arg.Name() << std::endl;
                out << "    " << arg.Description() << std::endl;
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

    void Command::Execute(Invocation& inv, std::ostream& out) const
    {
        if (inv.Contains(APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_STRING))
        {
            OutputHelp(out);
        }
        else
        {
            ExecuteInternal(inv, out);
        }
    }

    void Command::ExecuteInternal(Invocation&, std::ostream& out) const
    {
        out << LOCME("Oops, we forgot to do this...") << std::endl;
    }
}
