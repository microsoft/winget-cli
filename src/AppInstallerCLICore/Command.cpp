// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Command.h"
#include "Resources.h"
#include "Sixel.h"
#include <winget/UserSettings.h>
#include <AppInstallerRuntime.h>
#include <winget/Locale.h>
#include <winget/Reboot.h>
#include <winget/Authentication.h>

using namespace std::string_view_literals;
using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::Settings;

namespace AppInstaller::CLI
{
    namespace
    {
        constexpr Utility::LocIndView s_Command_ArgName_SilentAndInteractive = "silent|interactive"_liv;

        void LaunchLogsIfRequested(Execution::Context& context)
        {
            try
            {
                if (context.Args.Contains(Execution::Args::Type::OpenLogs))
                {
                    // TODO: Consider possibly adding functionality that if the context contains 'Execution::Args::Type::Log' to open the path provided for the log
                    // The above was omitted initially as a security precaution to ensure that user input to '--log' wouldn't be passed directly to ShellExecute
                    ShellExecute(NULL, NULL, Runtime::GetPathTo(Runtime::PathName::DefaultLogLocation).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
            }
            CATCH_LOG();
        }
    }

    Command::Command(
        std::string_view name,
        std::vector<std::string_view> aliases,
        std::string_view parent,
        Command::Visibility visibility,
        Settings::ExperimentalFeature::Feature feature,
        Settings::TogglePolicy::Policy groupPolicy,
        CommandOutputFlags outputFlags) :
        m_name(name), m_aliases(std::move(aliases)), m_visibility(visibility), m_feature(feature), m_groupPolicy(groupPolicy), m_outputFlags(outputFlags)
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
        auto infoOut = reporter.Info();
        VirtualTerminal::ConstructedSequence indent;

        if (reporter.SixelsEnabled())
        {
            try
            {
                std::filesystem::path imagePath = Runtime::GetPathTo(Runtime::PathName::ImageAssets);

                if (!imagePath.empty())
                {
                    // This image matches the target pixel size. If changing the target size, choose the most appropriate image.
                    imagePath /= "AppList.targetsize-40.png";

                    VirtualTerminal::Sixel::Image wingetIcon{ imagePath };

                    // Using a height of 2 to match the two lines of header.
                    UINT imageHeightCells = 2;
                    UINT imageWidthCells = 2 * imageHeightCells;

                    wingetIcon.RenderSizeInCells(imageWidthCells, imageHeightCells);
                    wingetIcon.RenderTo(infoOut);

                    indent = VirtualTerminal::Cursor::Position::Forward(static_cast<int16_t>(imageWidthCells));
                    infoOut << VirtualTerminal::Cursor::Position::Up(static_cast<int16_t>(imageHeightCells) - 1);
                }
            }
            CATCH_LOG();
        }

        auto productName = Runtime::IsReleaseBuild() ? Resource::String::WindowsPackageManager : Resource::String::WindowsPackageManagerPreview;
        infoOut << indent << productName(Runtime::GetClientVersion()) << std::endl
            << indent << Resource::String::MainCopyrightNotice << std::endl;
    }

