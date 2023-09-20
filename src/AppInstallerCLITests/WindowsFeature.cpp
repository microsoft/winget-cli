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
using namespace AppInstaller::Utility;
using namespace AppInstaller::WindowsFeature;
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
    testSettings.Set<Setting::EFWindowsFeature>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideOpenDependencySource(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_WindowsFeatures.yaml").GetPath().u8string());

    auto mockDismHelperOverride = TestHook::MockDismHelper_Override();
    auto doesFeatureExistOverride = TestHook::SetDoesWindowsFeatureExistResult_Override(false);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_INSTALL_MISSING_DEPENDENCY);
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
    testSettings.Set<Setting::EFWindowsFeature>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideOpenDependencySource(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_WindowsFeatures.yaml").GetPath().u8string());

    // Override with arbitrary DISM api error (DISMAPI_E_DISMAPI_NOT_INITIALIZED) and make windows feature discoverable.
    HRESULT dismErrorResult = 0xc0040001;
    auto mockDismHelperOverride = TestHook::MockDismHelper_Override();
    auto doesFeatureExistOverride = TestHook::SetDoesWindowsFeatureExistResult_Override(true);
    auto setIsFeatureEnabledOverride = TestHook::SetIsWindowsFeatureEnabledResult_Override(false);
    auto setEnableFeatureOverride = TestHook::SetEnableWindowsFeatureResult_Override(dismErrorResult);
    auto getDisplayNameOverride = TestHook::SetWindowsFeatureGetDisplayNameResult_Override(LocIndString{ "Test Windows Feature"_liv });
    auto getRestartStatusOverride = TestHook::SetWindowsFeatureGetRestartStatusResult_Override(DismRestartNo);

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
    LocIndString testFeatureDisplayName = LocIndString{ "Test Windows Feature"_liv };
    auto mockDismHelperOverride = TestHook::MockDismHelper_Override();
    auto setEnableFeatureOverride = TestHook::SetEnableWindowsFeatureResult_Override(dismErrorResult);
    auto doesFeatureExistOverride = TestHook::SetDoesWindowsFeatureExistResult_Override(true);
    auto setIsFeatureEnabledOverride = TestHook::SetIsWindowsFeatureEnabledResult_Override(false);
    auto getDisplayNameOverride = TestHook::SetWindowsFeatureGetDisplayNameResult_Override(testFeatureDisplayName);
    auto getRestartStatusOverride = TestHook::SetWindowsFeatureGetRestartStatusResult_Override(DismRestartNo);

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
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::FailedToEnableWindowsFeature(testFeatureDisplayName, LocIndView{ "testFeature1" })).get()) != std::string::npos);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::FailedToEnableWindowsFeature(testFeatureDisplayName, LocIndView{ "testFeature2" })).get()) != std::string::npos);
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
    testSettings.Set<Setting::EFWindowsFeature>(true);

    // Override with reboot required HRESULT.
    auto mockDismHelperOverride = TestHook::MockDismHelper_Override();
    auto setEnableFeatureOverride = TestHook::SetEnableWindowsFeatureResult_Override(ERROR_SUCCESS_REBOOT_REQUIRED);
    auto setIsFeatureEnabledOverride = TestHook::SetIsWindowsFeatureEnabledResult_Override (false);
    auto doesFeatureExistOverride = TestHook::SetDoesWindowsFeatureExistResult_Override(true);
    auto getDisplayNameOverride = TestHook::SetWindowsFeatureGetDisplayNameResult_Override(LocIndString{ "Test Windows Feature"_liv });
    auto getRestartStatusOverride = TestHook::SetWindowsFeatureGetRestartStatusResult_Override(DismRestartRequired);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideOpenDependencySource(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_WindowsFeatures.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_TO_INSTALL);
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
    testSettings.Set<Setting::EFWindowsFeature>(true);

    // Override with reboot required HRESULT.
    auto mockDismHelperOverride = TestHook::MockDismHelper_Override();
    auto setEnableFeatureOverride = TestHook::SetEnableWindowsFeatureResult_Override(ERROR_SUCCESS_REBOOT_REQUIRED);
    auto setIsFeatureEnabledOverride = TestHook::SetIsWindowsFeatureEnabledResult_Override(false);
    auto doesFeatureExistOverride = TestHook::SetDoesWindowsFeatureExistResult_Override(true);
    auto getDisplayNameOverride = TestHook::SetWindowsFeatureGetDisplayNameResult_Override(LocIndString{ "Test Windows Feature"_liv });
    auto getRestartStatusOverride = TestHook::SetWindowsFeatureGetRestartStatusResult_Override(DismRestartRequired);

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