// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "WorkflowCommon.h"
#include <AppInstallerRuntime.h>
#include <Commands/InstallCommand.h>
#include <winget/WindowsFeature.h>
#include "TestHooks.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Settings;
using namespace AppInstaller::WindowsFeature;
using namespace TestCommon;

// IMPORTANT: These tests require elevation and will modify your Windows Feature settings. 

const std::string s_featureName = "netfx3";

TEST_CASE("GetWindowsFeature", "[windowsFeature]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    WindowsFeature validFeature{ s_featureName };
    REQUIRE(validFeature.DoesExist());

    WindowsFeature invalidFeature{ "invalidFeature" };
    REQUIRE_FALSE(invalidFeature.DoesExist());
}

TEST_CASE("DisableEnableWindowsFeature", "[windowsFeature]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    WindowsFeature feature{ s_featureName };
    HRESULT disableResult = feature.DisableFeature();
    bool disableStatus = (disableResult == S_OK || disableResult == ERROR_SUCCESS_REBOOT_REQUIRED);
    REQUIRE(disableStatus);
    REQUIRE_FALSE(feature.IsEnabled());

    HRESULT enableResult = feature.EnableFeature();
    bool enableStatus = (enableResult == S_OK) || (enableResult == ERROR_SUCCESS_REBOOT_REQUIRED);
    REQUIRE(enableStatus);
    REQUIRE(feature.IsEnabled());
}

TEST_CASE("InstallFlow_ValidWindowsFeature", "[windowsFeature]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFWindowsFeature>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_WindowsFeatures.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("InstallFlow_InvalidWindowsFeature", "[windowsFeature]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFWindowsFeature>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_InvalidWindowsFeatures.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_INSTALL_MISSING_DEPENDENCY);
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::WindowsFeatureNotFoundOverrideRequired).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("invalidFeature") != std::string::npos);
    REQUIRE(installOutput.str().find("badFeature") != std::string::npos);
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
    testSettings.Set<Setting::EFWindowsFeature>(true);

    // Override with arbitrary DISM api error (DISMAPI_E_DISMAPI_NOT_INITIALIZED) and make windows feature discoverable.
    HRESULT dismErrorResult = 0xc0040001;
    TestHook::SetEnableWindowsFeatureResult_Override enableWindowsFeatureResultOverride(dismErrorResult);
    TestHook::SetIsWindowsFeatureEnabledResult_Override isWindowsFeatureEnabledResultOverride(false);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_InvalidWindowsFeatures.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::IgnoreMissingDependencies);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(context.GetTerminationHR() == dismErrorResult);
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
    testSettings.Set<Setting::EFWindowsFeature>(true);

    // Override with arbitrary DISM api error (DISMAPI_E_DISMAPI_NOT_INITIALIZED) and make windows feature discoverable.
    HRESULT dismErrorResult = 0xc0040001;
    TestHook::SetEnableWindowsFeatureResult_Override enableWindowsFeatureResultOverride(dismErrorResult);
    TestHook::SetIsWindowsFeatureEnabledResult_Override isWindowsFeatureEnabledResultOverride(false);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_InvalidWindowsFeatures.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::IgnoreMissingDependencies);
    context.Args.AddArg(Execution::Args::Type::Force);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(context.GetTerminationHR() == ERROR_SUCCESS);
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
    testSettings.Set<Setting::EFWindowsFeature>(true);

    // Override with reboot required HRESULT.
    TestHook::SetEnableWindowsFeatureResult_Override enableWindowsFeatureResultOverride(ERROR_SUCCESS_REBOOT_REQUIRED);
    TestHook::SetIsWindowsFeatureEnabledResult_Override isWindowsFeatureEnabledResultOverride(false);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_WindowsFeatures.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::RebootRequiredForEnablingWindowsFeature).get()) != std::string::npos);
}