    void Command::OutputHelp(Execution::Reporter& reporter, const CommandException* exception) const
    {
        // Header
        OutputIntroHeader(reporter);
        reporter.EmptyLine();

        // Error if given
        if (exception)
        {
            reporter.Error() << exception->Message() << std::endl << std::endl;
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
            commandChain = commandChain.substr(firstSplit + 1);
            for (char& c : commandChain)
            {
                if (c == ParentSplitChar)
                {
                    c = ' ';
                }
            }
        }

        // Output the command preamble and command chain
        infoOut << Resource::String::Usage("winget"_liv, Utility::LocIndView{ commandChain });

        auto commandAliases = Aliases();
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

                if (arg.Alias() == ArgumentCommon::NoAlias)
                {
                    infoOut << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Name();
                }
                else
                {
                    infoOut << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Alias();
                }

                infoOut << "] <"_liv << arg.Name() << '>';

                if (arg.Limit() > 1)
                {
                    infoOut << "..."_liv;
                }

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

        if (!commandAliases.empty())
        {
            infoOut << Resource::String::AvailableCommandAliases << std::endl;
            
            for (const auto& commandAlias : commandAliases)
            {
                infoOut << "  "_liv << Execution::HelpCommandEmphasis << commandAlias << std::endl;
            }
            infoOut << std::endl;
        }

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
                argNames.emplace_back(arg.GetUsageString());
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
        auto helpLink = HelpLink();
        if (!helpLink.empty())
        {
            infoOut << std::endl << Resource::String::HelpLinkPreamble(helpLink) << std::endl;
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
            if (
                Utility::CaseInsensitiveEquals(*itr, command->Name()) ||
                Utility::CaseInsensitiveContains(command->Aliases(), *itr)
            )
            {
                if (!ExperimentalFeature::IsEnabled(command->Feature()))
                {
                    auto feature = ExperimentalFeature::GetFeature(command->Feature());
                    AICLI_LOG(CLI, Error, << "Trying to use command: " << *itr << " without enabling feature " << feature.JsonName());
                    throw CommandException(Resource::String::FeatureDisabledMessage(feature.JsonName()));
                }

                if (!Settings::GroupPolicies().IsEnabled(command->GroupPolicy()))
                {
                    auto policy = TogglePolicy::GetPolicy(command->GroupPolicy());
                    AICLI_LOG(CLI, Error, << "Trying to use command: " << *itr << " disabled by group policy " << policy.RegValueName());
                    throw GroupPolicyException(command->GroupPolicy());
                }

                AICLI_LOG(CLI, Info, << "Found subcommand: " << *itr);
                inv.consume(itr);
                return std::move(command);
            }
        }

        // The command has opted-in to be executed when it has subcommands and the next token is a positional parameter value
        if (m_selectCurrentCommandIfUnrecognizedSubcommandFound)
        {
            return {};
        }

        // TODO: If we get to a large number of commands, do a fuzzy search much like git
        throw CommandException(Resource::String::UnrecognizedCommand(Utility::LocIndView{ *itr }));
    }

    // The argument parsing state machine.
    // It is broken out to enable completion to process arguments, ignore errors,
    // and determine the likely state of the word to be completed.
    struct ParseArgumentsStateMachine
    {
        ParseArgumentsStateMachine(Invocation& inv, Execution::Args& execArgs, std::vector<CLI::Argument> arguments);

        ParseArgumentsStateMachine(const ParseArgumentsStateMachine&) = delete;
        ParseArgumentsStateMachine& operator=(const ParseArgumentsStateMachine&) = delete;

        ParseArgumentsStateMachine(ParseArgumentsStateMachine&&) = default;
        ParseArgumentsStateMachine& operator=(ParseArgumentsStateMachine&&) = default;

        // Processes the next argument from the invocation.
        // Returns true if there was an argument to process;
        // returns false if there were none.
        bool Step();

        // Throws if there was an error during the prior step.
        void ThrowIfError() const;

        // The current state of the state machine.
        // An empty state indicates that the next argument can be anything.
        struct State
        {
            State() = default;
            State(Execution::Args::Type type, std::string_view arg) : m_type(type), m_arg(arg) {}
            State(CommandException ce) : m_exception(std::move(ce)) {}

            // If set, indicates that the next argument is a value for this type.
            const std::optional<Execution::Args::Type>& Type() const { return m_type; }

            // The actual argument string associated with Type.
            const Utility::LocIndString& Arg() const { return m_arg; }

            // If set, indicates that the last argument produced an error.
            const std::optional<CommandException>& Exception() const { return m_exception; }

        private:
            std::optional<Execution::Args::Type> m_type;
            Utility::LocIndString m_arg;
            std::optional<CommandException> m_exception;
        };

        const State& GetState() const { return m_state; }

        bool OnlyPositionalRemain() const { return m_onlyPositionalArgumentsRemain; }

        // Gets the next positional argument, or nullptr if there is not one.
        const CLI::Argument* NextPositional();

        const std::vector<CLI::Argument>& Arguments() const { return m_arguments; }

    private:
        State StepInternal();

        void ProcessAdjoinedValue(Execution::Args::Type type, std::string_view value);

        Invocation& m_invocation;
        Execution::Args& m_executionArgs;
        std::vector<CLI::Argument> m_arguments;

        Invocation::iterator m_invocationItr;
        std::vector<CLI::Argument>::iterator m_positionalSearchItr;
        bool m_onlyPositionalArgumentsRemain = false;

        State m_state;
    };

