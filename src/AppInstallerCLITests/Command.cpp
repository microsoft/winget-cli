// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Command.h>
#include <AppInstallerStrings.h>
#include <Commands/RootCommand.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;

std::string GetCommandName(const std::unique_ptr<Command>& command)
{
    return std::string{ command->Name() };
}

std::string GetArgumentName(const Argument& arg)
{
    return std::string{ arg.Name() };
}

std::string GetArgumentAlias(const Argument& arg)
{
    if (arg.Alias() == Argument::NoAlias)
    {
        return {};
    }
    else
    {
        return std::string(1, arg.Alias());
    }
}

template <typename Enumerable, typename Op>
void EnsureStringsAreLowercaseAndNoCollisions(const std::string& info, const Enumerable& e, Op& op, bool requireLower = true)
{
    INFO(info);
    std::unordered_set<std::string> values;

    for (const auto& val : e)
    {
        std::string valString = op(val);
        if (valString.empty())
        {
            continue;
        }
        INFO(valString);

        if (requireLower)
        {
            std::string lowerVal = Utility::ToLower(valString);
            REQUIRE(valString == lowerVal);
        }

        REQUIRE(values.find(valString) == values.end());

        values.emplace(std::move(valString));
    }
}

void EnsureCommandConsistency(const Command& command)
{
    EnsureStringsAreLowercaseAndNoCollisions(command.FullName() + " commands", command.GetCommands(), GetCommandName);

    auto args = command.GetArguments();
    Argument::GetCommon(args);
    EnsureStringsAreLowercaseAndNoCollisions(command.FullName() + " argument names", args, GetArgumentName);
    EnsureStringsAreLowercaseAndNoCollisions(command.FullName() + " argument alias", args, GetArgumentAlias, false);

    // No : allowed in commands
    for (const auto& comm : command.GetCommands())
    {
        INFO(command.FullName());
        INFO(comm->Name());

        REQUIRE(comm->Name().find_first_of(Command::ParentSplitChar) == std::string_view::npos);
    }

    // No = allowed in arguments
    // All positional args should be listed first
    bool foundNonPositional = false;
    for (const auto& arg : command.GetArguments())
    {
        INFO(command.FullName());
        INFO(arg.Name());

        REQUIRE(arg.Name().find_first_of(APPINSTALLER_CLI_ARGUMENT_SPLIT_CHAR) == std::string_view::npos);

        if (arg.Type() == ArgumentType::Positional)
        {
            REQUIRE(!foundNonPositional);
        }
        else
        {
            foundNonPositional = true;
        }
    }

    // Recurse for all subcommands
    for (const auto& sub : command.GetCommands())
    {
        EnsureCommandConsistency(*sub.get());
    }
}

// This test ensure that the command tree we expose does not have any inconsistencies.
//  1. No command name collisions
//  2. All command names are lower cased
//  3. No argument name collisions
//  4. All arguments are lower cased
//  5. No argument alias collisions
//  6. All argument alias are lower cased
//  7. No argument names contain '='
//  8. All positional arguments are first in the list
TEST_CASE("EnsureCommandTreeConsistency", "[command]")
{
    RootCommand root;
    EnsureCommandConsistency(root);
}

struct TestCommand : public Command
{
    TestCommand(std::vector<Argument> args) : Command("test", ""), m_args(std::move(args)) {}

    std::vector<Argument> GetArguments() const override
    {
        return m_args;
    }

    std::vector<Argument> m_args;
};

// Matcher that lets us verify CommandExceptions.
struct CommandExceptionMatcher : public Catch::MatcherBase<CommandException>
{
    CommandExceptionMatcher(const std::string &arg) : m_expectedArg(arg) {}

    bool match(const CommandException& ce) const override
    {
        const auto& params = ce.Params();
        return params.size() == 1 && params[0].get() == m_expectedArg;
    }

    std::string describe() const override
    {
        std::ostringstream result;
        result << "has param == " << m_expectedArg;
        return result.str();
    }

private:
    std::string m_expectedArg;
};

namespace Catch {
    template<>
    struct StringMaker<CommandException> {
        static std::string convert(CommandException const& ce) {
            std::string result{ "CommandException{ '" };
            result += ce.Message().get();
            result += '\'';

            bool first = true;
            for (const auto& param : ce.Params())
            {
                if (first)
                {
                    first = false;
                    result += ", ['";
                }
                else
                {
                    result += "', '";
                }
                result += param.get();
            }

            if (!first)
            {
                result += "']";
            }

            result += " }";
            return result;
        }
    };
}

#define REQUIRE_COMMAND_EXCEPTION(_expr_, _arg_)     REQUIRE_THROWS_MATCHES(_expr_, CommandException, CommandExceptionMatcher(_arg_))

void RequireValueParsedToArg(const std::string& value, const Argument& arg, const Args& args)
{
    REQUIRE(args.Contains(arg.ExecArgType()));
    REQUIRE(value == args.GetArg(arg.ExecArgType()));
}

// Description used for tests; doesn't need to be anything in particular.
static constexpr Resource::StringId DefaultDesc{ L""sv };

TEST_CASE("ParseArguments_MultiplePositional", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
        });

    std::vector<std::string> values{ "val1", "val2" };
    Invocation inv{ std::vector<std::string>(values) };

    command.ParseArguments(inv, args);

    RequireValueParsedToArg(values[0], command.m_args[0], args);
    RequireValueParsedToArg(values[1], command.m_args[2], args);
}

