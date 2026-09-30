// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include <Commands/InstallCommand.h>
#include <Commands/DownloadCommand.h>
#include <Commands/ShowCommand.h>
#include <Workflows/PromptFlow.h>
#include <Workflows/DownloadFlow.h>
#include <Workflows/InstallFlow.h>
#include <Workflows/ShowFlow.h>
#include <winget/ManifestYamlParser.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;

TEST_CASE("PackageSelection_Prompt", "[PackageSelection][PromptFlow]")
{
    TestUserSettings settings;
    auto response = GENERATE("1", "2", "10", " 2 \t", "0");
    std::istringstream input{ response };
    std::ostringstream output;
    TestContext context{ output, input };
    context.Reporter.SetConsoleStreamsForTest(true);
    Execution::TableOutput<1> table(context.Reporter, { Resource::String::SearchName });
    for (size_t i = 1; i <= 10; ++i)
    {
        table.OutputLine({ std::to_string(i) });
    }

    context << PromptForSelection(table, Resource::String::PackageSelectionInstall,
        Resource::String::PackageSelectionInvalid);
    auto selection = context.Get<Execution::Data::PromptSelection>();
    if (std::string_view{ response } == "0")
    {
        REQUIRE_FALSE(selection);
        REQUIRE_TERMINATED_WITH(context, E_ABORT);
    }
    else
    {
        REQUIRE_FALSE(context.IsTerminated());
        REQUIRE(selection == static_cast<size_t>(std::stoul(response) - 1));
    }
    REQUIRE(output.str().find(Resource::String::NumberedSelectionPrompt(10).get()) != std::string::npos);
}