    ParseArgumentsStateMachine::ParseArgumentsStateMachine(Invocation& inv, Execution::Args& execArgs, std::vector<CLI::Argument> arguments) :
        m_invocation(inv),
        m_executionArgs(execArgs),
        m_arguments(std::move(arguments)),
        m_invocationItr(m_invocation.begin()),
        m_positionalSearchItr(m_arguments.begin())
    {
    }

    bool ParseArgumentsStateMachine::Step()
    {
        if (m_invocationItr == m_invocation.end())
        {
            return false;
        }

        m_state = StepInternal();
        return true;
    }

    void ParseArgumentsStateMachine::ThrowIfError() const
    {
        if (m_state.Exception())
        {
            throw m_state.Exception().value();
        }
        // If the next argument was to be a value, but none was provided, convert it to an exception.
        else if (m_state.Type() && m_invocationItr == m_invocation.end())
        {
            throw CommandException(Resource::String::MissingArgumentError(m_state.Arg()));
        }
    }

    const CLI::Argument* ParseArgumentsStateMachine::NextPositional()
    {
        // Find the next appropriate positional arg if the current itr isn't one or has hit its limit.
        while (m_positionalSearchItr != m_arguments.end() &&
            (m_positionalSearchItr->Type() != ArgumentType::Positional || m_executionArgs.GetCount(m_positionalSearchItr->ExecArgType()) == m_positionalSearchItr->Limit()))
        {
            ++m_positionalSearchItr;
        }

        if (m_positionalSearchItr == m_arguments.end())
        {
            return nullptr;
        }

        return &*m_positionalSearchItr;
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
    ParseArgumentsStateMachine::State ParseArgumentsStateMachine::StepInternal()
    {
        auto currArg = Utility::LocIndView{ *m_invocationItr };
        ++m_invocationItr;

        // If the previous step indicated a value was needed, set it and forget it.
        if (m_state.Type())
        {
            m_executionArgs.AddArg(m_state.Type().value(), currArg);
            return {};
        }

        // This is a positional argument
        if (m_onlyPositionalArgumentsRemain || currArg.empty() || currArg[0] != APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR)
        {
            const CLI::Argument* nextPositional = NextPositional();
            if (!nextPositional)
            {
                return CommandException(Resource::String::ExtraPositionalError(currArg));
            }

            m_executionArgs.AddArg(nextPositional->ExecArgType(), currArg);
        }
        // The currentArg must not be empty, and starts with a -
        else if (currArg.length() == 1)
        {
            return CommandException(Resource::String::InvalidArgumentSpecifierError(currArg));
        }
        // Now it must be at least 2 chars
        else if (currArg[1] != APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR)
        {
            // Parse the single character alias argument
            char currChar = currArg[1];

            auto itr = std::find_if(m_arguments.begin(), m_arguments.end(), [&](const Argument& arg) { return (currChar == arg.Alias()); });
            if (itr == m_arguments.end())
            {
                return CommandException(Resource::String::InvalidAliasError(currArg));
            }

            if (itr->Type() == ArgumentType::Flag)
            {
                m_executionArgs.AddArg(itr->ExecArgType());

                for (size_t i = 2; i < currArg.length(); ++i)
                {
                    currChar = currArg[i];

                    auto itr2 = std::find_if(m_arguments.begin(), m_arguments.end(), [&](const Argument& arg) { return (currChar == arg.Alias()); });
                    if (itr2 == m_arguments.end())
                    {
                        return CommandException(Resource::String::AdjoinedNotFoundError(currArg));
                    }
                    else if (itr2->Type() != ArgumentType::Flag)
                    {
                        return CommandException(Resource::String::AdjoinedNotFlagError(currArg));
                    }
                    else
                    {
                        m_executionArgs.AddArg(itr2->ExecArgType());
                    }
                }
            }
            else if (currArg.length() > 2)
            {
                if (currArg[2] == APPINSTALLER_CLI_ARGUMENT_SPLIT_CHAR)
                {
                    ProcessAdjoinedValue(itr->ExecArgType(), currArg.substr(3));
                }
                else
                {
                    return CommandException(Resource::String::SingleCharAfterDashError(currArg));
                }
            }
            else
            {
                return { itr->ExecArgType(), currArg };
            }
        }
        // The currentArg is at least 2 chars, both of which are --
        else if (currArg.length() == 2)
        {
            m_onlyPositionalArgumentsRemain = true;
        }
        // The currentArg is more than 2 chars, both of which are --
        else
        {
            // This is an arg name, find it and process its value if needed.
            // Skip the double arg identifier chars.
            size_t argStart = currArg.find_first_not_of(APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR);
            std::string_view argName = currArg.substr(argStart);
            bool argFound = false;

            bool hasValue = false;
            std::string_view argValue;
            size_t splitChar = argName.find_first_of(APPINSTALLER_CLI_ARGUMENT_SPLIT_CHAR);
            if (splitChar != std::string::npos)
            {
                hasValue = true;
                argValue = argName.substr(splitChar + 1);
                argName = argName.substr(0, splitChar);
            }

            for (const auto& arg : m_arguments)
            {
                if (
                    Utility::CaseInsensitiveEquals(argName, arg.Name()) ||
                    Utility::CaseInsensitiveEquals(argName, arg.AlternateName())
                   )
                {
                    if (arg.Type() == ArgumentType::Flag)
                    {
                        if (hasValue)
                        {
                            return CommandException(Resource::String::FlagContainAdjoinedError(currArg));
                        }

                        m_executionArgs.AddArg(arg.ExecArgType());
                    }
                    else if (hasValue)
                    {
                        ProcessAdjoinedValue(arg.ExecArgType(), argValue);
                    }
                    else
                    {
                        return { arg.ExecArgType(), currArg };
                    }
                    argFound = true;
                    break;
                }
            }

            if (!argFound)
            {
                return CommandException(Resource::String::InvalidNameError(currArg));
            }
        }

        // If we get here, the next argument can be anything again.
        return {};
    }

    void ParseArgumentsStateMachine::ProcessAdjoinedValue(Execution::Args::Type type, std::string_view value)
    {
        // If the adjoined value is wrapped in quotes, strip them off.
        if (value.length() >= 2 && value[0] == '"' && value[value.length() - 1] == '"')
        {
            value = value.substr(1, value.length() - 2);
        }

        m_executionArgs.AddArg(type, std::string{ value });
    }

    void Command::ParseArguments(Invocation& inv, Execution::Args& execArgs) const
    {
        auto definedArgs = GetArguments();
        Argument::GetCommon(definedArgs);

        ParseArgumentsStateMachine stateMachine{ inv, execArgs, std::move(definedArgs) };

        while (stateMachine.Step())
        {
            stateMachine.ThrowIfError();
        }

        // Special handling for multi-query arguments:
        execArgs.MakeMultiQueryContainUniqueValues();
        execArgs.MoveMultiQueryToSingleQueryIfNeeded();
    }

    void Command::ValidateArguments(Execution::Args& execArgs) const
    {
        // If help is asked for, don't bother validating anything else
        if (execArgs.Contains(Execution::Args::Type::Help))
        {
            return;
        }

        // Common arguments need to be validated with command arguments, as there may be common arguments blocked by Experimental Feature or Group Policy
        auto allArgs = GetArguments();
        Argument::GetCommon(allArgs);

        for (const auto& arg : allArgs)
        {
            if (!Settings::GroupPolicies().IsEnabled(arg.GroupPolicy()) && execArgs.Contains(arg.ExecArgType()))
            {
                auto policy = TogglePolicy::GetPolicy(arg.GroupPolicy());
                AICLI_LOG(CLI, Error, << "Trying to use argument: " << arg.Name() << " disabled by group policy " << policy.RegValueName());
                throw GroupPolicyException(arg.GroupPolicy());
            }

            if (arg.AdminSetting() != BoolAdminSetting::Unknown && !Settings::IsAdminSettingEnabled(arg.AdminSetting()) && execArgs.Contains(arg.ExecArgType()))
            {
                auto setting = Settings::AdminSettingToString(arg.AdminSetting());
                AICLI_LOG(CLI, Error, << "Trying to use argument: " << arg.Name() << " disabled by admin setting " << setting);
                throw CommandException(Resource::String::FeatureDisabledByAdminSettingMessage(setting));
            }

            if (!ExperimentalFeature::IsEnabled(arg.Feature()) && execArgs.Contains(arg.ExecArgType()))
            {
                auto feature = ExperimentalFeature::GetFeature(arg.Feature());
                AICLI_LOG(CLI, Error, << "Trying to use argument: " << arg.Name() << " without enabling feature " << feature.JsonName());
                throw CommandException(Resource::String::FeatureDisabledMessage(feature.JsonName()));
            }

            if (arg.Required() && !execArgs.Contains(arg.ExecArgType()))
            {
                throw CommandException(Resource::String::RequiredArgError(arg.Name()));
            }

            if (arg.Limit() < execArgs.GetCount(arg.ExecArgType()))
            {
                throw CommandException(Resource::String::TooManyArgError(arg.Name()));
            }
        }

        if (execArgs.Contains(Execution::Args::Type::Silent) && execArgs.Contains(Execution::Args::Type::Interactive))
        {
            throw CommandException(Resource::String::TooManyBehaviorsError(s_Command_ArgName_SilentAndInteractive));
        }

        if (execArgs.Contains(Execution::Args::Type::CustomHeader) && !execArgs.Contains(Execution::Args::Type::Source) &&
           !execArgs.Contains(Execution::Args::Type::SourceName))
        {
            throw CommandException(Resource::String::HeaderArgumentNotApplicableWithoutSource(Argument::ForType(Execution::Args::Type::CustomHeader).Name()));
        }

        if (execArgs.Contains(Execution::Args::Type::Count))
        {
            try
            {
                int countRequested = std::stoi(std::string(execArgs.GetArg(Execution::Args::Type::Count)));
                if (countRequested < 1 || countRequested > 1000)
                {
                    throw CommandException(Resource::String::CountOutOfBoundsError);
                }
            }
            catch (...)
            {
                throw CommandException(Resource::String::CountOutOfBoundsError);
            }
        }

        if (execArgs.Contains(Execution::Args::Type::InstallArchitecture))
        {
            Utility::Architecture selectedArch = Utility::ConvertToArchitectureEnum(std::string(execArgs.GetArg(Execution::Args::Type::InstallArchitecture)));
            if ((selectedArch == Utility::Architecture::Unknown) || (Utility::IsApplicableArchitecture(selectedArch) == Utility::InapplicableArchitecture))
            {
                std::vector<Utility::LocIndString> applicableArchitectures;
                for (Utility::Architecture i : Utility::GetApplicableArchitectures())
                {
                    applicableArchitectures.emplace_back(Utility::ToString(i));
                }

                auto validOptions = Utility::Join(", "_liv, applicableArchitectures);
                throw CommandException(Resource::String::InvalidArgumentValueError(Argument::ForType(Execution::Args::Type::InstallArchitecture).Name(), validOptions));
            }
        }

        if (execArgs.Contains(Execution::Args::Type::InstallerArchitecture))
        {
            Utility::Architecture selectedArch = Utility::ConvertToArchitectureEnum(std::string(execArgs.GetArg(Execution::Args::Type::InstallerArchitecture)));
            if (selectedArch == Utility::Architecture::Unknown)
            {
                std::vector<Utility::LocIndString> applicableArchitectures;
                for (Utility::Architecture i : Utility::GetAllArchitectures())
                {
                    applicableArchitectures.emplace_back(Utility::ToString(i));
                }

                auto validOptions = Utility::Join(", "_liv, applicableArchitectures);
                throw CommandException(Resource::String::InvalidArgumentValueError(Argument::ForType(Execution::Args::Type::InstallerArchitecture).Name(), validOptions));
            }
        }

        if (execArgs.Contains(Execution::Args::Type::Locale))
        {
            if (!Locale::IsWellFormedBcp47Tag(execArgs.GetArg(Execution::Args::Type::Locale)))
            {
                throw CommandException(Resource::String::InvalidArgumentValueErrorWithoutValidValues(Argument::ForType(Execution::Args::Type::Locale).Name()));
            }
        }

        if (execArgs.Contains(Execution::Args::Type::InstallScope))
        {
            if (Manifest::ConvertToScopeEnum(execArgs.GetArg(Execution::Args::Type::InstallScope)) == Manifest::ScopeEnum::Unknown)
            {
                auto validOptions = Utility::Join(", "_liv, std::vector<Utility::LocIndString>{ "user"_lis, "machine"_lis });
                throw CommandException(Resource::String::InvalidArgumentValueError(ArgumentCommon::ForType(Execution::Args::Type::InstallScope).Name, validOptions));
            }
        }

        if (execArgs.Contains(Execution::Args::Type::InstallerType))
        {
            Manifest::InstallerTypeEnum selectedInstallerType = Manifest::ConvertToInstallerTypeEnum(std::string(execArgs.GetArg(Execution::Args::Type::InstallerType)));
            if (selectedInstallerType == Manifest::InstallerTypeEnum::Unknown)
            {
                throw CommandException(Resource::String::InvalidArgumentValueErrorWithoutValidValues(Argument::ForType(Execution::Args::Type::InstallerType).Name()));
            }
        }

        if (execArgs.Contains(Execution::Args::Type::AuthenticationMode))
        {
            if (Authentication::ConvertToAuthenticationMode(execArgs.GetArg(Execution::Args::Type::AuthenticationMode)) == Authentication::AuthenticationMode::Unknown)
            {
                auto validOptions = Utility::Join(", "_liv, std::vector<Utility::LocIndString>{ "interactive"_lis, "silentPreferred"_lis, "silent"_lis });
                throw CommandException(Resource::String::InvalidArgumentValueError(ArgumentCommon::ForType(Execution::Args::Type::AuthenticationMode).Name, validOptions));
            }
        }

        Argument::ValidateExclusiveArguments(execArgs);

        ValidateArgumentsInternal(execArgs);
    }

    // Completion can produce one of several things if the completion context is appropriate:
    //  1. Sub commands, if the context is immediately after this command.
    //  2. Argument names, if a value is not expected.
    //  3. Argument values, if one is expected.
    void Command::Complete(Execution::Context& context) const
    {
        CompletionData& data = context.Get<Execution::Data::CompletionData>();
        const std::string& word = data.Word();

        // The word we are to complete is directly after the command, thus it's sub-commands are potentials.
        if (data.BeforeWord().begin() == data.BeforeWord().end())
        {
            for (const auto& command : GetCommands())
            {
                if (word.empty() || Utility::CaseInsensitiveStartsWith(command->Name(), word))
                {
                    context.Reporter.Completion() << command->Name() << std::endl;
                }
                // Allow for command aliases to be auto-completed
                if (!(command->Aliases()).empty() && !word.empty())
                {
                    for (const auto& commandAlias : command->Aliases())
                    {
                        if (Utility::CaseInsensitiveStartsWith(commandAlias, word))
                        {
                            context.Reporter.Completion() << commandAlias << std::endl;
                        }
                    }
                }
            }
        }

        // Consume what remains, if any, of the preceding values to determine what type the word is.
        auto definedArgs = GetArguments();
        Argument::GetCommon(definedArgs);

        ParseArgumentsStateMachine stateMachine{ data.BeforeWord(), context.Args, std::move(definedArgs) };

        // We don't care if there are errors along the way, just do the best that can be done and try to
        // complete whatever would be next if the bad strings were simply ignored. To do that we just spin
        // through the state until we reach our word.
        while (stateMachine.Step());

        const auto& state = stateMachine.GetState();

        // This means that anything is possible, so argument names are on the table.
        if (!state.Type() && !stateMachine.OnlyPositionalRemain())
        {
            // Use argument names if:
            //  1. word is empty
            //  2. word is just "-"
            //  3. word starts with "--"
            if (word.empty() ||
                word == APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_STRING ||
                Utility::CaseInsensitiveStartsWith(word, APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_STRING APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_STRING))
            {
                for (const auto& arg : stateMachine.Arguments())
                {
                    if (word.length() <= 2 || Utility::CaseInsensitiveStartsWith(arg.Name(), word.substr(2)))
                    {
                        context.Reporter.Completion() << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Name() << std::endl;
                    }
                }
            }
            // Use argument aliases if the word is already one; allow cycling through them.
            else if (Utility::CaseInsensitiveStartsWith(word, APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_STRING) && word.length() == 2)
            {
                for (const auto& arg : stateMachine.Arguments())
                {
                    if (arg.Alias() != ArgumentCommon::NoAlias)
                    {
                        context.Reporter.Completion() << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << arg.Alias() << std::endl;
                    }
                }
            }
        }

        std::optional<Execution::Args::Type> typeToComplete = state.Type();

        // We are not waiting on an argument value, so the next could be a positional if the incoming word is not an argument name.
        // If there is one, offer to complete it.
        if (!typeToComplete && (word.empty() || word[0] != APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR))
        {
            const auto* nextPositional = stateMachine.NextPositional();
            if (nextPositional)
            {
                typeToComplete = nextPositional->ExecArgType();
            }
        }

        // To enable more complete scenarios, also attempt to parse any arguments after the word to complete.
        // This will allow these later values to affect the result of the completion (for instance, if a specific source is listed).
        {
            ParseArgumentsStateMachine afterWordStateMachine{ data.AfterWord(), context.Args, stateMachine.Arguments() };
            while (afterWordStateMachine.Step());
        }

        // Let the derived command take over supplying context sensitive argument value.
        if (typeToComplete)
        {
            Complete(context, typeToComplete.value());
        }
    }

    void Command::Complete(Execution::Context&, Execution::Args::Type) const
    {
        // Derived commands must supply context sensitive argument values.
    }

    void Command::Execute(Execution::Context& context) const
    {
        // Block any execution if winget is disabled by policy.
        // Override the function to bypass this.
        if (!Settings::GroupPolicies().IsEnabled(Settings::TogglePolicy::Policy::WinGet))
        {
            AICLI_LOG(CLI, Error, << "WinGet is disabled by group policy");
            throw Settings::GroupPolicyException::GroupPolicyException(Settings::TogglePolicy::Policy::WinGet);
        }

        AICLI_LOG(CLI, Info, << "Executing command: " << Name());
        if (context.Args.Contains(Execution::Args::Type::Help))
        {
            OutputHelp(context.Reporter);
        }
        else
        {
            ExecuteInternal(context);
        }

        // NOTE: Reboot logic will still run even if the context is terminated (not including unhandled exceptions).
        if (context.Args.Contains(Execution::Args::Type::AllowReboot) &&
            WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::RebootRequired))
        {
            context.Reporter.Warn() << Resource::String::InitiatingReboot << std::endl;

            if (context.Args.Contains(Execution::Args::Type::Wait))
            {
                context.Reporter.PromptForEnter();
            }

            if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::RegisterResume))
            {
                // RegisterResume context flag assumes we already wrote to the RunOnce registry.
                // Since we are about to initiate a restart, this is no longer needed as a safety net.
                Reboot::UnregisterRestartForWER();

                context.ClearFlags(Execution::ContextFlag::RegisterResume);
            }

