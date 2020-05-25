// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Command.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace Utility::literals;

    Command::Command(std::string_view name, std::string_view parent) :
        m_name(name)
    {
        if (!parent.empty())
        {
            m_fullName.reserve(parent.length() + 1 + name.length());
            m_fullName = parent;
            m_fullName += ParentSplitChar;
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
            "Windows Package Manager v"_liv << Runtime::GetClientVersion() << ' ' << Resource::String::PreviewVersion << std::endl <<
            Resource::String::MainCopyrightNotice << std::endl;
    }

    void Command::OutputHelp(Execution::Reporter& reporter, const CommandException* exception) const
    {
        // Header
        OutputIntroHeader(reporter);
        reporter.EmptyLine();

        // Error if given
        if (exception)
        {
            reporter.Error() <<
                exception->Message() << " : '"_liv << exception->Param() << '\'' << std::endl <<
                std::endl;
        }

        // Description
        auto infoOut = reporter.Info();
        infoOut <<
            LongDescription() << std::endl <<
            std::endl;

        // Example usage for this command
        // First create the command chain for output
        std::string commandChain = FullName();
        size_t firstSplit = commandChain.find_first_of(ParentSplitChar);
        if (firstSplit == std::string::npos)
        {
            commandChain.clear();
        }
        else
        {
            commandChain = commandChain.substr(firstSplit);
            for (char& c : commandChain)
            {
                if (c == ParentSplitChar)
                {
                    c = ' ';
                }
            }
        }

        // Output the command preamble and command chain
        infoOut << Resource::String::Usage << ": winget"_liv << Utility::LocIndView{ commandChain };

        auto commands = GetCommands();
        auto arguments = GetArguments();

        bool hasArguments = false;
        bool hasOptions = false;

        // Output the command token, made optional if arguments are present.
        if (!commands.empty())
        {
            infoOut << ' ';

            if (!arguments.empty())
            {
                infoOut << '[';
            }

            infoOut << '<' << Resource::String::Command << '>';

            if (!arguments.empty())
            {
                infoOut << ']';
            }
        }

        // Arguments are required by a test to have all positionals first.
        for (const auto& arg : arguments)
        {
            if (arg.Type() == ArgumentType::Positional)
            {
                hasArguments = true;

                infoOut << ' ';

                if (!arg.Required())
                {
                    infoOut << '[';
                }

                infoOut << '[';

                if (arg.Alias() == APPINSTALLER_CLI_ARGUMENT_NO_SHORT_VER)
                {
                    infoOut << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Name();
                }
                else
                {
                    infoOut << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Alias();
                }

                infoOut << "] <"_liv << arg.Name() << '>';

                if (!arg.Required())
                {
                    infoOut << ']';
                }
            }
            else
            {
                hasOptions = true;
                infoOut << " [<"_liv << Resource::String::Options << ">]"_liv;
                break;
            }
        }

        infoOut <<
            std::endl <<
            std::endl;

        if (!commands.empty())
        {
            if (Name() == FullName())
            {
                infoOut << Resource::String::AvailableCommands << std::endl;
            }
            else
            {
                infoOut << Resource::String::AvailableSubcommands << std::endl;
            }

            size_t maxCommandNameLength = 0;
            for (const auto& command : commands)
            {
                maxCommandNameLength = std::max(maxCommandNameLength, command->Name().length());
            }

            for (const auto& command : commands)
            {
                size_t fillChars = (maxCommandNameLength - command->Name().length()) + 2;
                infoOut << "  "_liv << Execution::HelpCommandEmphasis << command->Name() << Utility::LocIndString{ std::string(fillChars, ' ') } << command->ShortDescription() << std::endl;
            }

            infoOut <<
                std::endl <<
                Resource::String::HelpForDetails
                << " ["_liv << APPINSTALLER_CLI_HELP_ARGUMENT << ']' << std::endl;
        }

        if (!arguments.empty())
        {
            if (!commands.empty())
            {
                infoOut << std::endl;
            }

            std::vector<std::string> argNames;
            size_t maxArgNameLength = 0;
            for (const auto& arg : GetArguments())
            {
                if (arg.Visibility() != Visibility::Hidden)
                {
                    std::ostringstream strstr;
                    if (arg.Alias() != APPINSTALLER_CLI_ARGUMENT_NO_SHORT_VER)
                    {
                        strstr << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Alias() << ',';
                    }
                    strstr << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Name();

                    argNames.emplace_back(strstr.str());
                    maxArgNameLength = std::max(maxArgNameLength, argNames.back().length());
                }
            }

            if (hasArguments)
            {
                infoOut << Resource::String::AvailableArguments << std::endl;

                size_t i = 0;
                for (const auto& arg : GetArguments())
                {
                    if (arg.Visibility() != Visibility::Hidden)
                    {
                        const std::string& argName = argNames[i++];
                        if (arg.Type() == ArgumentType::Positional)
                        {
                            size_t fillChars = (maxArgNameLength - argName.length()) + 2;
                            infoOut << "  "_liv << Execution::HelpArgumentEmphasis << argName << Utility::LocIndString{ std::string(fillChars, ' ') } << arg.Description() << std::endl;
                        }
                    }
                }
            }

            if (hasOptions)
            {
                if (hasArguments)
                {
                    infoOut << std::endl;
                }

                infoOut << Resource::String::AvailableOptions << std::endl;

                size_t i = 0;
                for (const auto& arg : GetArguments())
                {
                    if (arg.Visibility() != Visibility::Hidden)
                    {
                        const std::string& argName = argNames[i++];
                        if (arg.Type() != ArgumentType::Positional)
                        {
                            size_t fillChars = (maxArgNameLength - argName.length()) + 2;
                            infoOut << "  "_liv << Execution::HelpArgumentEmphasis << argName << Utility::LocIndString{ std::string(fillChars, ' ') } << arg.Description() << std::endl;
                        }
                    }
                }
            }
        }

        // Finally, the link to the documentation pages
        std::string helpLink = HelpLink();
        if (!helpLink.empty())
        {
            infoOut << std::endl << Resource::String::HelpLinkPreamble << ' ' << helpLink << std::endl;
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
        throw CommandException(Resource::String::UnrecognizedCommand, *itr);
    }

    // Parse arguments as such:
    //  1. If argument starts with a single -, only the single character alias is considered.
    //      a. If the named argument alias (a) needs a VALUE, it can be provided in these ways:
    //          -a=VALUE
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
                    throw CommandException(Resource::String::ExtraPositionalError, currArg);
                }

                execArgs.AddArg(positionalSearchItr->ExecArgType(), currArg);
            }
            // The currentArg must not be empty, and starts with a -
            else if (currArg.length() == 1)
            {
                throw CommandException(Resource::String::InvalidArgumentSpecifierError, currArg);
            }
            // Now it must be at least 2 chars
            else if (currArg[1] != APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR)
            {
                // Parse the single character alias argument
                char currChar = currArg[1];

                auto itr = std::find_if(definedArgs.begin(), definedArgs.end(), [&](const Argument& arg) { return (currChar == arg.Alias()); });
                if (itr == definedArgs.end())
                {
                    throw CommandException(Resource::String::InvalidAliasError, currArg);
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
                            throw CommandException(Resource::String::AdjoinedNotFoundError, currArg);
                        }
                        else if (itr2->Type() != ArgumentType::Flag)
                        {
                            throw CommandException(Resource::String::AdjoinedNotFlagError, currArg);
                        }
                        else
                        {
                            execArgs.AddArg(itr2->ExecArgType());
                        }
                    }
                }
                else if (currArg.length() > 2)
                {
                    if (currArg[2] == APPINSTALLER_CLI_ARGUMENT_SPLIT_CHAR)
                    {
                        execArgs.AddArg(itr->ExecArgType(), currArg.substr(3));
                    }
                    else
                    {
                        throw CommandException(Resource::String::SingleCharAfterDashError, currArg);
                    }
                }
                else
                {
                    ++incomingArgsItr;
                    if (incomingArgsItr == inv.end())
                    {
                        throw CommandException(Resource::String::MissingArgumentError, currArg);
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
                                throw CommandException(Resource::String::FlagContainAdjoinedError, currArg);
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
                                throw CommandException(Resource::String::MissingArgumentError, currArg);
                            }
                            execArgs.AddArg(arg.ExecArgType(), *incomingArgsItr);
                        }
                        argFound = true;
                        break;
                    }
                }

                if (!argFound)
                {
                    throw CommandException(Resource::String::InvalidNameError, *incomingArgsItr);
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
                throw CommandException(Resource::String::RequiredArgError, arg.Name());
            }

            if (arg.Limit() < execArgs.GetCount(arg.ExecArgType()))
            {
                throw CommandException(Resource::String::TooManyArgError, arg.Name());
            }
        }
    }

    void Command::ExecuteInternal(Execution::Context& context) const
    {
        context.Reporter.Error() << Resource::String::PendingWorkError << std::endl;
        THROW_HR(E_NOTIMPL);
    }
}