TEST_CASE("PackageSelection_InvalidInput", "[PackageSelection][PromptFlow]")
{
    TestUserSettings settings;
    auto response = GENERATE("", " ", "-1", "+1", "3", "1x", "1.0", "1 2", "99999999999999999999999999");
    std::istringstream input{ std::string{ response } + "\n2\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    context.Reporter.SetConsoleStreamsForTest(true);
    Execution::TableOutput<2> table(context.Reporter, { Resource::String::SearchName, Resource::String::SearchSource });
    table.OutputLine({ "First", "FirstSource" });
    if (GENERATE(false, true))
    {
        table.OutputLine({ "", "SecondSource" });
    }
    table.OutputLine({ "Second", "FirstSource" });

    context << PromptForSelection(table, Resource::String::PackageSelectionInstall,
        Resource::String::PackageSelectionInvalid);
    REQUIRE_FALSE(context.IsTerminated());
    REQUIRE(context.Get<Execution::Data::PromptSelection>() == 1);
    const std::string prompt = Resource::String::NumberedSelectionPrompt(2).get();
    const std::string invalid = Resource::LocString{ Resource::String::PackageSelectionInvalid }.get();
    REQUIRE(output.str().find(prompt + " " + invalid + '\n' + prompt + " ") != std::string::npos);
}

TEST_CASE("PromptFlow_Selection_CustomStrings", "[PromptFlow]")
{
    TestUserSettings settings;
    std::istringstream input{ "wrong\n2\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    context.Reporter.SetConsoleStreamsForTest(true);
    context.Reporter.SetStyle(VisualStyle::NoVT);
    TestHook::SetConsoleWidth_Override widthOverride{ std::optional<size_t>{120} };
    auto text = [](std::string value)
    {
        return Resource::LocString{ AppInstaller::Utility::LocIndString{ std::move(value) } };
    };
    Execution::TableOutput<1> table(context.Reporter, { text("Choice") });
    table.OutputLine({ "1 First" });
    table.OutputLine({ "2 Second" });
    context << PromptForSelection(table, text("Choose a value"), text("Try again"));

    REQUIRE_FALSE(context.IsTerminated());
    REQUIRE(context.Get<Execution::Data::PromptSelection>() == 1);
    const std::string prompt = Resource::String::NumberedSelectionPrompt(2).get();
    REQUIRE_FALSE(prompt.empty());
    REQUIRE(output.str() == "Choose a value\n\nChoice\n--------\n1 First\n2 Second\n\n" + prompt + " Try again\n" + prompt + " ");
}

TEST_CASE("PromptFlow_Selection_Unavailable", "[PromptFlow]")
{
    TestUserSettings settings;
    std::istringstream input{ "1\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    context.Reporter.SetConsoleStreamsForTest(true);
    context.Add<Execution::Data::PromptSelection>(std::optional<size_t>{0});
    Execution::TableOutput<1> table(context.Reporter, { Resource::String::SearchName });
    table.OutputLine({ "First" });

    SECTION("Context disabled")
    {
        context.SetFlags(Execution::ContextFlag::DisableInteractivity);
    }
    SECTION("Argument disabled")
    {
        context.Args.AddArg(Execution::Args::Type::DisableInteractivity);
    }
    SECTION("Setting disabled")
    {
        settings.Set<Setting::InteractivityDisable>(true);
    }
    SECTION("Redirected streams")
    {
        context.Reporter.SetConsoleStreamsForTest(false);
    }
    SECTION("Hidden output")
    {
        context.Reporter.SetLevelMask(Execution::Reporter::Level::Info, false);
    }

    context << PromptForSelection(table, Resource::String::PackageSelectionInstall,
        Resource::String::PackageSelectionInvalid);
    REQUIRE_FALSE(context.IsTerminated());
    REQUIRE_FALSE(context.Get<Execution::Data::PromptSelection>());
    REQUIRE(output.str().empty());
    REQUIRE(input.peek() == '1');
}

TEST_CASE("PromptFlow_Selection_InputFailure", "[PromptFlow]")
{
    TestUserSettings settings;
    std::istringstream input;
    std::ostringstream output;
    TestContext context{ output, input };
    context.Reporter.SetConsoleStreamsForTest(true);
    Execution::TableOutput<1> table(context.Reporter, { Resource::String::SearchName });
    auto count = GENERATE(size_t{0}, size_t{1});
    if (count)
    {
        table.OutputLine({ "First" });
    }
    if (GENERATE(false, true))
    {
        table.OutputLine({ "" });
    }
    PromptForSelection prompt(table, Resource::String::PackageSelectionInstall,
        Resource::String::PackageSelectionInvalid);

    REQUIRE_THROWS_HR(prompt(context), count ? APPINSTALLER_CLI_ERROR_PROMPT_INPUT_ERROR : E_INVALIDARG);
    REQUIRE_FALSE(context.Get<Execution::Data::PromptSelection>());
}

TEST_CASE("ReporterReadLine", "[PromptFlow]")
{
    auto response = GENERATE("", "  text \t", "0", "invalid", "99999999999999999999999999");
    std::istringstream input{ std::string{ response } + '\n' + "next\n" };
    std::ostringstream output;
    Execution::Reporter reporter{ output, input };
    reporter.SetConsoleStreamsForTest(true);

    REQUIRE(reporter.ReadLine() == response);
    REQUIRE(input.peek() == 'n');
    REQUIRE(output.str().empty());
}

TEST_CASE("ReporterReadLine_InputFailure", "[PromptFlow]")
{
    std::istringstream input;
    std::ostringstream output;
    Execution::Reporter reporter{ output, input };
    reporter.SetConsoleStreamsForTest(true);

    SECTION("EOF")
    {
        REQUIRE_THROWS_HR(reporter.ReadLine(), APPINSTALLER_CLI_ERROR_PROMPT_INPUT_ERROR);
    }
    SECTION("Cancelled input failure")
    {
        int checks = 0;
        REQUIRE_FALSE(reporter.ReadLine([&]() { return ++checks == 3; }));
    }
    SECTION("Console read aborts before the signal handler runs")
    {
        struct AbortedInputBuffer : std::streambuf
        {
            int_type underflow() override
            {
                SetLastError(ERROR_OPERATION_ABORTED);
                return traits_type::eof();
            }
        } buffer;
        std::istream abortedInput{ &buffer };
        Execution::Reporter abortedReporter{ output, abortedInput };
        abortedReporter.SetConsoleStreamsForTest(true);
        REQUIRE_FALSE(abortedReporter.ReadLine([]() { return false; }));
    }
    REQUIRE(output.str().empty());
}

TEST_CASE("ReporterReadLine_CancelBeforeRead", "[PromptFlow]")
{
    auto cancelOnCheck = GENERATE(1, 2, 3);
    std::istringstream input{ "invalid\n2\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    context.Reporter.SetConsoleStreamsForTest(true);
    int checks = 0;
    auto isCancelled = [&]()
    {
        if (++checks == cancelOnCheck)
        {
            context.Terminate(E_ABORT);
        }
        return context.IsTerminated();
    };

    REQUIRE_FALSE(context.Reporter.ReadLine(isCancelled));
    REQUIRE_TERMINATED_WITH(context, E_ABORT);
    REQUIRE(input.peek() == (cancelOnCheck >= 3 ? '2' : 'i'));
    REQUIRE(output.str().empty());
}

TEST_CASE("ReporterReadLine_CancelPendingRead", "[PromptFlow]")
{
    struct PipeInputBuffer : std::streambuf
    {
        wil::unique_handle ReadHandle;
        wil::unique_handle WriteHandle;
        wil::unique_event ReadStarted{ wil::EventOptions::ManualReset };
        char Character = 0;

        PipeInputBuffer()
        {
            THROW_IF_WIN32_BOOL_FALSE(CreatePipe(ReadHandle.put(), WriteHandle.put(), nullptr, 0));
        }

        int_type underflow() override
        {
            ReadStarted.SetEvent();
            DWORD count = 0;
            if (!ReadFile(ReadHandle.get(), &Character, 1, &count, nullptr) || !count)
            {
                return traits_type::eof();
            }
            setg(&Character, &Character, &Character + 1);
            return traits_type::to_int_type(Character);
        }
    };

    PipeInputBuffer buffer;
    std::istream input{ &buffer };
    std::ostringstream output;
    TestContext context{ output, input };
    context.Reporter.SetConsoleStreamsForTest(true);
    wil::unique_event finished{ wil::EventOptions::ManualReset };
    bool timedOut = false;
    std::thread cancel([&]()
    {
        if (buffer.ReadStarted.wait(5000))
        {
            context.Cancel(AppInstaller::CancelReason::CtrlCSignal);
        }
    });
    std::thread watchdog([&]()
    {
        if (!finished.wait(5000))
        {
            timedOut = true;
            DWORD written = 0;
            LOG_IF_WIN32_BOOL_FALSE(WriteFile(buffer.WriteHandle.get(), "\n", 1, &written, nullptr));
        }
    });
    auto join = wil::scope_exit([&]()
    {
        finished.SetEvent();
        cancel.join();
        watchdog.join();
    });

    REQUIRE_FALSE(context.Reporter.ReadLine([&]() { return context.IsTerminated(); }));
    finished.SetEvent();
    cancel.join();
    watchdog.join();
    join.release();
    REQUIRE_FALSE(timedOut);
    REQUIRE_TERMINATED_WITH(context, E_ABORT);
    REQUIRE(output.str().empty());
}

TEST_CASE("PackageSelection_ConsoleStreams", "[PackageSelection][PromptFlow]")
{
    Execution::Reporter reporter;
    DWORD mode = 0;
    bool consoleStreams = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) &&
        GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode);
    REQUIRE(reporter.CanPrompt() == consoleStreams);

    reporter.SetConsoleStreamsForTest(GENERATE(false, true));
    Execution::Reporter clone{ reporter, Execution::Reporter::clone_t{} };
    REQUIRE(clone.CanPrompt() == reporter.CanPrompt());
}

TEST_CASE("PackageSelection_ReporterUnavailable", "[PackageSelection][PromptFlow]")
{
    std::istringstream input{ "1\n" };
    std::ostringstream output;
    Execution::Reporter reporter{ output, input };
    reporter.SetConsoleStreamsForTest(true);

    SECTION("Redirected streams")
    {
        reporter.SetConsoleStreamsForTest(false);
    }
    SECTION("Hidden informational output")
    {
        reporter.SetLevelMask(Execution::Reporter::Level::Info, false);
    }
    SECTION("Non-output channel")
    {
        reporter.SetChannel(GENERATE(Execution::Reporter::Channel::Completion, Execution::Reporter::Channel::Json,
            Execution::Reporter::Channel::Disabled));
    }

    REQUIRE_FALSE(reporter.CanPrompt());
    REQUIRE_THROWS_HR(reporter.ReadLine(), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
    REQUIRE(input.peek() == '1');
}

TEST_CASE("PackageSelection_CommandCancel", "[PackageSelection][workflow]")
{
    TestUserSettings settings;
    std::istringstream input{ "0\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Reporter.SetConsoleStreamsForTest(true);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestQuery_ReturnTwo.Query);
    OverrideForOpenSource(context, CreateTestSource({ TSR::TestQuery_ReturnTwo }));

    SECTION("Install")
    {
        bool silent = GENERATE(false, true);
        if (silent)
        {
            context.Args.AddArg(Execution::Args::Type::Silent);
        }
        context.Args.AddArg(Execution::Args::Type::Force);
        InstallCommand({}).Execute(context);
    }
    SECTION("Show")
    {
        ShowCommand({}).Execute(context);
    }
    SECTION("Show versions")
    {
        context.Args.AddArg(Execution::Args::Type::ListVersions);
        ShowCommand({}).Execute(context);
    }
    SECTION("Download")
    {
        DownloadCommand({}).Execute(context);
    }

    INFO(output.str());
    REQUIRE_TERMINATED_WITH(context, E_ABORT);
    REQUIRE_FALSE(context.Contains(Execution::Data::Package));
    REQUIRE_FALSE(context.Contains(Execution::Data::Manifest));
    REQUIRE(output.str().find(Resource::String::NumberedSelectionPrompt(2).get()) != std::string::npos);
}

TEST_CASE("PackageSelection_CommandContinue", "[PackageSelection][workflow]")
{
    TestUserSettings settings;
    std::istringstream input{ "1\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Reporter.SetConsoleStreamsForTest(true);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestQuery_ReturnTwo.Query);
    context.Args.AddArg(Execution::Args::Type::Version, "1.0.0.0"sv);
    OverrideForOpenSource(context, CreateTestSource({ TSR::TestQuery_ReturnTwo }));

    auto checkSelection = [](TestContext& selectedContext)
    {
        REQUIRE(selectedContext.Get<Execution::Data::Package>()->GetProperty(AppInstaller::Repository::PackageProperty::Id) ==
            "AppInstallerCliTest.TestExeInstaller");
        REQUIRE(selectedContext.Get<Execution::Data::Manifest>().Version == "1.0.0.0");
        REQUIRE(selectedContext.Args.GetArg(Execution::Args::Type::Version) == "1.0.0.0");
        selectedContext.Terminate(E_ABORT);
    };

    SECTION("Install")
    {
        bool silent = GENERATE(false, true);
        if (silent)
        {
            context.Args.AddArg(Execution::Args::Type::Silent);
        }
        context.Args.AddArg(Execution::Args::Type::Force);
        context.Override({ Workflow::InstallSinglePackage, checkSelection, 1 });
        InstallCommand({}).Execute(context);
        REQUIRE(context.Args.Contains(Execution::Args::Type::Silent) == silent);
    }
    SECTION("Show")
    {
        context.Override({ Workflow::ShowManifestInfo, checkSelection, 1 });
        ShowCommand({}).Execute(context);
    }
    SECTION("Download")
    {
        context.Override({ Workflow::SetDownloadDirectory, checkSelection, 1 });
        DownloadCommand({}).Execute(context);
    }

    INFO(output.str());
    REQUIRE_TERMINATED_WITH(context, E_ABORT);
    REQUIRE(output.str().find(Resource::String::NumberedSelectionPrompt(2).get()) != std::string::npos);
}

TEST_CASE("PackageSelection_MultipleQueries", "[PackageSelection][workflow][MultiQuery]")
{
    TestUserSettings settings;
    std::istringstream input{ "1\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Reporter.SetConsoleStreamsForTest(true);
    context.Args.AddArg(Execution::Args::Type::Force);
    context.Args.AddArg(Execution::Args::Type::MultiQuery, TSR::TestQuery_ReturnTwo.Query);
    context.Args.AddArg(Execution::Args::Type::MultiQuery, "MissingPackage"sv);
    OverrideForOpenSource(context, CreateTestSource({ TSR::TestQuery_ReturnTwo }));
    context.Override({ Workflow::GetSearchRequestForSingle, [](TestContext& subContext)
    {
        subContext.Reporter.SetConsoleStreamsForTest(true);
        Workflow::GetSearchRequestForSingle(subContext);
    } });

    InstallCommand({}).Execute(context);
    INFO(output.str());
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NOT_ALL_QUERIES_FOUND_SINGLE);
    REQUIRE(input.peek() == '1');
    REQUIRE(output.str().find(Resource::String::NumberedSelectionPrompt(2).get()) == std::string::npos);
}

TEST_CASE("PackageSelection_SearchResult", "[PackageSelection][SourcePriority][workflow]")
{
    TestUserSettings settings;
    auto width = GENERATE(size_t{20}, size_t{120});
    TestHook::SetConsoleWidth_Override widthOverride{ std::optional<size_t>{width} };
    auto operation = GENERATE(OperationType::Install, OperationType::Show, OperationType::Download);
    auto manifest = AppInstaller::Manifest::YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
    std::vector<AppInstaller::Manifest::Manifest> versions{ manifest };
    auto firstSource = std::make_shared<TestSource>();
    auto secondSource = std::make_shared<TestSource>();
    auto lowPriority = std::make_shared<TestSource>();
    firstSource->Details.Name = "FirstSource";
    secondSource->Details.Name = "SecondSource";
    SearchResult result;
    result.Matches.emplace_back(TestCompositePackage::Make(versions, firstSource),
        PackageMatchFilter{ PackageMatchField::Id, MatchType::Exact, manifest.Id });
    result.Matches.emplace_back(TestCompositePackage::Make(versions, secondSource),
        PackageMatchFilter{ PackageMatchField::Id, MatchType::Exact, manifest.Id });
    auto expectedPackage = result.Matches[1].Package;
    bool expectPrompt = true;
    bool expectSecondSource = true;
    size_t expectedRows = 2;

    SECTION("Same identity across sources")
    {
    }
    SECTION("Single source")
    {
        result.Matches[1].Package = TestCompositePackage::Make(versions, firstSource);
        expectedPackage = result.Matches[1].Package;
        expectSecondSource = false;
    }
    SECTION("Multiple available sources for a candidate")
    {
        auto package = TestCompositePackage::Make(versions, firstSource);
        package->Available.emplace_back(TestPackage::Make(versions, secondSource));
        result.Matches[0].Package = package;
        expectedRows = 3;
    }
    SECTION("Unique source priority")
    {
        secondSource->Details.Priority = 1;
        expectPrompt = false;
    }
    SECTION("Priority tie")
    {
        firstSource->Details.Priority = 1;
        secondSource->Details.Priority = 1;
        auto excluded = TestCompositePackage::Make(versions, lowPriority);
        result.Matches.insert(result.Matches.begin(), ResultMatch{ excluded,
            PackageMatchFilter{ PackageMatchField::Id, MatchType::Exact, manifest.Id } });
    }
    SECTION("Single match")
    {
        result.Matches.erase(result.Matches.begin());
        expectPrompt = false;
    }

    std::istringstream input{ "2\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Reporter.SetConsoleStreamsForTest(true);
    context.Reporter.SetStyle(VisualStyle::NoVT);
    context.Add<Execution::Data::SearchResult>(std::move(result));
    context << EnsureOneMatchFromSearchResult(operation, PackageSelectionBehavior::Prompt);

    INFO(output.str());
    REQUIRE_FALSE(context.IsTerminated());
    REQUIRE(context.Get<Execution::Data::Package>() == expectedPackage);
    REQUIRE((output.str().find(Resource::String::NumberedSelectionPrompt(2).get()) != std::string::npos) == expectPrompt);
    if (expectPrompt)
    {
        auto tableStart = output.str().find("\n# ");
        REQUIRE(tableStart != std::string::npos);
        auto tableEnd = output.str().find("\n\n", tableStart + 1);
        REQUIRE(tableEnd != std::string::npos);
        auto tableText = output.str().substr(tableStart + 1, tableEnd - tableStart - 1);
        REQUIRE(tableText.find("\n1 ") != std::string::npos);
        REQUIRE(tableText.find("\n2 ") != std::string::npos);
        std::istringstream tableStream{ tableText };
        std::string line;
        size_t lineCount = 0;
        while (std::getline(tableStream, line))
        {
            REQUIRE(AppInstaller::Utility::UTF8ColumnWidth(line) < width);
            ++lineCount;
        }
        REQUIRE(lineCount == expectedRows + 2);
        if (width == 120)
        {
            REQUIRE(tableText.find(manifest.DefaultLocalization.Get<AppInstaller::Manifest::Localization::PackageName>()) != std::string::npos);
            REQUIRE(tableText.find(manifest.Id) != std::string::npos);
            REQUIRE(tableText.find(manifest.Version) != std::string::npos);
            REQUIRE(tableText.find(Resource::LocString{ Resource::String::SearchSource }.get()) != std::string::npos);
            REQUIRE(tableText.find("FirstSource") != std::string::npos);
            REQUIRE((tableText.find("SecondSource") != std::string::npos) == expectSecondSource);
        }
        else
        {
            REQUIRE(tableText.find("\xE2\x80\xA6") != std::string::npos);
        }
    }
    else
    {
        REQUIRE(input.peek() == '2');
    }
    REQUIRE(firstSource->CountOfCallsRequiringManifestData == 0);
    REQUIRE(secondSource->CountOfCallsRequiringManifestData == 0);
}

TEST_CASE("PackageSelection_SourceRowIdentity", "[PackageSelection][workflow]")
{
    struct PrimaryPackage : TestCompositePackage
    {
        using TestCompositePackage::TestCompositePackage;

        LocIndString GetProperty(PackageProperty property) const override
        {
            return Available.at(1)->GetProperty(property);
        }
    };

    TestUserSettings settings;
    TestHook::SetConsoleWidth_Override widthOverride{ std::optional<size_t>{120} };
    auto operation = GENERATE(OperationType::Install, OperationType::Show, OperationType::Download);
    bool differentName = GENERATE(false, true);
    bool differentId = GENERATE(false, true);
    bool missingMetadata = GENERATE(false, true);
    auto manifest = AppInstaller::Manifest::YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
    manifest.Id = "Public.App";
    manifest.Version = "1.0";
    manifest.DefaultLocalization.Add<AppInstaller::Manifest::Localization::PackageName>("PublicName");
    auto firstSource = std::make_shared<TestSource>();
    auto secondSource = std::make_shared<TestSource>();
    firstSource->Details.Name = "FirstSource";
    secondSource->Details.Name = "SecondSource";
    auto package = std::make_shared<PrimaryPackage>(std::vector{ manifest }, firstSource);

    std::string secondName = differentName ? "PrivateName" : "PublicName";
    std::string secondId = differentId ? "Private.App" : "Public.App";
    manifest.Id = secondId;
    manifest.Version = "2.0";
    manifest.DefaultLocalization.Add<AppInstaller::Manifest::Localization::PackageName>(secondName);
    package->Available.emplace_back(TestPackage::Make(std::vector{ manifest }, secondSource));
    manifest.Version = std::string{ missingMetadata ? "" : "3.0" };
    auto thirdPackage = TestPackage::Make(std::vector{ manifest }, secondSource);
    if (missingMetadata)
    {
        thirdPackage->Source.reset();
    }
    package->Available.emplace_back(std::move(thirdPackage));

    SearchResult result;
    result.Matches.emplace_back(package, PackageMatchFilter{ PackageMatchField::Id, MatchType::Exact, secondId });
    manifest.Id = "Other.App";
    manifest.Version = "4.0";
    manifest.DefaultLocalization.Add<AppInstaller::Manifest::Localization::PackageName>("OtherName");
    result.Matches.emplace_back(TestCompositePackage::Make(std::vector{ manifest }, firstSource),
        PackageMatchFilter{ PackageMatchField::Id, MatchType::Exact, manifest.Id });

    std::istringstream input{ "1\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Reporter.SetConsoleStreamsForTest(true);
    context.Reporter.SetStyle(VisualStyle::NoVT);
    context.Add<Execution::Data::SearchResult>(std::move(result));
    context << EnsureOneMatchFromSearchResult(operation, PackageSelectionBehavior::Prompt);

    INFO(output.str());
    REQUIRE_FALSE(context.IsTerminated());
    REQUIRE(context.Get<Execution::Data::Package>() == package);
    auto tableStart = output.str().find("\n# ");
    REQUIRE(tableStart != std::string::npos);
    auto tableEnd = output.str().find("\n\n", tableStart + 1);
    REQUIRE(tableEnd != std::string::npos);
    std::istringstream tableStream{ output.str().substr(tableStart + 1, tableEnd - tableStart - 1) };
    std::string line;
    REQUIRE(static_cast<bool>(std::getline(tableStream, line)));
    REQUIRE(static_cast<bool>(std::getline(tableStream, line)));
    const std::string unavailable = Resource::LocString{ Resource::String::Unavailable }.get();
    std::vector<std::vector<std::string>> expectedRows{
        { "1", "PublicName", "Public.App", "1.0", "FirstSource" },
        differentName || differentId ? std::vector<std::string>{ secondName, secondId, "2.0", "SecondSource" } :
            std::vector<std::string>{ "2.0", "SecondSource" },
        { missingMetadata ? unavailable : "3.0", missingMetadata ? unavailable : "SecondSource" },
        { "2", "OtherName", "Other.App", "4.0", "FirstSource" }
    };
    for (const auto& expectedRow : expectedRows)
    {
        REQUIRE(static_cast<bool>(std::getline(tableStream, line)));
        std::istringstream rowStream{ line };
        std::vector<std::string> fields;
        std::string field;
        while (rowStream >> field)
        {
            fields.emplace_back(std::move(field));
        }
        REQUIRE(fields == expectedRow);
    }
    REQUIRE_FALSE(std::getline(tableStream, line));
    REQUIRE(firstSource->CountOfCallsRequiringManifestData == 0);
    REQUIRE(secondSource->CountOfCallsRequiringManifestData == 0);
}

TEST_CASE("PackageSelection_Unavailable", "[PackageSelection][workflow]")
{
    TestUserSettings settings;
    std::istringstream input{ "2\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Reporter.SetConsoleStreamsForTest(true);
    auto source = CreateTestSource({ TSR::TestQuery_ReturnTwo });
    auto result = source->Search({});
    auto operation = OperationType::Install;
    auto selectionBehavior = PackageSelectionBehavior::Prompt;
    HRESULT expectedError = APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND;

    SECTION("Default workflow")
    {
        selectionBehavior = PackageSelectionBehavior::Disabled;
    }
    SECTION("Context disabled")
    {
        context.SetFlags(Execution::ContextFlag::DisableInteractivity);
    }
    SECTION("Argument disabled")
    {
        context.Args.AddArg(Execution::Args::Type::DisableInteractivity);
    }
    SECTION("Setting disabled")
    {
        settings.Set<Setting::InteractivityDisable>(true);
    }
    SECTION("Silent with interactivity disabled")
    {
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::DisableInteractivity);
    }
    SECTION("Redirected streams")
    {
        context.Reporter.SetConsoleStreamsForTest(false);
    }
    SECTION("Hidden output")
    {
        context.Reporter.SetChannel(Execution::Reporter::Channel::Disabled);
    }
    SECTION("Truncated results")
    {
        result.Truncated = true;
    }
    SECTION("No matches")
    {
        result.Matches.clear();
        expectedError = APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND;
    }
    SECTION("Excluded operation")
    {
        operation = GENERATE(OperationType::Upgrade, OperationType::Uninstall, OperationType::Repair,
            OperationType::Export, OperationType::Pin, OperationType::Search, OperationType::List, OperationType::Completion);
    }

    context.Add<Execution::Data::SearchResult>(std::move(result));
    context << EnsureOneMatchFromSearchResult(operation, selectionBehavior);
    INFO(output.str());
    REQUIRE_TERMINATED_WITH(context, expectedError);
    REQUIRE_FALSE(context.Contains(Execution::Data::Package));
    REQUIRE(input.peek() == '2');
    REQUIRE(output.str().find(Resource::String::NumberedSelectionPrompt(2).get()) == std::string::npos);
}

TEST_CASE("PromptFlow_InteractivityDisabled", "[PromptFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_LicenseAgreement.yaml").GetPath().u8string());

    SECTION("Disabled by setting")
    {
        testSettings.Set<Setting::InteractivityDisable>(true);
    }
    SECTION("Disabled by arg")
    {
        context.Args.AddArg(Execution::Args::Type::DisableInteractivity);
    }

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify prompt is not shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::PackageAgreementsPrompt).get()) == std::string::npos);

    // Verify installation failed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
    REQUIRE_FALSE(std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::PackageAgreementsNotAgreedTo).get()) != std::string::npos);
}

