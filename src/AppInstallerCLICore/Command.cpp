// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Command.h"
#include "Localization.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    constexpr std::string_view s_Command_NameSeparator = "::"sv;

    Command::Command(std::string_view name, std::string_view parent) :
        m_name(name)
    {
        if (!parent.empty())
        {
            m_fullName.reserve(parent.length() + s_Command_NameSeparator.length() + name.length());
            m_fullName = parent;
            m_fullName += s_Command_NameSeparator;
            m_fullName += name;
        }
        else
        {
            m_fullName = name;
        }
    }

    void Command::OutputIntroHeader(Execution::Reporter& reporter) const
    {
        reporter.Info() <<
            "AppInstaller Command Line [" << Runtime::GetClientVersion() << ']' << std::endl <<
            "Copyright (c) Microsoft Corporation" << std::endl;
    }

    void Command::OutputHelp(Execution::Reporter& reporter, const CommandException* exception) const
    {
        OutputIntroHeader(reporter);
        reporter.EmptyLine();

        if (exception)
        {
            reporter.Error() <<
                exception->Message() << " : '" << exception->Param() << '\'' << std::endl <<
                std::endl;
        }

        auto infoOut = reporter.Info();
        infoOut <<
            GetLongDescription() << std::endl <<
            std::endl;

        // Example usage for this command, if applicable
        auto arguments = GetArguments();
        bool needsExePart = true;

        for (const auto& arg : arguments)
        {
            if (arg.Visibility() == Visibility::Example)
            {
                if (needsExePart)
                {
                    infoOut << "<exe>";
                    needsExePart = false;
                }

                infoOut << ' ';

                if (!arg.Required())
                {
                    infoOut << '[';
                }

                if (arg.Type() == ArgumentType::Positional)
                {
                    infoOut << '[';
                }

                if (arg.Alias() == APPINSTALLER_CLI_ARGUMENT_NO_SHORT_VER)
                {
                    infoOut << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Name();
                }
                else
                {
                    infoOut << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Alias();
                }

                if (arg.Type() == ArgumentType::Positional)
                {
                    infoOut << ']';
                }

                if (arg.Type() != ArgumentType::Flag)
                {
                    infoOut << " <" << arg.Name() << '>';
                }

                if (!arg.Required())
                {
                    infoOut << ']';
                }
            }
        }

        if (!needsExePart)
        {
            infoOut <<
                std::endl <<
                std::endl;
        }

        auto commands = GetCommands();
        if (!commands.empty())
        {
            if (Name() == FullName())
            {
                infoOut << LOCME("The following commands are available:") << std::endl;
            }
            else
            {
                infoOut << LOCME("The following sub-commands are available:") << std::endl;
            }

            // TODO: Create table output functionality in reporter
            for (const auto& command : commands)
            {
                infoOut << "  " << command->Name() << "  " << command->ShortDescription() << std::endl;
            }

            infoOut <<
                std::endl <<
                LOCME("For more details on a specific command, pass it the help argument.") << " [" << APPINSTALLER_CLI_HELP_ARGUMENT << ']' << std::endl;
        }

        if (!arguments.empty())
        {
            if (!commands.empty())
            {
                infoOut << std::endl;
            }

            infoOut << LOCME("The following arguments are available:") << std::endl;

            // TODO: Create table output functionality in reporter
            for (const auto& arg : GetArguments())
            {
                if (arg.Visibility() != Visibility::Hidden)
                {
                    infoOut << "  ";
                    if (arg.Alias() != APPINSTALLER_CLI_ARGUMENT_NO_SHORT_VER)
                    {
                        infoOut << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Alias() << ',';
                    }
                    infoOut << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Name() << "  " << arg.Description() << std::endl;
                }
            }
        }
    }

    std::unique_ptr<Command> Command::FindSubCommand(Invocation& inv) const
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
            if (Utility::CaseInsensitiveEquals(*itr, command->Name()))
            {
                AICLI_LOG(CLI, Info, << "Found subcommand: " << *itr);
                inv.consume(itr);
                return std::move(command);
            }
        }

        // TODO: If we get to a large number of commands, do a fuzzy search much like git
        throw CommandException(LOCME("Unrecognized command"), *itr);
    }

    // Parse arguments as such:
    //  1. If argument starts with a single -, only the single character alias is considered.
    //      a. If the named argument alias (a) needs a VALUE, it can be provided in these ways:
    //          -aVALUE
    //          -a VALUE
    //      b. If the argument is a flag, additional characters after are treated as if they start
    //          with a -, repeatedly until the end of the argument is reached.  Fails if non-flags hit.
    //  2. If the argument starts with a double --, only the full name is considered.
    //      a. If the named argument (arg) needs a VALUE, it can be provided in these ways:
    //          --arg=VALUE
    //          --arg VALUE
    //  3. If the argument does not start with any -, it is considered the next positional argument.
    //  4. If the argument is only a double --, all further arguments are only considered as positional.
    void Command::ParseArguments(Invocation& inv, Execution::Args& execArgs) const
    {
        auto definedArgs = GetArguments();
        Argument::GetCommon(definedArgs);
        auto positionalSearchItr = definedArgs.begin();

        // The user can override processing '-blah' as an argument name by passing '--'.
        bool onlyPositionalArgsRemain = false;

        for (auto incomingArgsItr = inv.begin(); incomingArgsItr != inv.end(); ++incomingArgsItr)
        {
            const std::string& currArg = *incomingArgsItr;

            if (onlyPositionalArgsRemain || currArg.empty() || currArg[0] != APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR)
            {
                // Positional argument, find the next appropriate one if the current itr isn't one or has hit its limit.
                if (positionalSearchItr != definedArgs.end() &&
                    (positionalSearchItr->Type() != ArgumentType::Positional || execArgs.GetCount(positionalSearchItr->ExecArgType()) == positionalSearchItr->Limit()))
                {
                    for (++positionalSearchItr; positionalSearchItr != definedArgs.end() && positionalSearchItr->Type() != ArgumentType::Positional; ++positionalSearchItr);
                }

                if (positionalSearchItr == definedArgs.end())
                {
                    throw CommandException(LOCME("Found a positional argument when none was expected"), currArg);
                }

                execArgs.AddArg(positionalSearchItr->ExecArgType(), currArg);
            }
            // The currentArg must not be empty, and starts with a -
            else if (currArg.length() == 1)
            {
                throw CommandException(LOCME("Invalid argument specifier"), currArg);
            }
            // Now it must be at least 2 chars
            else if (currArg[1] != APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR)
            {
                // Parse the single character alias argument
                char currChar = currArg[1];

                auto itr = std::find_if(definedArgs.begin(), definedArgs.end(), [&](const Argument& arg) { return (currChar == arg.Alias()); });
                if (itr == definedArgs.end())
                {
                    throw CommandException(LOCME("Argument alias was not recognized for the current command"), currArg);
                }

                if (itr->Type() == ArgumentType::Flag)
                {
                    execArgs.AddArg(itr->ExecArgType());

                    for (size_t i = 2; i < currArg.length(); ++i)
                    {
                        currChar = currArg[i];

                        auto itr2 = std::find_if(definedArgs.begin(), definedArgs.end(), [&](const Argument& arg) { return (currChar == arg.Alias()); });
                        if (itr2 == definedArgs.end())
                        {
                            throw CommandException(LOCME("Adjoined flag alias not found"), currArg);
                        }
                        else if (itr2->Type() != ArgumentType::Flag)
                        {
                            throw CommandException(LOCME("Adjoined alias is not a flag"), currArg);
                        }
                        else
                        {
                            execArgs.AddArg(itr2->ExecArgType());
                        }
                    }
                }
                else if (currArg.length() > 2)
                {
                    execArgs.AddArg(itr->ExecArgType(), currArg.substr(2));
                }
                else
                {
                    ++incomingArgsItr;
                    if (incomingArgsItr == inv.end())
                    {
                        throw CommandException(LOCME("Argument value required, but none found"), currArg);
                    }
                    execArgs.AddArg(itr->ExecArgType(), *incomingArgsItr);
                }
            }
            // The currentArg is at least 2 chars, both of which are --
            else if (currArg.length() == 2)
            {
                onlyPositionalArgsRemain = true;
            }
            // The currentArg is more than 2 chars, both of which are --
            else
            {
                // This is an arg name, find it and process its value if needed.
                // Skip the double arg identifier chars.
                std::string argName = currArg.substr(2);
                bool argFound = false;

                bool hasValue = false;
                std::string argValue;
                size_t splitChar = argName.find_first_of(APPINSTALLER_CLI_ARGUMENT_SPLIT_CHAR);
                if (splitChar != std::string::npos)
                {
                    hasValue = true;
                    argValue = argName.substr(splitChar + 1);
                    argName.resize(splitChar);
                }

                for (const auto& arg : definedArgs)
                {
                    if (Utility::CaseInsensitiveEquals(argName, arg.Name()))
                    {
                        if (arg.Type() == ArgumentType::Flag)
                        {
                            if (hasValue)
                            {
                                throw CommandException(LOCME("Flag argument cannot contain adjoined value"), currArg);
                            }

                            execArgs.AddArg(arg.ExecArgType());
                        }
                        else if (hasValue)
                        {
                            execArgs.AddArg(arg.ExecArgType(), std::move(argValue));
                        }
                        else
                        {
                            ++incomingArgsItr;
                            if (incomingArgsItr == inv.end())
                            {
                                throw CommandException(LOCME("Argument value required, but none found"), currArg);
                            }
                            execArgs.AddArg(arg.ExecArgType(), *incomingArgsItr);
                        }
                        argFound = true;
                        break;
                    }
                }

                if (!argFound)
                {
                    throw CommandException(LOCME("Argument name was not recognized for the current command"), *incomingArgsItr);
                }
            }
        }
    }

    void Command::ValidateArguments(Execution::Args& execArgs) const
    {
        // If help is asked for, don't bother validating anything else
        if (execArgs.Contains(Execution::Args::Type::Help))
        {
            return;
        }

        ValidateArgumentsInternal(execArgs);
    }

    void Command::Execute(Execution::Context& context) const
    {
        AICLI_LOG(CLI, Info, << "Executing command: " << Name());
        if (context.Args.Contains(Execution::Args::Type::Help))
        {
            OutputHelp(context.Reporter);
        }
        else
        {
            ExecuteInternal(context);
        }
    }

    void Command::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        for (const auto& arg : GetArguments())
        {
            if (arg.Required() && !execArgs.Contains(arg.ExecArgType()))
            {
                throw CommandException(LOCME("Required argument not provided"), arg.Name());
            }

            if (arg.Limit() < execArgs.GetCount(arg.ExecArgType()))
            {
                throw CommandException(LOCME("Argument provided more times than allowed"), arg.Name());
            }
        }
    }

    void Command::ExecuteInternal(Execution::Context& context) const
    {
        context.Reporter.ShowMsg(LOCME("Oops, we forgot to do this..."), Execution::Reporter::Level::Error);
        THROW_HR(E_NOTIMPL);
    }
}
