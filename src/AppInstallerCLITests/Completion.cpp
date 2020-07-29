// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerErrors.h>
#include <Command.h>
#include <Commands/CompleteCommand.h>
#include <Commands/RootCommand.h>
#include <Commands/SourceCommand.h>
#include <CompletionData.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;


TEST_CASE("CompletionData_EmptyWord_PositionAtEnd", "[complete]")
{
    CompletionData cd{ "", "winget ", "7" };
    REQUIRE(cd.Word() == "");
    REQUIRE(cd.Position() == 7);
    REQUIRE(cd.BeforeWord().size() == 0);
    REQUIRE(cd.AfterWord().size() == 0);
}

TEST_CASE("CompletionData_EmptyWord_PositionPastEnd", "[complete]")
{
    CompletionData cd{ "", "winget ", "8" };
    REQUIRE(cd.Word() == "");
    REQUIRE(cd.Position() == 7);
    REQUIRE(cd.BeforeWord().size() == 0);
    REQUIRE(cd.AfterWord().size() == 0);
}

TEST_CASE("CompletionData_EmptyWord_PositionCorrect", "[complete]")
{
    CompletionData cd{ "", "winget install", "7" };
    REQUIRE(cd.Word() == "");
    REQUIRE(cd.Position() == 7);
    REQUIRE(cd.BeforeWord().size() == 0);
    REQUIRE(cd.AfterWord().size() == 1);
}


TEST_CASE("CompletionData_EmptyWord_PositionOffset", "[complete]")
{
    CompletionData cd{ "", "winget install PowerToys --version", "17" };
    REQUIRE(cd.Word() == "");
    REQUIRE(cd.Position() == 15);
    REQUIRE(cd.BeforeWord().size() == 1);
    REQUIRE(cd.AfterWord().size() == 2);
}

TEST_CASE("CompletionData_Word_NoMatch", "[complete]")
{
    auto lambda = []() { CompletionData cd{ "foo", "winget install PowerToys --version", "17" }; };
    REQUIRE_THROWS_HR(lambda(), APPINSTALLER_CLI_ERROR_COMPLETE_INPUT_BAD);
}

TEST_CASE("CompletionData_Word_SingleMatch", "[complete]")
{
    CompletionData cd{ "power", "winget install power --version", "17" };
    REQUIRE(cd.Word() == "power");
    REQUIRE(cd.Position() == 17);
    REQUIRE(cd.BeforeWord().size() == 1);
    REQUIRE(cd.AfterWord().size() == 1);
}

TEST_CASE("CompletionData_Word_MultiMatch_PositionCorrect", "[complete]")
{
    CompletionData cd{ "power", "winget install power --id power", "27" };
    REQUIRE(cd.Word() == "power");
    REQUIRE(cd.Position() == 27);
    REQUIRE(cd.BeforeWord().size() == 3);
    REQUIRE(cd.AfterWord().size() == 0);
}

TEST_CASE("CompletionData_Word_MultiMatch_PositionOffset", "[complete]")
{
    CompletionData cd{ "power", "winget install power --id power", "21" };
    REQUIRE(cd.Word() == "power");
    REQUIRE(cd.Position() == 21);
    REQUIRE(cd.BeforeWord().size() == 1);
    REQUIRE(cd.AfterWord().size() == 2);
}

void OutputAllSubCommands(Command& command, std::ostream& out, std::string_view filter = {})
{
    for (const auto& c : command.GetCommands())
    {
        if (Utility::CaseInsensitiveStartsWith(c->Name(), filter))
        {
            out << c->Name() << std::endl;
        }
    }
}

void OutputAllArgumentNames(Command& command, std::ostream& out, std::string_view filter = {}, bool includeCommon = true)
{
    auto args = command.GetArguments();
    if (includeCommon)
    {
        Argument::GetCommon(args);
    }

    for (const auto& a : args)
    {
        if (Utility::CaseInsensitiveStartsWith(a.Name(), filter))
        {
            out << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << a.Name() << std::endl;
        }
    }
}

TEST_CASE("CompleteCommand_FindRoot", "[complete]")
{
    std::stringstream out, in;
    Context context{ out, in };

    CompleteCommand command{ "test" };
    context.Args.AddArg(Args::Type::Word, ""sv);
    context.Args.AddArg(Args::Type::CommandLine, "winget "sv);
    context.Args.AddArg(Args::Type::Position, "7"sv);
    command.Execute(context);

    // Create expected values
    RootCommand expectedCommand;
    std::stringstream expected;
    OutputAllSubCommands(expectedCommand, expected);
    OutputAllArgumentNames(expectedCommand, expected);

    REQUIRE(out.str() == expected.str());
}