TEST_CASE("PromptFlow_InstallerAbortsTerminal_Proceed", "[PromptFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    // Accept that the installer may abort the terminal by saying "Yes" at the prompt
    std::istringstream installInput{ "y" };

    std::ostringstream installOutput;
    TestContext context{ installOutput, installInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_AbortsTerminal.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify prompt is shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InstallerAbortsTerminal).get()) != std::string::npos);

    // Verify Installer is called.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("PromptFlow_InstallerAbortsTerminal_Cancel", "[PromptFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    // Cancel the installation by saying "No" at the prompt that the installer may abort the terminal
    std::istringstream installInput{ "n" };

    std::ostringstream installOutput;
    TestContext context{ installOutput, installInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_AbortsTerminal.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify prompt is shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InstallerAbortsTerminal).get()) != std::string::npos);

    // Verify installation failed
    REQUIRE_TERMINATED_WITH(context, E_ABORT);
    REQUIRE_FALSE(std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("PromptFlow_InstallLocationRequired", "[PromptFlow][workflow]")
{
    TestCommon::TempDirectory installLocation("TempDirectory");
    TestCommon::TestUserSettings testSettings;

    std::istringstream installInput;
    std::ostringstream installOutput;
    TestContext context{ installOutput, installInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_InstallLocationRequired.yaml").GetPath().u8string());

    bool shouldShowPrompt = false;
    std::filesystem::path installResultPath = installLocation.GetPath() / "TestExeInstalled.txt";
    SECTION("From argument")
    {
        context.Args.AddArg(Execution::Args::Type::InstallLocation, installLocation.GetPath().string());
    }
    SECTION("From settings")
    {
        testSettings.Set<Setting::InstallDefaultRoot>(installLocation.GetPath().string());

        // When using the default location from settings, the Package ID is appended to the root
        auto installLocationWithPackageId = installLocation.GetPath() / "AppInstallerCliTest.TestInstaller";
        std::filesystem::create_directory(installLocationWithPackageId);
        installResultPath = installLocationWithPackageId / "TestExeInstalled.txt";
    }
    SECTION("From prompt")
    {
        installInput.str(installLocation.GetPath().string());
        shouldShowPrompt = true;
    }

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    bool promptShown = installOutput.str().find(Resource::LocString(Resource::String::InstallerRequiresInstallLocation).get()) != std::string::npos;
    REQUIRE(shouldShowPrompt == promptShown);

    // Verify Installer is called with the right parameters
    REQUIRE(std::filesystem::exists(installResultPath));
    std::ifstream installResultFile(installResultPath);
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    const auto installDirArgument = "/InstallDir " + installLocation.GetPath().string();
    REQUIRE(installResultStr.find(installDirArgument) != std::string::npos);
}

TEST_CASE("PromptFlow_InstallLocationRequired_Missing", "[PromptFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_InstallLocationRequired.yaml").GetPath().u8string());
    // Disable interactivity so that there is not prompt and we cannot get the required location
    context.Args.AddArg(Execution::Args::Type::DisableInteractivity);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify prompt is shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InstallerRequiresInstallLocation).get()) != std::string::npos);

    // Verify installation failed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_INSTALL_LOCATION_REQUIRED);
    REQUIRE_FALSE(std::filesystem::exists(installResultPath.GetPath()));
}