TEST_CASE("ParseArguments_ForcePositional", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
        });

    std::vector<std::string> values{ "val1", "--", "-std1" };
    Invocation inv{ std::vector<std::string>(values) };

    command.ParseArguments(inv, args);

    RequireValueParsedToArg(values[0], command.m_args[0], args);
    RequireValueParsedToArg(values[2], command.m_args[2], args);
}

TEST_CASE("ParseArguments_TooManyPositional", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
        });

    std::vector<std::string> values{ "val1", "--", "-std1" };
    Invocation inv{ std::vector<std::string>(values) };

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), values[2]);
}

TEST_CASE("ParseArguments_InvalidChar", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
        });

    std::vector<std::string> values{ "val1", "-", "-std1" };
    Invocation inv{ std::vector<std::string>(values) };

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), values[1]);
}

TEST_CASE("ParseArguments_InvalidAlias", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
        });

    std::vector<std::string> values{ "val1", "-b", "-std1" };
    Invocation inv{ std::vector<std::string>(values) };

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), values[1]);
}

TEST_CASE("ParseArguments_MultiFlag", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Flag },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag2", 't', Args::Type::Exact, DefaultDesc, ArgumentType::Flag },
        });

    std::vector<std::string> values{ "val1", "-st", "val2" };
    Invocation inv{ std::vector<std::string>(values) };

    command.ParseArguments(inv, args);

    REQUIRE(args.Contains(command.m_args[1].ExecArgType()));
    REQUIRE(args.Contains(command.m_args[3].ExecArgType()));
}

TEST_CASE("ParseArguments_FlagThenUnknown", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Flag },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag2", 't', Args::Type::Exact, DefaultDesc, ArgumentType::Flag },
        });

    std::vector<std::string> values{ "val1", "-sr", "val2" };
    Invocation inv{ std::vector<std::string>(values) };

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), values[1]);
}

TEST_CASE("ParseArguments_FlagThenNonFlag", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Flag },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag2", 't', Args::Type::Exact, DefaultDesc, ArgumentType::Flag },
        });

    std::vector<std::string> values{ "val1", "-sp", "val2" };
    Invocation inv{ std::vector<std::string>(values) };

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), values[1]);
}

TEST_CASE("ParseArguments_NameUsingAliasSpecifier", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag1", 'f', Args::Type::Exact, DefaultDesc, ArgumentType::Flag },
        });

    std::vector<std::string> values{ "another", "-flag1" };
    Invocation inv{ std::vector<std::string>(values) };

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), values[1]);
}

TEST_CASE("ParseArguments_AliasWithAdjoinedValue", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
        });

    std::vector<std::string> values{ "-s=Val1" };
    Invocation inv{ std::vector<std::string>(values) };

    command.ParseArguments(inv, args);

    RequireValueParsedToArg(values[0].substr(3), command.m_args[1], args);
}

TEST_CASE("ParseArguments_AliasWithSeparatedValue", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
        });

    std::vector<std::string> values{ "-s", "Val1" };
    Invocation inv{ std::vector<std::string>(values) };

    command.ParseArguments(inv, args);

    RequireValueParsedToArg(values[1], command.m_args[1], args);
}

TEST_CASE("ParseArguments_AliasWithSeparatedValueMissing", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
        });

    std::vector<std::string> values{ "-s" };
    Invocation inv{ std::vector<std::string>(values) };

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), values[0]);
}

TEST_CASE("ParseArguments_NameWithAdjoinedValue", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
        });

    std::vector<std::string> values{ "--pos1=Val1" };
    Invocation inv{ std::vector<std::string>(values) };

    command.ParseArguments(inv, args);

    RequireValueParsedToArg(values[0].substr(7), command.m_args[0], args);
}

TEST_CASE("ParseArguments_NameFlag", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag1", 'f', Args::Type::Exact, DefaultDesc, ArgumentType::Flag },
        });

    std::vector<std::string> values{ "--flag1", "arbitrary" };
    Invocation inv{ std::vector<std::string>(values) };

    command.ParseArguments(inv, args);

    RequireValueParsedToArg(values[1], command.m_args[0], args);
    REQUIRE(args.Contains(command.m_args[3].ExecArgType()));
}

TEST_CASE("ParseArguments_NameFlagWithAdjoinedValue", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag1", 'f', Args::Type::Exact, DefaultDesc, ArgumentType::Flag },
        });

    std::vector<std::string> values{ "another", "--flag1=arbitrary" };
    Invocation inv{ std::vector<std::string>(values) };

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), values[1]);
}

TEST_CASE("ParseArguments_NameWithSeparatedValue", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag1", 'f', Args::Type::Exact, DefaultDesc, ArgumentType::Flag },
        });

    std::vector<std::string> values{ "--pos2", "arbitrary" };
    Invocation inv{ std::vector<std::string>(values) };

    command.ParseArguments(inv, args);

    RequireValueParsedToArg(values[1], command.m_args[2], args);
}

TEST_CASE("ParseArguments_NameWithSeparatedValueMissing", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag1", 'f', Args::Type::Exact, DefaultDesc, ArgumentType::Flag },
        });

    std::vector<std::string> values{ "--pos2" };
    Invocation inv{ std::vector<std::string>(values) };

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), values[0]);
}

TEST_CASE("ParseArguments_UnknownName", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag1", 'f', Args::Type::Exact, DefaultDesc, ArgumentType::Flag },
        });

    std::vector<std::string> values{ "another", "--nope" };
    Invocation inv{ std::vector<std::string>(values) };

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), values[1]);
}
