// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Command.h"
#include "Resources.h"
#include <winget/UserSettings.h>

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace Utility::literals;
    using namespace Settings;

    Command::Command(std::string_view name, std::string_view parent, Command::Visibility visibility, ExperimentalFeature::Feature feature) :
        m_name(name), m_visibility(visibility), m_feature(feature)
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

        auto commands = GetVisibleCommands();
        auto arguments = GetVisibleArguments();

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

                if (arg.Alias() == Argument::NoAlias)
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
            for (const auto& arg : arguments)
            {
                std::ostringstream strstr;
                if (arg.Alias() != Argument::NoAlias)
                {
                    strstr << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Alias() << ',';
                }
                strstr << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Name();

                argNames.emplace_back(strstr.str());
                maxArgNameLength = std::max(maxArgNameLength, argNames.back().length());
            }

            if (hasArguments)
            {
                infoOut << Resource::String::AvailableArguments << std::endl;

                size_t i = 0;
                for (const auto& arg : arguments)
                {
                    const std::string& argName = argNames[i++];
                    if (arg.Type() == ArgumentType::Positional)
                    {
                        size_t fillChars = (maxArgNameLength - argName.length()) + 2;
                        infoOut << "  "_liv << Execution::HelpArgumentEmphasis << argName << Utility::LocIndString{ std::string(fillChars, ' ') } << arg.Description() << std::endl;
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
                for (const auto& arg : arguments)
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
                if (!ExperimentalFeature::IsEnabled(command->Feature()))
                {
                    auto feature = ExperimentalFeature::GetFeature(command->Feature());
                    AICLI_LOG(CLI, Error, << "Trying to use command: " << *itr << " without enabling feature " << feature.JsonName());
                    throw CommandException(Resource::String::FeatureDisabledMessage, feature.JsonName());
                }

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

    void Command::Complete(Execution::Context& context, CompletionData& data) const
    {
        // The word we are to complete is directly after the command.
        if (data.BeforeWord().begin() == data.BeforeWord().end())
        {
            // TODO: MORE STUFF / filtering
            for (const auto& command : GetCommands())
            {
                context.Reporter.Info() << command->Name() << std::endl;
            }
        }
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
            if (!ExperimentalFeature::IsEnabled(arg.Feature()) && execArgs.Contains(arg.ExecArgType()))
            {
                auto feature = ExperimentalFeature::GetFeature(arg.Feature());
                AICLI_LOG(CLI, Error, << "Trying to use argument: " << arg.Name() << " without enabling feature " << feature.JsonName());
                throw CommandException(Resource::String::FeatureDisabledMessage, feature.JsonName());
            }

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

    Command::Visibility Command::GetVisibility() const
    {
        if (!ExperimentalFeature::IsEnabled(m_feature))
        {
            return Command::Visibility::Hidden;
        }

        return m_visibility;
    }

    std::vector<std::unique_ptr<Command>> Command::GetVisibleCommands() const
    {
        auto commands = GetCommands();

        commands.erase(
            std::remove_if(
                commands.begin(), commands.end(),
                [](const std::unique_ptr<Command>& command) { return command->GetVisibility() == Command::Visibility::Hidden; }),
            commands.end());

        return commands;
    }

    std::vector<Argument> Command::GetVisibleArguments() const
    {
        auto arguments = GetArguments();

        arguments.erase(
            std::remove_if(
                arguments.begin(), arguments.end(),
                [](const Argument& arg) { return arg.GetVisibility() == Argument::Visibility::Hidden; }),
            arguments.end());

        return arguments;
    }

    // Completion takes in the following values:
    //  Word :: The token from the command line that is being targeted for completion.
    //          This value may have quotes surrounding it, and will need to be removed in such a case.
    //  CommandLine :: The full command line that contains the word to be completed.
    //                 This value has the fully quoted strings, as well as escaped quotations if needed.
    //  Position :: The position of the cursor within the command line.
    //
    // Completions here will not attempt to take exact cursor position into account; meaning if the cursor
    // is in the middle of the word, it is not different than at the beginning or end. This functionality
    // could be added later.
    CompletionData::CompletionData(std::string_view word, std::string_view commandLine, std::string_view position)
    {
        // Prepare the word to complete. It may come with quotes around it; if so, remove them.
        if (word.length() >= 2 && word[0] == '"' && word[word.length() - 1] == '"')
        {
            m_word = word.substr(1, word.length() - 2);
        }
        else
        {
            m_word = word;
        }

        AICLI_LOG(CLI, Info, << "Completing word '" << m_word << '\'');

        // Determine position as an integer
        m_position = std::stoull(std::string{ position });

        AICLI_LOG(CLI, Info, << "Cursor position starts at '" << m_position << '\'');

        std::vector<std::string> argsBeforeWord;
        std::vector<std::string> argsAfterWord;

        // If the word is empty, we must determine where the split is. We operate as PowerShell does; the cursor
        // being at the front of a token results in an empty word and an insertion rather than a replacement.
        // If the user put spaces at the front of the statement, this can lead to the position being out of sorts;
        // PowerShell sends the cursor position, but does not include leading spaces in the AST output. If the
        // user puts too many spaces at the front we will be unable to determine the true location.
        if (m_word.empty())
        {
            // The cursor is past the end, so everything is before the word.
            if (m_position > commandLine.length())
            {
                ParseInto(commandLine, argsBeforeWord, true);
            }
            // The cursor is not past the end; ensure that the preceding character is whitespace or move the
            // position back until it is. This is far from foolproof, but until we have evidence otherwise,
            // very few users are likely to put any spaces at the front of their statements, let alone many.
            else
            {
                for (; m_position > 0 && !std::isspace(commandLine[m_position - 1]); --m_position);

                AICLI_LOG(CLI, Info, << "Cursor position moved to '" << m_position << '\'');

                // If we actually hit the front of the string, something bad probably happened.
                THROW_HR_IF(E_UNEXPECTED, m_position == 0);

                ParseInto(commandLine.substr(0, m_position), argsBeforeWord, true);
                ParseInto(commandLine.substr(m_position), argsAfterWord, false);
            }
        }
        // If the word is not empty, the cursor is either in the middle of a token, or at the end of one.
        // The value will be replaced, and we will remove it from the args here.
        else
        {
            std::vector<std::string> allArgs;
            ParseInto(commandLine, allArgs, true);

            // Find the word amongst the arguments
            std::vector<size_t> wordIndeces;
            for (size_t i = 0; i < allArgs.size(); ++i)
            {
                if (word == allArgs[i])
                {
                    wordIndeces.push_back(i);
                }
            }

            // If we didn't find a matching string, we probably made some bad assumptions.
            THROW_HR_IF(E_UNEXPECTED, wordIndeces.empty());

            // If we find an exact match only once, we can just split on that.
            size_t wordIndexForSplit = wordIndeces[0];

            // If we found more than one match, we have to rely on the position to
            // determine which argument is the word in question.
            if (wordIndeces.size() > 1)
            {
                // Escape the word and search for it in the command line.
                std::string escapedWord = m_word;
                Utility::FindAndReplace(escapedWord, "\"", "\"\"");

                std::vector<size_t> escapedIndeces;
                for (size_t offset = 0; offset < commandLine.length();)
                {
                    size_t pos = commandLine.find(escapedWord, offset);

                    if (pos == std::string::npos)
                    {
                        break;
                    }

                    escapedIndeces.push_back(pos);
                    offset = pos + escapedWord.length();
                }

                // If these are out of sync we don't have much hope.
                THROW_HR_IF(E_UNEXPECTED, wordIndeces.size() != escapedIndeces.size());

                // Find the closest one to the position. This can be fooled as above if there is
                // leading whitespace in the statement. But it is the best we can do.
                size_t indexToUse = std::numeric_limits<size_t>::max();
                size_t distanceToCursor = std::numeric_limits<size_t>::max();

                for (size_t i = 0; i < escapedIndeces.size(); ++i)
                {
                    size_t lowerBound = escapedIndeces[i];
                    size_t upperBound = lowerBound + escapedWord.length();
                    size_t distance = 0;

                    // The cursor is square in the middle of this location, this is the one.
                    if (m_position > lowerBound && m_position <= upperBound)
                    {
                        indexToUse = i;
                        break;
                    }
                    else if (m_position <= lowerBound)
                    {
                        distance = lowerBound - m_position;
                    }
                    else // m_position > upperBound
                    {
                        distance = m_position - upperBound;
                    }

                    if (distance < distanceToCursor)
                    {
                        indexToUse = i;
                        distanceToCursor = distance;
                    }
                }

                // It really would be unexpected to not find a closest one.
                THROW_HR_IF(E_UNEXPECTED, indexToUse == std::numeric_limits<size_t>::max());

                wordIndexForSplit = wordIndeces[indexToUse];
            }

            std::vector<std::string>* moveTarget = &argsBeforeWord;
            for (size_t i = 0; i < allArgs.size(); ++i)
            {
                if (i == wordIndexForSplit)
                {
                    // Intentionally leave the matched arg behind.
                    moveTarget = &argsAfterWord;
                }
                else
                {
                    moveTarget->emplace_back(std::move(allArgs[i]));
                }
            }
        }

        // Move the arguments into an Invocation for future use.
        m_argsBeforeWord = std::make_unique<CLI::Invocation>(std::move(argsBeforeWord));
        m_argsAfterWord = std::make_unique<CLI::Invocation>(std::move(argsAfterWord));

        AICLI_LOG(CLI, Info, << "Completion invoked for arguments:" << [&]() {
            std::stringstream strstr;
            for (const auto& arg : *m_argsBeforeWord)
            {
                strstr << " '" << arg << '\'';
            }
            if (m_word.empty())
            {
                strstr << " << [insert] >> ";
            }
            else
            {
                strstr << " << [replace] '" << m_word << "' >> ";
            }
            for (const auto& arg : *m_argsAfterWord)
            {
                strstr << " '" << arg << '\'';
            }
            return strstr.str();
            }());
    }

    std::unique_ptr<Command> CompletionData::FindCommand(std::unique_ptr<Command>&& root)
    {
        std::unique_ptr<Command> result = std::move(root);

        std::unique_ptr<Command> subCommand = result->FindSubCommand(BeforeWord());
        while (subCommand)
        {
            result = std::move(subCommand);
            subCommand = result->FindSubCommand(BeforeWord());
        }

        return result;
    }

    void CompletionData::ParseInto(std::string_view line, std::vector<std::string>& args, bool skipFirst)
    {
        std::wstring commandLineW = Utility::ConvertToUTF16(line);
        int argc = 0;
        wil::unique_hlocal_ptr<LPWSTR> argv{ CommandLineToArgvW(commandLineW.c_str(), &argc) };
        THROW_LAST_ERROR_IF_NULL(argv);

        for (int i = (skipFirst ? 1 : 0); i < argc; ++i)
        {
            args.emplace_back(Utility::ConvertToUTF8(argv.get()[i]));
        }
    }
}
