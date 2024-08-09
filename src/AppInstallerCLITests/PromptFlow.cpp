// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include <Commands/InstallCommand.h>
#include <Workflows/PromptFlow.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Settings;

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
