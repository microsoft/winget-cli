// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include <AppInstallerErrors.h>
#include <AppInstallerFileLogger.h>
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
    REQUIRE(cd.BeforeWord().size() == 0);
    REQUIRE(cd.AfterWord().size() == 0);
}

TEST_CASE("CompletionData_EmptyWord_PositionPastEnd", "[complete]")
{
    CompletionData cd{ "", "winget ", "8" };
    REQUIRE(cd.Word() == "");
    REQUIRE(cd.BeforeWord().size() == 0);
    REQUIRE(cd.AfterWord().size() == 0);
}

TEST_CASE("CompletionData_EmptyWord_PositionCorrect", "[complete]")
{
    CompletionData cd{ "", "winget install", "7" };
    REQUIRE(cd.Word() == "");
    REQUIRE(cd.BeforeWord().size() == 0);
    REQUIRE(cd.AfterWord().size() == 1);
}


TEST_CASE("CompletionData_EmptyWord_PositionOffset", "[complete]")
{
    CompletionData cd{ "", "winget install PowerToys --version", "17" };
    REQUIRE(cd.Word() == "");
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
    REQUIRE(cd.BeforeWord().size() == 1);
    REQUIRE(cd.AfterWord().size() == 1);
}

TEST_CASE("CompletionData_Word_MultiMatch_PositionCorrect", "[complete]")
{
    CompletionData cd{ "power", "winget install power --id power", "27" };
    REQUIRE(cd.Word() == "power");
    REQUIRE(cd.BeforeWord().size() == 3);
    REQUIRE(cd.AfterWord().size() == 0);
}

TEST_CASE("CompletionData_Word_MultiMatch_PositionOffset", "[complete]")
{
    CompletionData cd{ "power", "winget install power --id power", "21" };
    REQUIRE(cd.Word() == "power");
    REQUIRE(cd.BeforeWord().size() == 1);
    REQUIRE(cd.AfterWord().size() == 2);
}

TEST_CASE("CompletionData_UTF8_EmptyWord_End", "[complete]")
{
    CompletionData cd{ "", u8"winget install \x175\x12b\x14b\x1e5\x229\x288 --version ", "32" };
    REQUIRE(cd.Word() == "");
    REQUIRE(cd.BeforeWord().size() == 3);
    REQUIRE(cd.AfterWord().size() == 0);
}

TEST_CASE("CompletionData_UTF8_EmptyWord_Middle", "[complete]")
{
    CompletionData cd{ "", u8"winget install \x175\x12b\x14b\x1e5\x229\x288 --version ", "22" };
    REQUIRE(cd.Word() == "");
    REQUIRE(cd.BeforeWord().size() == 2);
    REQUIRE(cd.AfterWord().size() == 1);
}

