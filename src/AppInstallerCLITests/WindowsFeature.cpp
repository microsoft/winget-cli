// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "WorkflowCommon.h"
#include <AppInstallerRuntime.h>
#include <Commands/InstallCommand.h>
#include "TestHooks.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;
using namespace TestCommon;

TEST_CASE("InstallFlow_WindowsFeatureDoesNotExist", "[windowsFeature]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideOpenDependencySource(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_WindowsFeatures.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_INSTALL_DEPENDENCIES);
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::WindowsFeatureNotFound(LocIndView{ "testFeature1" })).get()) != std::string::npos);

    // "badFeature" should not be displayed as the flow should terminate after failing to find the first feature.
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::WindowsFeatureNotFound(LocIndView{ "testFeature2" })).get()) == std::string::npos);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::FailedToEnableWindowsFeatureOverrideRequired).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_FailedToEnableWindowsFeature", "[windowsFeature]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideOpenDependencySource(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_WindowsFeatures.yaml").GetPath().u8string());

    auto setDoesFeatureExistOverride = TestHook::SetDoesWindowsFeatureExistResult_Override(ERROR_SUCCESS);
    auto setEnableFeatureOverride = TestHook::SetEnableWindowsFeatureResult_Override(0xc0040001); // DISMAPI_E_DISMAPI_NOT_INITIALIZED

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_INSTALL_DEPENDENCIES);
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::FailedToEnableWindowsFeatureOverrideRequired).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_FailedToEnableWindowsFeature_Force", "[windowsFeature]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;

    auto doesFeatureExistOverride = TestHook::SetDoesWindowsFeatureExistResult_Override(ERROR_SUCCESS);
    auto expectedErrorCode = 0xc0040001; // DISMAPI_E_DISMAPI_NOT_INITIALIZED
    auto setEnableFeatureOverride = TestHook::SetEnableWindowsFeatureResult_Override(expectedErrorCode);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideOpenDependencySource(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_WindowsFeatures.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Force);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(context.GetTerminationHR() == ERROR_SUCCESS);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::FailedToEnableWindowsFeature(LocIndView{ "testFeature1" }, expectedErrorCode)).get()) != std::string::npos);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::FailedToEnableWindowsFeature(LocIndView{ "testFeature2" }, expectedErrorCode)).get()) != std::string::npos);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::FailedToEnableWindowsFeatureOverridden).get()) != std::string::npos);
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("InstallFlow_RebootRequired", "[windowsFeature]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;

    // Override with reboot required HRESULT.
    auto doesFeatureExistOverride = TestHook::SetDoesWindowsFeatureExistResult_Override(ERROR_SUCCESS);
    auto setEnableFeatureOverride = TestHook::SetEnableWindowsFeatureResult_Override(ERROR_SUCCESS_REBOOT_REQUIRED);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideOpenDependencySource(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_WindowsFeatures.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_FOR_INSTALL);
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::RebootRequiredToEnableWindowsFeatureOverrideRequired).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_RebootRequired_Force", "[windowsFeature]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;

    // Override with reboot required HRESULT.
    auto doesFeatureExistOverride = TestHook::SetDoesWindowsFeatureExistResult_Override(ERROR_SUCCESS);
    auto setEnableFeatureOverride = TestHook::SetEnableWindowsFeatureResult_Override(ERROR_SUCCESS_REBOOT_REQUIRED);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideOpenDependencySource(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_WindowsFeatures.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Force);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(context.GetTerminationHR() == ERROR_SUCCESS);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::RebootRequiredToEnableWindowsFeatureOverridden).get()) != std::string::npos);
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}