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
    return std::string{ command->FullName() };
}

std::vector<std::string_view> GetCommandAliases(const std::unique_ptr<Command>& command)
{
    return std::vector<std::string_view> { command->Aliases() };
}

std::string GetArgumentName(const Argument& arg)
{
    return std::string{ arg.Name() };
}

std::string GetArgumentAlternateName(const Argument& arg)
{
    if (arg.AlternateName() == Argument::NoAlternateName)
    {
        return {};
    }
    else
    {
        return std::string{ arg.AlternateName() };
    }
}

std::string GetArgumentAlias(const Argument& arg)
{
    if (arg.Alias() == ArgumentCommon::NoAlias)
    {
        return {};
    }
    else
    {
        return std::string(1, arg.Alias());
    }
}

bool StringIsLowercase(const std::string& s)
{
    return Utility::ToLower(s) == s;
}

template <typename Enumerable, typename Op>
void EnsureStringsAreLowercaseAndNoCollisions(const std::string& info, const Enumerable& e, Op& op, std::unordered_set<std::string>& values, bool requireLower = true)
{
    INFO(info);

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
            REQUIRE(StringIsLowercase(valString));
        }

        REQUIRE(values.find(valString) == values.end());

        values.emplace(std::move(valString));
    }
}

template <typename Enumerable, typename Op>
void EnsureStringsAreLowercaseAndNoCollisions(const std::string& info, const Enumerable& e, Op& op, bool requireLower = true)
{
    std::unordered_set<std::string> values;
    EnsureStringsAreLowercaseAndNoCollisions(info, e, op, values, requireLower);
}

template <typename Enumerable, typename Op>
void EnsureVectorStringViewsAreLowercaseAndNoCollisions(const std::string& info, const Enumerable& e, Op& op, std::unordered_set<std::string>& values, bool requireLower = true)
{
    INFO(info);

    for (const auto& val : e)
    { 
        std::vector<std::string_view> aliasVector = op(val);
        std::vector<std::string> valVector(aliasVector.begin(), aliasVector.end());
        if (valVector.empty())
        {
            continue;
        }
        // When op returns a vector, we need to ensure every value in the vector does not cause a collision
        for (auto& valString : valVector)
        {
            INFO(valString);

            if (requireLower)
            {
                REQUIRE(StringIsLowercase(valString));
            }

            REQUIRE(values.find(valString) == values.end());
            values.emplace(std::move(valString));
        }
    }
}

void EnsureCommandConsistency(const Command& command)
{
    // Command names and aliases exist in the same space, so both need to be checked as a set
    // However, collisions do not occur between levels, so the full name must be used to check for collision
    std::unordered_set<std::string> allCommandAliasNames; 
    EnsureStringsAreLowercaseAndNoCollisions(command.FullName() + " commands", command.GetCommands(), GetCommandName, allCommandAliasNames);
    EnsureVectorStringViewsAreLowercaseAndNoCollisions(command.FullName() + " aliases", command.GetCommands(), GetCommandAliases, allCommandAliasNames);

    auto args = command.GetArguments();
    Argument::GetCommon(args);

    // Argument names and alternate names exist in the same space, so both need to be checked as a set
    std::unordered_set<std::string> allArgumentNames;
    EnsureStringsAreLowercaseAndNoCollisions(command.FullName() + " argument names", args, GetArgumentName, allArgumentNames);
    EnsureStringsAreLowercaseAndNoCollisions(command.FullName() + " argument alternate name", args, GetArgumentAlternateName, allArgumentNames);
    // Argument aliases use a different space than the names and can be checked separately
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
    bool queryArgPresent = false;
    bool multiQueryArgPresent = false;
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

        if (arg.ExecArgType() == Execution::Args::Type::Query)
        {
            queryArgPresent = true;
        }

        if (arg.ExecArgType() == Execution::Args::Type::MultiQuery)
        {
            multiQueryArgPresent = true;
        }
    }

    REQUIRE((!queryArgPresent || !multiQueryArgPresent));

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
//  9. No command includes both Query and MultiQuery arguments
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
    CommandExceptionMatcher(CLI::Resource::LocString message) : m_expectedMessage(std::move(message)) {}

    bool match(const CommandException& ce) const override
    {
        return ce.Message() == m_expectedMessage;
    }

    std::string describe() const override
    {
        std::ostringstream result;
        result << "has message == " << m_expectedMessage;
        return result.str();
    }

private:
    CLI::Resource::LocString m_expectedMessage;
};

namespace Catch {
    template<>
    struct StringMaker<CommandException> {
        static std::string convert(CommandException const& ce) {
            return Utility::Format("CommandException{ '{0}' }", ce.Message().get());
        }
    };
}

#define REQUIRE_COMMAND_EXCEPTION(_expr_, _arg_)     REQUIRE_THROWS_MATCHES(_expr_, CommandException, CommandExceptionMatcher(_arg_))

void RequireValueParsedToArg(const std::string& value, const Argument& arg, const Args& args)
{
    REQUIRE(args.Contains(arg.ExecArgType()));
    REQUIRE(value == args.GetArg(arg.ExecArgType()));
}