            context.ClearFlags(Execution::ContextFlag::RebootRequired);

            if (!Reboot::InitiateReboot())
            {
                context.Reporter.Error() << Resource::String::FailedToInitiateReboot << std::endl;
            }
        }
        else
        {
            LaunchLogsIfRequested(context);

            if (context.Args.Contains(Execution::Args::Type::Wait))
            {
                context.Reporter.PromptForEnter();
            }
        }
    }

    void Command::Resume(Execution::Context& context) const
    {
        context.Reporter.Error() << Resource::String::CommandDoesNotSupportResumeMessage << std::endl;
        AICLI_TERMINATE_CONTEXT(E_NOTIMPL);
    }
    
    void Command::SelectCurrentCommandIfUnrecognizedSubcommandFound(bool value)
    {
        m_selectCurrentCommandIfUnrecognizedSubcommandFound = value;
    }

    void Command::ValidateArgumentsInternal(Execution::Args&) const
    {
        // Do nothing by default.
        // Commands may not need any extra validation.
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

        if (!Settings::GroupPolicies().IsEnabled(m_groupPolicy))
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
        Argument::GetCommon(arguments);

        arguments.erase(
            std::remove_if(
                arguments.begin(), arguments.end(),
                [](const Argument& arg) { return arg.GetVisibility() == Argument::Visibility::Hidden; }),
            arguments.end());

        return arguments;
    }

    void ExecuteWithoutLoggingSuccess(Execution::Context& context, Command* command)
    {
        try
        {
            if (!Settings::User().GetWarnings().empty() &&
                !WI_IsFlagSet(command->GetOutputFlags(), CommandOutputFlags::IgnoreSettingsWarnings))
            {
                context.Reporter.Warn() << Resource::String::SettingsWarnings << std::endl;
            }

            command->Execute(context);
        }
        catch (...)
        {
            context.SetTerminationHR(Workflow::HandleException(context, std::current_exception()));

            LaunchLogsIfRequested(context);
        }
    }

    int Execute(Execution::Context& context, std::unique_ptr<Command>& command)
    {
        ExecuteWithoutLoggingSuccess(context, command.get());

        if (SUCCEEDED(context.GetTerminationHR()))
        {
            Logging::Telemetry().LogCommandSuccess(command->FullName());
        }

        return context.GetTerminationHR();
    }
}