TEST_CASE("CompleteCommand_FindSource", "[complete]")
{
    std::stringstream out, in;
    Context context{ out, in };

    CompleteCommand command{ "test" };
    context.Args.AddArg(Args::Type::Word, ""sv);
    context.Args.AddArg(Args::Type::CommandLine, "winget source "sv);
    context.Args.AddArg(Args::Type::Position, "14"sv);
    command.Execute(context);

    // Create expected values
    SourceCommand expectedCommand{ "test" };
    std::stringstream expected;
    OutputAllSubCommands(expectedCommand, expected);
    OutputAllArgumentNames(expectedCommand, expected);

    REQUIRE(out.str() == expected.str());
}

TEST_CASE("CompleteCommand_FindSourceAdd", "[complete]")
{
    std::stringstream out, in;
    Context context{ out, in };

    CompleteCommand command{ "test" };
    context.Args.AddArg(Args::Type::Word, ""sv);
    context.Args.AddArg(Args::Type::CommandLine, "winget source add "sv);
    context.Args.AddArg(Args::Type::Position, "18"sv);
    command.Execute(context);

    // Create expected values
    SourceAddCommand expectedCommand{ "test" };
    std::stringstream expected;
    OutputAllSubCommands(expectedCommand, expected);
    OutputAllArgumentNames(expectedCommand, expected);

    REQUIRE(out.str() == expected.str());
}

struct CompletionTestCommand : public Command
{
    CompletionTestCommand() : Command("test", "") {}
    CompletionTestCommand(std::string_view name) : Command(name, "") {}

    std::vector<std::unique_ptr<Command>> GetCommands() const override
    {
        std::vector<std::unique_ptr<Command>> result;

        for (const auto& sc : SubCommandNames)
        {
            result.emplace_back(std::make_unique<CompletionTestCommand>(sc));
        }

        return result;
    }

    std::vector<Argument> GetArguments() const override
    {
        return Arguments;
    }

    using Command::Complete;

    void Complete(Execution::Context& context, Execution::Args::Type valueType) const override
    {
        if (ArgumentValueCallback)
        {
            ArgumentValueCallback(context, valueType);
        }
    }

    std::vector<std::string> SubCommandNames;
    std::vector<Argument> Arguments;
    std::function<void(Context&, Execution::Args::Type)> ArgumentValueCallback;
};

TEST_CASE("CommandComplete_Simple", "[complete]")
{
    std::stringstream out, in;
    Context context{ out, in };
    context.Reporter.SetChannel(Execution::Reporter::Channel::Completion);

    // This will treat our test command like the root
    CompletionData cd{ "", "winget ", "7" };
    context.Add<Data::CompletionData>(std::move(cd));

    CompletionTestCommand command;
    command.SubCommandNames = { "test1", "test2" };
    command.Arguments = { Argument{ "arg1", 'a', Args::Type::Query, Resource::String::Done, ArgumentType::Standard } };
    command.Complete(context);

    // Create expected values
    std::stringstream expected;
    OutputAllSubCommands(command, expected);
    OutputAllArgumentNames(command, expected);

    REQUIRE(out.str() == expected.str());
}

TEST_CASE("CommandComplete_PartialCommandMatch", "[complete]")
{
    std::stringstream out, in;
    Context context{ out, in };
    context.Reporter.SetChannel(Execution::Reporter::Channel::Completion);

    // This will treat our test command like the root
    CompletionData cd{ "cart", "winget cart", "11" };
    context.Add<Data::CompletionData>(std::move(cd));

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesion", "carpet" };
    command.Complete(context);

    // Create expected values
    std::stringstream expected;
    OutputAllSubCommands(command, expected, "cart");
    OutputAllArgumentNames(command, expected, "cart");

    REQUIRE(out.str() == expected.str());
}

TEST_CASE("CommandComplete_CommandsNotAllowed", "[complete]")
{
    std::stringstream out, in;
    Context context{ out, in };
    context.Reporter.SetChannel(Execution::Reporter::Channel::Completion);

    // This will treat our test command like the root
    CompletionData cd{ "", "winget foobar ", "14" };
    context.Add<Data::CompletionData>(std::move(cd));

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesion", "carpet" };
    command.Complete(context);

    // Create expected values
    std::stringstream expected;
    OutputAllArgumentNames(command, expected);

    REQUIRE(out.str() == expected.str());
}
