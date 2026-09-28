// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include <Commands/InstallCommand.h>
#include <Commands/DownloadCommand.h>
#include <Commands/ShowCommand.h>
#include <Workflows/PromptFlow.h>
#include <Workflows/DownloadFlow.h>
#include <Workflows/InstallFlow.h>
#include <Workflows/ShowFlow.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Settings;

TEST_CASE("PackageSelection_Prompt", "[PackageSelection][PromptFlow]")
{
    auto response = GENERATE("1", "2", "10", " 2 \t", "0");
    std::istringstream input{ response };
    std::ostringstream output;
    Execution::Reporter reporter{ output, input };
    reporter.SetConsoleStreamsForTest(true);

    auto selection = reporter.PromptForSelection(10);
    if (std::string_view{ response } == "0")
    {
        REQUIRE_FALSE(selection);
    }
    else
    {
        REQUIRE(selection == static_cast<size_t>(std::stoul(response) - 1));
    }
    REQUIRE(output.str().find(Resource::String::PackageSelectionPrompt(10).get()) != std::string::npos);
}

TEST_CASE("PackageSelection_InvalidInput", "[PackageSelection][PromptFlow]")
{
    auto response = GENERATE("", " ", "-1", "+1", "3", "1x", "1.0", "1 2", "99999999999999999999999999");
    std::istringstream input{ std::string{ response } + "\n2\n" };
    std::ostringstream output;
    Execution::Reporter reporter{ output, input };
    reporter.SetConsoleStreamsForTest(true);

    REQUIRE(reporter.PromptForSelection(2) == 1);
    REQUIRE(output.str().find(Resource::String::PackageSelectionInvalid(2).get()) != std::string::npos);
}

TEST_CASE("PackageSelection_InputFailure", "[PackageSelection][PromptFlow]")
{
    std::istringstream input;
    std::ostringstream output;
    Execution::Reporter reporter{ output, input };
    reporter.SetConsoleStreamsForTest(true);

    SECTION("EOF")
    {
        REQUIRE_THROWS_HR(reporter.PromptForSelection(2), APPINSTALLER_CLI_ERROR_PROMPT_INPUT_ERROR);
    }
    SECTION("Cancelled input failure")
    {
        int checks = 0;
        REQUIRE_FALSE(reporter.PromptForSelection(2, [&]() { return ++checks == 3; }));
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
        REQUIRE_FALSE(abortedReporter.PromptForSelection(2, []() { return false; }));
    }
}

TEST_CASE("PackageSelection_CancelBeforeRead", "[PackageSelection][PromptFlow]")
{
    auto cancelOnCheck = GENERATE(1, 2, 3, 4);
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

    REQUIRE_FALSE(context.Reporter.PromptForSelection(2, isCancelled));
    REQUIRE_TERMINATED_WITH(context, E_ABORT);
    REQUIRE(input.peek() == (cancelOnCheck >= 3 ? '2' : 'i'));
    if (cancelOnCheck == 1)
    {
        REQUIRE(output.str().empty());
    }
}

TEST_CASE("PackageSelection_CancelPendingRead", "[PackageSelection][PromptFlow]")
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

    REQUIRE_FALSE(context.Reporter.PromptForSelection(2, [&]() { return context.IsTerminated(); }));
    finished.SetEvent();
    cancel.join();
    watchdog.join();
    join.release();
    REQUIRE_FALSE(timedOut);
    REQUIRE_TERMINATED_WITH(context, E_ABORT);
    REQUIRE(output.str().find(Resource::String::PackageSelectionInvalid(2).get()) == std::string::npos);
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
    REQUIRE_THROWS_HR(reporter.PromptForSelection(2), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
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
    REQUIRE(output.str().find(Resource::String::PackageSelectionPrompt(2).get()) != std::string::npos);
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
        context.Args.AddArg(Execution::Args::Type::Force);
        context.Override({ Workflow::InstallSinglePackage, checkSelection, 1 });
        InstallCommand({}).Execute(context);
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
    REQUIRE(output.str().find(Resource::String::PackageSelectionPrompt(2).get()) != std::string::npos);
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
    REQUIRE(output.str().find(Resource::String::PackageSelectionPrompt(2).get()) == std::string::npos);
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