void RequireValuesParsedToArg(const std::vector<std::string>& values, Args::Type execArgType, const Args& args)
{
    REQUIRE(args.Contains(execArgType));
    REQUIRE(args.GetCount(execArgType) == values.size());

    auto argValues = args.GetArgs(execArgType);
    for (size_t i = 0; i < values.size(); ++i)
    {
        REQUIRE(argValues->at(i) == values[i]);
    }
}

void RequireValuesParsedToArg(const std::vector<std::string>& values, const Argument& arg, const Args& args)
{
    RequireValuesParsedToArg(values, arg.ExecArgType(), args);
}

// Description used for tests; doesn't need to be anything in particular.
static constexpr CLI::Resource::StringId DefaultDesc{ L""sv };

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

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), CLI::Resource::String::ExtraPositionalError(Utility::LocIndView{ values[2] }));
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

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), CLI::Resource::String::InvalidArgumentSpecifierError(Utility::LocIndView{ values[1] }));
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

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), CLI::Resource::String::InvalidAliasError(Utility::LocIndView{ values[1] }));
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

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), CLI::Resource::String::AdjoinedNotFoundError(Utility::LocIndView{ values[1] }));
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

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), CLI::Resource::String::AdjoinedNotFlagError(Utility::LocIndView{ values[1] }));
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

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), CLI::Resource::String::AdjoinedNotFoundError(Utility::LocIndView{ values[1] }));
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

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), CLI::Resource::String::MissingArgumentError(Utility::LocIndView{ values[0] }));
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

TEST_CASE("ParseArguments_AlternateNameWithAdjoinedValue", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', "p1", Args::Type::Channel, DefaultDesc, ArgumentType::Positional},
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
        });

    std::vector<std::string> values{ "--p1=Val1" };
    Invocation inv{ std::vector<std::string>(values) };

    command.ParseArguments(inv, args);

    RequireValueParsedToArg(values[0].substr(5), command.m_args[0], args);
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

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), CLI::Resource::String::FlagContainAdjoinedError(Utility::LocIndView{ values[1] }));
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

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), CLI::Resource::String::MissingArgumentError(Utility::LocIndView{ values[0] }));
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

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), CLI::Resource::String::InvalidNameError(Utility::LocIndView{ values[1] }));
}

TEST_CASE("ParseArguments_PositionalWithMultipleValues", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "multi", 'm', Args::Type::MultiQuery, DefaultDesc, ArgumentType::Positional }.SetCountLimit(5),
        });

    std::vector<std::string> values{ "value1" "value2", "value3" };
    Invocation inv{ std::vector<std::string>(values) };

    command.ParseArguments(inv, args);
    RequireValuesParsedToArg(values, command.m_args[0], args);
}

TEST_CASE("ParseArguments_PositionalWithMultipleValuesAndOtherArgs", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Source, DefaultDesc, ArgumentType::Positional },
            Argument{ "pos2", 'q', Args::Type::All, DefaultDesc, ArgumentType::Positional },
            Argument{ "multi", 'm', Args::Type::MultiQuery, DefaultDesc, ArgumentType::Positional }.SetCountLimit(5),
            Argument{ "flag", 'f', Args::Type::BlockingPin, DefaultDesc, ArgumentType::Flag },
        });

    std::vector<std::string> values{ "positional", "-q", "anotherPos", "multiValue1", "multiValue2", "-f" };
    Invocation inv{ std::vector<std::string>(values) };

    command.ParseArguments(inv, args);
    RequireValueParsedToArg(values[0], command.m_args[0], args);
    RequireValueParsedToArg(values[2], command.m_args[1], args);
    RequireValuesParsedToArg({ values[3], values[4] }, command.m_args[2], args);
    REQUIRE(args.Contains(command.m_args[3].ExecArgType()));
}

TEST_CASE("ParseArguments_PositionalWithMultipleValuesAndName", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "multi", 'm', Args::Type::MultiQuery, DefaultDesc, ArgumentType::Positional }.SetCountLimit(5),
        });

    std::vector<std::string> values{ "--multi", "one", "two", "three" };
    Invocation inv{ std::vector<std::string>(values) };

    command.ParseArguments(inv, args);
    RequireValuesParsedToArg({ values[1], values[2], values[3] }, command.m_args[0], args);
}

TEST_CASE("ParseArguments_MultiQueryConvertedToSingleQuery", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "multi", 'm', Args::Type::MultiQuery, DefaultDesc, ArgumentType::Positional }.SetCountLimit(5),
        });

    std::vector<std::string> values{ "singleValue" };
    Invocation inv{ std::vector<std::string>(values) };

    // ParseArguments converts MultiQuery args with a single value into Query args
    command.ParseArguments(inv, args);
    RequireValuesParsedToArg({ values[0] }, Args::Type::Query, args);
}

TEST_CASE("ParseArguments_PositionalWithTooManyValues", "[command]")
{
    Args args;
    TestCommand command({
            Argument{ "multi", 'm', Args::Type::MultiQuery, DefaultDesc, ArgumentType::Positional }.SetCountLimit(5),
        });

    std::vector<std::string> values{ "1", "2", "3", "4", "5", "tooMany" };
    Invocation inv{ std::vector<std::string>(values) };

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), CLI::Resource::String::ExtraPositionalError(Utility::LocIndView{ values.back() }));
}