TEST_CASE("CompletionData_UTF8_UTF8Word", "[complete]")
{
    CompletionData cd{ u8"\x175\x12b\x14b\x1e5\x229\x288", u8"winget install \x175\x12b\x14b\x1e5\x229\x288 --version ", "18" };
    REQUIRE(cd.Word() == u8"\x175\x12b\x14b\x1e5\x229\x288");
    REQUIRE(cd.BeforeWord().size() == 1);
    REQUIRE(cd.AfterWord().size() == 1);
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

void OutputAllArgumentAliases(Command& command, std::ostream& out, bool includeCommon = true)
{
    auto args = command.GetArguments();
    if (includeCommon)
    {
        Argument::GetCommon(args);
    }

    for (const auto& a : args)
    {
        if (a.Alias() != ArgumentCommon::NoAlias)
        {
            out << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << a.Alias() << std::endl;
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

    CLI::Resource::LocString ShortDescription() const override { return {}; }
    CLI::Resource::LocString LongDescription() const override { return {}; }

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

struct CompletionTestContext
{
    CompletionTestContext(std::string_view word, std::string_view commandLine, std::string_view position) :
        context(out, in)
    {
        context.Reporter.SetChannel(Execution::Reporter::Channel::Completion);
        context.Add<Data::CompletionData>(CompletionData{ word, commandLine, position });
    }

    std::stringstream out;
    std::stringstream in;
    Context context;
};

TEST_CASE("CommandComplete_ProxyWordLogRedactsCredentials", "[complete][logging]")
{
    const auto testCase = GENERATE(
        std::pair{ "https://ProxyUser:ProxyPassword@proxy.invalid:8080", "https://[redacted]@proxy.invalid:8080/" },
        std::pair{ "https://proxy.invalid:8080", "https://proxy.invalid:8080/" });
    const bool inlineValue = GENERATE(false, true);
    const bool proxyContext = GENERATE(false, true);
    CAPTURE(testCase, inlineValue, proxyContext);
    const std::string word = (inlineValue ? "--proxy="s : ""s) + testCase.first;
    const std::string expectedWord = proxyContext ?
        (inlineValue ? "--proxy="s : ""s) + testCase.second : word;
    std::string commandLine = "winget ";
    if (!inlineValue || !proxyContext)
    {
        commandLine += proxyContext ? "--proxy " : "--source ";
    }
    commandLine += "\"" + word + "\"";

    TestUserSettings settings;
    GroupPolicyTestOverride policies;
    CompletionTestContext ctc{ word, commandLine, std::to_string(commandLine.size()) };
    auto previousThreadGlobals = ctc.context.SetForCurrentThread();
    CompletionTestCommand command;
    command.Arguments = { Argument::ForType(Args::Type::Source) };
    command.ArgumentValueCallback = [&](Context& context, Args::Type type)
    {
        REQUIRE_FALSE((proxyContext && inlineValue));
        REQUIRE(type == (proxyContext ? Args::Type::Proxy : Args::Type::Source));
        REQUIRE(context.Get<Data::CompletionData>().Word() == word);
        context.Reporter.Completion() << "completion-result" << std::endl;
    };

    TempFile logFile{ "CommandComplete_ProxyWordLog", ".log" };
    Logging::Log().SetEnabledChannels(Logging::Channel::CLI);
    Logging::Log().SetLevel(Logging::Level::Info);
    Logging::FileLogger::Add(logFile.GetPath());
    command.Complete(ctc.context);
    Logging::Log().RemoveLogger(Logging::FileLogger::GetNameForPath(logFile.GetPath()));

    REQUIRE(ctc.context.Get<Data::CompletionData>().Word() == word);
    REQUIRE(ctc.out.str() == (proxyContext && inlineValue ? "" : "completion-result\n"));
    REQUIRE_FALSE(ctc.context.Args.Contains(Args::Type::Proxy));
    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = Utility::ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    REQUIRE(logContents.find("Completing word '" + expectedWord + "'\r\n") != std::string::npos);
    const std::string expectedArguments = inlineValue && proxyContext ? "" : (proxyContext ? " '--proxy'" : " '--source'");
    REQUIRE(logContents.find("Completion invoked for arguments:" + expectedArguments +
        " << [replace] '" + expectedWord + "' >> \r\n") != std::string::npos);
    if (proxyContext)
    {
        REQUIRE(logContents.find("ProxyUser") == std::string::npos);
        REQUIRE(logContents.find("ProxyPassword") == std::string::npos);
    }
}

TEST_CASE("CommandComplete_ProxyArgumentLogRedactsCredentials", "[complete][logging]")
{
    const auto proxy = GENERATE(
        std::pair{ "https://ProxyUser:ProxyPassword@proxy.invalid:8080", "https://[redacted]@proxy.invalid:8080/" },
        std::pair{ "https://proxy.invalid:8080", "https://proxy.invalid:8080/" });
    const bool inlineValue = GENERATE(false, true);
    const std::string_view location = GENERATE("before"sv, "after"sv, "insertion"sv);
    CAPTURE(proxy, inlineValue, location);

    const std::string proxyArgument = (inlineValue ? "--proxy="s : "--proxy "s) + proxy.first;
    const std::string proxyForLog = inlineValue ? " '--proxy="s + proxy.second + "'" :
        " '--proxy' '"s + proxy.second + "'";
    const std::string word = location == "insertion" ? "" : "value";
    const std::string prefix = "winget test "s + (location == "before" ? proxyArgument + " " : "") +
        (location == "insertion" && !inlineValue ? "--proxy " : "");
    const std::string suffix = location == "after" ? " " + proxyArgument :
        (location == "insertion" ? (inlineValue ? proxyArgument : proxy.first) : "");
    const std::string expectedBefore = " 'test'"s + (location == "before" ? proxyForLog : "") +
        (location == "insertion" && !inlineValue ? " '--proxy'" : "");
    const std::string expectedAfter = location == "after" || (location == "insertion" && inlineValue) ? proxyForLog :
        (location == "insertion" ? " '"s + proxy.second + "'" : "");
    const std::string expectedMarker = word.empty() ? " << [insert] >> " : " << [replace] 'value' >> ";

    TestUserSettings settings;
    GroupPolicyTestOverride policies;
    std::stringstream out;
    std::stringstream in;
    Context context{ out, in };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Reporter.SetChannel(Reporter::Channel::Completion);
    TempFile logFile{ "CommandComplete_ProxyArgumentLog", ".log" };
    Logging::Log().SetEnabledChannels(Logging::Channel::CLI);
    Logging::Log().SetLevel(Logging::Level::Info);
    Logging::FileLogger::Add(logFile.GetPath());

    context.Add<Data::CompletionData>(CompletionData{ word, prefix + word + suffix, std::to_string(prefix.size() + word.size()) });
    auto& data = context.Get<Data::CompletionData>();
    const auto before = data.BeforeWord().GetArgs();
    const auto after = data.AfterWord().GetArgs();
    data.BeforeWord().consume(data.BeforeWord().begin());
    CompletionTestCommand command;
    command.Arguments = { Argument::ForType(Args::Type::Query) };
    command.ArgumentValueCallback = [&](Context& completionContext, Args::Type type)
    {
        REQUIRE(type == (location == "insertion" && !inlineValue ? Args::Type::Proxy : Args::Type::Query));
        completionContext.Reporter.Completion() << "completion-result" << std::endl;
    };
    command.Complete(context);
    Logging::Log().RemoveLogger(Logging::FileLogger::GetNameForPath(logFile.GetPath()));

    REQUIRE(data.Word() == word);
    REQUIRE(data.BeforeWord().GetArgs() == before);
    REQUIRE(data.AfterWord().GetArgs() == after);
    REQUIRE(data.BeforeWord().begin().index() == 1);
    REQUIRE(data.AfterWord().begin().index() == 0);
    REQUIRE(context.Args.GetArg(Args::Type::Proxy) == (location == "insertion" && !inlineValue ? ""sv : proxy.first));
    std::stringstream expectedOutput;
    if (location == "insertion" && inlineValue)
    {
        OutputAllArgumentNames(command, expectedOutput);
    }
    expectedOutput << "completion-result\n";
    REQUIRE(out.str() == expectedOutput.str());

    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = Utility::ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    REQUIRE(logContents.find("Completion invoked for arguments:" + expectedBefore + expectedMarker +
        expectedAfter + "\r\n") != std::string::npos);
    REQUIRE(logContents.find("ProxyUser") == std::string::npos);
    REQUIRE(logContents.find("ProxyPassword") == std::string::npos);
}

TEST_CASE("CommandComplete_ProxyArgumentLogPreservesUnrelatedValues", "[complete][logging]")
{
    const bool sourceValue = GENERATE(false, true);
    const std::string proxy = "https://ProxyUser:ProxyPassword@proxy.invalid:8080";
    const std::string argument = sourceValue ? "--source" : "--";
    const std::string commandLine = "winget test --proxy " + proxy + " " + argument + " " + proxy;

    TestUserSettings settings;
    GroupPolicyTestOverride policies;
    CompletionTestContext ctc{ proxy, commandLine, std::to_string(commandLine.size()) };
    auto previousThreadGlobals = ctc.context.SetForCurrentThread();
    auto& data = ctc.context.Get<Data::CompletionData>();
    data.BeforeWord().consume(data.BeforeWord().begin());
    CompletionTestCommand command;
    command.Arguments = { Argument::ForType(Args::Type::Query), Argument::ForType(Args::Type::Source) };
    command.ArgumentValueCallback = [&](Context& context, Args::Type type)
    {
        REQUIRE(type == (sourceValue ? Args::Type::Source : Args::Type::Query));
        REQUIRE(context.Get<Data::CompletionData>().Word() == proxy);
        context.Reporter.Completion() << "completion-result" << std::endl;
    };

    TempFile logFile{ "CommandComplete_ProxyArgumentLogUnrelated", ".log" };
    Logging::Log().SetEnabledChannels(Logging::Channel::CLI);
    Logging::Log().SetLevel(Logging::Level::Info);
    Logging::FileLogger::Add(logFile.GetPath());
    command.Complete(ctc.context);
    Logging::Log().RemoveLogger(Logging::FileLogger::GetNameForPath(logFile.GetPath()));

    REQUIRE(ctc.context.Args.GetArg(Args::Type::Proxy) == proxy);
    REQUIRE(ctc.out.str() == "completion-result\n");
    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = Utility::ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    REQUIRE(logContents.find("Completing word '" + proxy + "'\r\n") != std::string::npos);
    REQUIRE(logContents.find("Completion invoked for arguments: 'test' '--proxy' 'https://[redacted]@proxy.invalid:8080/' '" +
        argument + "' << [replace] '" + proxy + "' >> \r\n") != std::string::npos);
}

TEST_CASE("CommandComplete_DefaultProxyLogRedactsCredentials", "[complete][logging]")
{
    const auto setting = GENERATE(
        std::pair{ "DefaultProxy", true },
        std::pair{ "dEfAuLtPrOxY", true },
        std::pair{ "OtherSetting", false });
    const bool valueBeforeName = GENERATE(false, true);
    const bool completingValue = GENERATE(false, true);
    CAPTURE(setting, valueBeforeName, completingValue);
    const std::string settingName = setting.first;
    const std::string proxy = "https://ProxyUser:ProxyPassword@proxy.invalid:8080";
    const std::string valueForLog = setting.second ? "https://[redacted]@proxy.invalid:8080/" : proxy;
    const std::string valuePrefix = valueBeforeName ? "--value=" : "";
    const std::string word = completingValue ? valuePrefix + proxy : "--he";
    const std::string expectedWord = completingValue ? valuePrefix + valueForLog : word;
    const std::string prefix = "winget settings set " + (valueBeforeName ? "" : settingName + " ") +
        (completingValue ? "" : valuePrefix + proxy + " ");
    const std::string suffix = valueBeforeName ? " --setting " + settingName : "";
    const std::string expectedBefore = " 'settings' 'set'"s + (valueBeforeName ? "" : " '" + settingName + "'") +
        (completingValue ? "" : " '" + valuePrefix + valueForLog + "'");
    const std::string expectedAfter = valueBeforeName ? " '--setting' '" + settingName + "'" : "";

    TestUserSettings settings;
    GroupPolicyTestOverride policies;
    CompletionTestContext ctc{ word, prefix + word + suffix, std::to_string(prefix.size() + word.size()) };
    auto previousThreadGlobals = ctc.context.SetForCurrentThread();
    auto& data = ctc.context.Get<Data::CompletionData>();
    const auto before = data.BeforeWord().GetArgs();
    const auto after = data.AfterWord().GetArgs();
    RootCommand root;
    const auto settingsCommand = root.FindSubCommand(data.BeforeWord());
    REQUIRE(settingsCommand);
    const auto command = settingsCommand->FindSubCommand(data.BeforeWord());
    REQUIRE(command);

    TempFile logFile{ "CommandComplete_DefaultProxyLog", ".log" };
    Logging::Log().SetEnabledChannels(Logging::Channel::CLI);
    Logging::Log().SetLevel(Logging::Level::Info);
    Logging::FileLogger::Add(logFile.GetPath());
    command->Complete(ctc.context);
    Logging::Log().RemoveLogger(Logging::FileLogger::GetNameForPath(logFile.GetPath()));

    CHECK(data.Word() == word);
    CHECK(data.BeforeWord().GetArgs() == before);
    CHECK(data.AfterWord().GetArgs() == after);
    CHECK(ctc.context.Args.GetArg(Args::Type::SettingName) == settingName);
    CHECK(ctc.context.Args.GetArg(Args::Type::SettingValue) == (completingValue ? "" : proxy));
    std::stringstream expectedOutput;
    if (!completingValue)
    {
        OutputAllArgumentNames(*command, expectedOutput, "he");
    }
    CHECK(ctc.out.str() == expectedOutput.str());
    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = Utility::ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    CHECK(logContents.find("Completing word '" + expectedWord + "'\r\n") != std::string::npos);
    CHECK(logContents.find("Completion invoked for arguments:" + expectedBefore +
        " << [replace] '" + expectedWord + "' >> " + expectedAfter + "\r\n") != std::string::npos);
    if (setting.second)
    {
        CHECK(logContents.find("ProxyUser") == std::string::npos);
        CHECK(logContents.find("ProxyPassword") == std::string::npos);
    }
}

TEST_CASE("CommandComplete_Simple", "[complete]")
{
    CompletionTestContext ctc{ "", "winget ", "7" };

    CompletionTestCommand command;
    command.SubCommandNames = { "test1", "test2" };
    command.Arguments = { Argument{ "arg1", 'a', Args::Type::Query, CLI::Resource::String::Done, ArgumentType::Standard } };
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type) { FAIL("No argument value should be requested"); };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;
    OutputAllSubCommands(command, expected);
    OutputAllArgumentNames(command, expected);

    REQUIRE(ctc.out.str() == expected.str());
}

TEST_CASE("CommandComplete_PartialCommandMatch", "[complete]")
{
    CompletionTestContext ctc{ "cart", "winget cart", "11" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type) { FAIL("No argument value should be requested"); };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;
    OutputAllSubCommands(command, expected, "cart");
    OutputAllArgumentNames(command, expected, "cart");

    REQUIRE(ctc.out.str() == expected.str());
}

TEST_CASE("CommandComplete_CommandsNotAllowed", "[complete]")
{
    CompletionTestContext ctc{ "", "winget foobar ", "14" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type) { FAIL("No argument value should be requested"); };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;
    OutputAllArgumentNames(command, expected);

    REQUIRE(ctc.out.str() == expected.str());
}

TEST_CASE("CommandComplete_Routing1", "[complete]")
{
    CompletionTestContext ctc{ "", "winget --arg1 ", "14" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.Arguments = {
        Argument{ "arg1", '1', Args::Type::Query, CLI::Resource::String::Done, ArgumentType::Standard },
        Argument{ "arg2", '2', Args::Type::Channel, CLI::Resource::String::Done, ArgumentType::Standard },
    };
    Args::Type argType = static_cast<Args::Type>(-1);
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type type) { argType = type; };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;

    REQUIRE(ctc.out.str() == expected.str());
    REQUIRE(argType == command.Arguments[0].ExecArgType());
}

TEST_CASE("CommandComplete_Routing2", "[complete]")
{
    CompletionTestContext ctc{ "", "winget --arg2 ", "14" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.Arguments = {
        Argument{ "arg1", '1', Args::Type::Query, CLI::Resource::String::Done, ArgumentType::Standard },
        Argument{ "arg2", '2', Args::Type::Channel, CLI::Resource::String::Done, ArgumentType::Standard },
    };
    Args::Type argType = static_cast<Args::Type>(-1);
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type type) { argType = type; };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;

    REQUIRE(ctc.out.str() == expected.str());
    REQUIRE(argType == command.Arguments[1].ExecArgType());
}

TEST_CASE("CommandComplete_PositionalRouting", "[complete]")
{
    CompletionTestContext ctc{ "", "winget ", "7" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.Arguments = {
        Argument{ "arg1", '1', Args::Type::Query, CLI::Resource::String::Done, ArgumentType::Standard },
        Argument{ "arg2", '2', Args::Type::Channel, CLI::Resource::String::Done, ArgumentType::Positional },
    };
    Args::Type argType = static_cast<Args::Type>(-1);
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type type) { argType = type; };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;
    OutputAllSubCommands(command, expected);
    OutputAllArgumentNames(command, expected);

    REQUIRE(ctc.out.str() == expected.str());
    REQUIRE(argType == command.Arguments[1].ExecArgType());
}

TEST_CASE("CommandComplete_PositionalRoutingAfterArgs", "[complete]")
{
    CompletionTestContext ctc{ "", "winget --arg1 value ", "20" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.Arguments = {
        Argument{ "arg1", '1', Args::Type::Query, CLI::Resource::String::Done, ArgumentType::Standard },
        Argument{ "arg2", '2', Args::Type::Channel, CLI::Resource::String::Done, ArgumentType::Positional },
    };
    Args::Type argType = static_cast<Args::Type>(-1);
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type type) { argType = type; };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;
    OutputAllArgumentNames(command, expected);

    REQUIRE(ctc.out.str() == expected.str());
    REQUIRE(argType == command.Arguments[1].ExecArgType());
}

TEST_CASE("CommandComplete_PositionalRoutingAfterDoubleDash", "[complete]")
{
    CompletionTestContext ctc{ "", "winget -- ", "10" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.Arguments = {
        Argument{ "arg1", '1', Args::Type::Query, CLI::Resource::String::Done, ArgumentType::Standard },
        Argument{ "arg2", '2', Args::Type::Channel, CLI::Resource::String::Done, ArgumentType::Positional },
    };
    Args::Type argType = static_cast<Args::Type>(-1);
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type type) { argType = type; };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;

    REQUIRE(ctc.out.str() == expected.str());
    REQUIRE(argType == command.Arguments[1].ExecArgType());
}

TEST_CASE("CommandComplete_ArgNamesAfterDash", "[complete]")
{
    CompletionTestContext ctc{ "-", "winget -", "8" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.Arguments = {
        Argument{ "arg1", '1', Args::Type::Query, CLI::Resource::String::Done, ArgumentType::Standard },
        Argument{ "arg2", '2', Args::Type::Channel, CLI::Resource::String::Done, ArgumentType::Positional },
    };
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type) { FAIL("No argument value should be requested"); };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;
    OutputAllArgumentNames(command, expected);

    REQUIRE(ctc.out.str() == expected.str());
}

TEST_CASE("CommandComplete_AliasNames", "[complete]")
{
    CompletionTestContext ctc{ "-a", "winget -a", "9" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.Arguments = {
        Argument{ "arg1", '1', Args::Type::Query, CLI::Resource::String::Done, ArgumentType::Standard },
        Argument{ "arg2", '2', Args::Type::Channel, CLI::Resource::String::Done, ArgumentType::Positional },
    };
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type) { FAIL("No argument value should be requested"); };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;
    OutputAllArgumentAliases(command, expected);

    REQUIRE(ctc.out.str() == expected.str());
}

TEST_CASE("CommandComplete_ArgNamesFilter", "[complete]")
{
    CompletionTestContext ctc{ "--a", "winget --a", "10" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.Arguments = {
        Argument{ "arg1", '1', Args::Type::Query, CLI::Resource::String::Done, ArgumentType::Standard },
        Argument{ "arg2", '2', Args::Type::Channel, CLI::Resource::String::Done, ArgumentType::Positional },
        Argument{ "foo1", '2', Args::Type::Channel, CLI::Resource::String::Done, ArgumentType::Positional },
    };
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type) { FAIL("No argument value should be requested"); };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;
    OutputAllArgumentNames(command, expected, "a");

    REQUIRE(ctc.out.str() == expected.str());
}

TEST_CASE("CommandComplete_IgnoreBadArgs", "[complete]")
{
    CompletionTestContext ctc{ "", "winget foo bar --arg1 ", "22" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.Arguments = {
        Argument{ "arg1", '1', Args::Type::Query, CLI::Resource::String::Done, ArgumentType::Standard },
        Argument{ "arg2", '2', Args::Type::Channel, CLI::Resource::String::Done, ArgumentType::Standard },
    };
    Args::Type argType = static_cast<Args::Type>(-1);
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type type) { argType = type; };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;

    REQUIRE(ctc.out.str() == expected.str());
    REQUIRE(argType == command.Arguments[0].ExecArgType());
}

TEST_CASE("CommandComplete_OtherArgsParsed", "[complete]")
{
    CompletionTestContext ctc{ "", "winget --arg1 value1  --arg2 value2", "21" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.Arguments = {
        Argument{ "arg1", '1', Args::Type::Query, CLI::Resource::String::Done, ArgumentType::Standard },
        Argument{ "arg2", '2', Args::Type::Channel, CLI::Resource::String::Done, ArgumentType::Standard },
    };
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type) { FAIL("No argument value should be requested"); };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;
    OutputAllArgumentNames(command, expected);

    REQUIRE(ctc.out.str() == expected.str());
    REQUIRE(ctc.context.Args.Contains(command.Arguments[0].ExecArgType()));
    REQUIRE(ctc.context.Args.GetArg(command.Arguments[0].ExecArgType()) == "value1");
    REQUIRE(ctc.context.Args.Contains(command.Arguments[1].ExecArgType()));
    REQUIRE(ctc.context.Args.GetArg(command.Arguments[1].ExecArgType()) == "value2");
}

TEST_CASE("CommandComplete_Complex", "[complete]")
{
    CompletionTestContext ctc{ "", "winget foo --arg1 value1 bar junk --arg2 ", "41" };

    CompletionTestCommand command;
    command.SubCommandNames = { "car", "cart", "cartesian", "carpet" };
    command.Arguments = {
        Argument{ "arg1", '1', Args::Type::Query, CLI::Resource::String::Done, ArgumentType::Standard },
        Argument{ "arg2", '2', Args::Type::Channel, CLI::Resource::String::Done, ArgumentType::Standard },
    };
    Args::Type argType = static_cast<Args::Type>(-1);
    command.ArgumentValueCallback = [&](Context&, Execution::Args::Type type) { argType = type; };
    command.Complete(ctc.context);

    // Create expected values
    std::stringstream expected;

    REQUIRE(ctc.out.str() == expected.str());
    REQUIRE(argType == command.Arguments[1].ExecArgType());
    REQUIRE(ctc.context.Args.Contains(command.Arguments[0].ExecArgType()));
    REQUIRE(ctc.context.Args.GetArg(command.Arguments[0].ExecArgType()) == "value1");
}
