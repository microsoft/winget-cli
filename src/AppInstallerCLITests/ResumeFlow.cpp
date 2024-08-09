// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include <Commands/InstallCommand.h>
#include <Commands/ResumeCommand.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>
#include <CheckpointManager.h>
#include <Workflows/ShellExecuteInstallerHandler.h>

using namespace std::string_literals;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Runtime;
using namespace TestCommon;
using namespace AppInstaller::Checkpoints;

constexpr std::string_view s_AutomaticCheckpoint = "automatic"sv;
constexpr std::string_view s_CheckpointsFileName = "checkpoints.db"sv;

TEST_CASE("ResumeFlow_IndexNotFound", "[Resume]")
{
    std::string tempGuidString = "{ec3a098c-a815-4d52-8866-946c03093a37}";

    std::ostringstream resumeOutput;
    TestContext context{ resumeOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ResumeId, tempGuidString);

    ResumeCommand resume({});
    context.SetExecutingCommand(&resume);
    resume.Execute(context);
    INFO(resumeOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_RESUME_ID_NOT_FOUND);
    auto expectedMessage = Resource::String::ResumeIdNotFoundError(AppInstaller::Utility::LocIndString(tempGuidString));
    REQUIRE(resumeOutput.str().find(Resource::LocString(expectedMessage).get()) != std::string::npos);
}

TEST_CASE("ResumeFlow_InvalidClientVersion", "[Resume]")
{
    TestCommon::TempDirectory tempCheckpointRecordDirectory("TempCheckpointRecordDirectory", true);

    const auto& tempCheckpointRecordDirectoryPath = tempCheckpointRecordDirectory.GetPath();
    TestHook_SetPathOverride(PathName::CheckpointsLocation, tempCheckpointRecordDirectoryPath);

    // Create temp guid and populate with invalid client version.
    std::string tempGuidString = "{615339e9-3ac5-4e86-a5ab-c246657aca25}";
    auto tempRecordPath = tempCheckpointRecordDirectoryPath / tempGuidString / s_CheckpointsFileName;
    std::string_view invalidClientVersion = "1.2.3.4"sv;

    INFO("Using temporary file named: " << tempRecordPath);

    {
        // Manually set invalid client version
        std::filesystem::create_directories(tempRecordPath.parent_path());
        std::shared_ptr<CheckpointDatabase> checkpointRecord = CheckpointDatabase::CreateNew(tempRecordPath.u8string());
        CheckpointDatabase::IdType checkpointId = checkpointRecord->AddCheckpoint(s_AutomaticCheckpoint);
        checkpointRecord->SetDataValue(checkpointId, AutomaticCheckpointData::ClientVersion, {}, { "1.2.3.4" });
    }

    std::ostringstream resumeOutput;
    TestContext context{ resumeOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ResumeId, tempGuidString);

    ResumeCommand resume({});
    context.SetExecutingCommand(&resume);
    resume.Execute(context);
    INFO(resumeOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_CLIENT_VERSION_MISMATCH);
    auto expectedMessage = Resource::String::ClientVersionMismatchError(AppInstaller::Utility::LocIndString(invalidClientVersion));
    REQUIRE(resumeOutput.str().find(Resource::LocString(expectedMessage).get()) != std::string::npos);
}

TEST_CASE("ResumeFlow_EmptyIndex", "[Resume]")
{
    TestCommon::TempDirectory tempCheckpointRecordDirectory("TempCheckpointRecordDirectory", true);

    const auto& tempCheckpointRecordDirectoryPath = tempCheckpointRecordDirectory.GetPath();
    TestHook_SetPathOverride(PathName::CheckpointsLocation, tempCheckpointRecordDirectoryPath);

    std::string tempGuidString = "{43ca664c-3eae-4f73-99ee-18cf83912c02}";
    auto tempRecordPath = tempCheckpointRecordDirectoryPath / tempGuidString / s_CheckpointsFileName;

    INFO("Using temporary file named: " << tempRecordPath);

    {
        std::filesystem::create_directories(tempRecordPath.parent_path());
        CheckpointDatabase::CreateNew(tempRecordPath.u8string());
    }

    std::ostringstream resumeOutput;
    TestContext context{ resumeOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ResumeId, tempGuidString);

    ResumeCommand resume({});
    context.SetExecutingCommand(&resume);
    resume.Execute(context);
    INFO(resumeOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_INVALID_RESUME_STATE);
    REQUIRE(resumeOutput.str().find(Resource::LocString(Resource::String::ResumeStateDataNotFoundError).get()) != std::string::npos);
}

TEST_CASE("ResumeFlow_InstallSuccess", "[Resume]")
{
    TestCommon::TempDirectory tempCheckpointRecordDirectory("TempCheckpointRecordDirectory", false);

    const auto& tempCheckpointRecordDirectoryPath = tempCheckpointRecordDirectory.GetPath();
    TestHook_SetPathOverride(PathName::CheckpointsLocation, tempCheckpointRecordDirectoryPath);

    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFResume>(true);

    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        OverrideForShellExecute(context);

        const auto& testManifestPath = TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string();
        context.Args.AddArg(Execution::Args::Type::Manifest, testManifestPath);

        InstallCommand install({});
        context.SetExecutingCommand(&install);
        install.Execute(context);
        INFO(installOutput.str());
    }
    
    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);

    // The checkpoint index should not exist if the context succeeded.
    std::vector<std::filesystem::path> checkpointFiles;
    for (const auto& entry : std::filesystem::directory_iterator(tempCheckpointRecordDirectoryPath))
    {
        checkpointFiles.emplace_back(entry.path());
    }

    REQUIRE(checkpointFiles.size() == 0);
}

TEST_CASE("ResumeFlow_InstallFailure", "[Resume]")
{
    TestCommon::TempDirectory tempCheckpointRecordDirectory("TempCheckpointRecordDirectory", false);

    const auto& tempCheckpointRecordDirectoryPath = tempCheckpointRecordDirectory.GetPath();
    TestHook_SetPathOverride(PathName::CheckpointsLocation, tempCheckpointRecordDirectoryPath);

    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFResume>(true);

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();

        const auto& testManifestPath = TestDataFile("InstallFlowTest_UnsupportedArguments.yaml").GetPath().u8string();
        context.Args.AddArg(Execution::Args::Type::Manifest, testManifestPath);
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "installLocation"sv);

        InstallCommand install({});
        context.SetExecutingCommand(&install);
        install.Execute(context);
        INFO(installOutput.str());

        // Verify unsupported arguments error message is shown 
        REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UNSUPPORTED_ARGUMENT);
    }

    // Checkpoint file should be cleaned up if the hr is not reboot related.
    REQUIRE(std::filesystem::is_empty(tempCheckpointRecordDirectoryPath));
}


TEST_CASE("ResumeFlow_WriteToRunOnceRegistry", "[Reboot][Resume][windowsFeature]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestCommon::TempDirectory tempCheckpointRecordDirectory("TempCheckpointRecordDirectory", false);

    const auto& tempCheckpointRecordDirectoryPath = tempCheckpointRecordDirectory.GetPath();
    TestHook_SetPathOverride(PathName::CheckpointsLocation, tempCheckpointRecordDirectoryPath);

    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFResume>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideOpenDependencySource(context);
    OverrideRegisterStartupAfterReboot(context);

    // Override with reboot required HRESULT.
    auto doesFeatureExistOverride = TestHook::SetDoesWindowsFeatureExistResult_Override(ERROR_SUCCESS);
    auto setEnableFeatureOverride = TestHook::SetEnableWindowsFeatureResult_Override(ERROR_SUCCESS_REBOOT_REQUIRED);
    TestHook::SetRegisterForRestartResult_Override registerForRestartResultOverride(false);
    TestHook::SetInitiateRebootResult_Override initiateRebootResultOverride(false);

    const auto& testManifestPath = TestDataFile("InstallFlowTest_WindowsFeatures.yaml").GetPath().u8string();
    context.Args.AddArg(Execution::Args::Type::Manifest, testManifestPath);
    context.Args.AddArg(Execution::Args::Type::AllowReboot);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_FOR_INSTALL);
}

TEST_CASE("ResumeFlow_ResumeLimitExceeded", "[Resume]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestCommon::TempDirectory tempCheckpointsLocationDir("TempCheckpointsLocationDir", true);

    const auto& tempCheckpointRecordDirectoryPath = tempCheckpointsLocationDir.GetPath();
    TestHook_SetPathOverride(PathName::CheckpointsLocation, tempCheckpointRecordDirectoryPath);

    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFResume>(true);
    testSettings.Set<Setting::MaxResumes>(1);

    std::filesystem::path testResumeId = L"testResumeId";
    std::filesystem::path tempResumeDir = tempCheckpointRecordDirectoryPath / testResumeId;
    std::filesystem::path tempCheckpointDatabasePath = tempResumeDir / L"checkpoints.db";
    std::filesystem::create_directory(tempResumeDir);

    {
        std::shared_ptr<CheckpointDatabase> database = CheckpointDatabase::CreateNew(tempCheckpointDatabasePath.u8string(), {1, 0});
        std::string_view testCheckpointName = "testCheckpoint"sv;

        CheckpointDatabase::IdType checkpointId = database->AddCheckpoint(testCheckpointName);
        database->SetDataValue(checkpointId, AutomaticCheckpointData::Command, {}, { "install" });
        database->SetDataValue(checkpointId, AutomaticCheckpointData::ClientVersion, {}, { GetClientVersion()});
        database->SetDataValue(checkpointId, AutomaticCheckpointData::ResumeCount, {}, {"1"});
    }

    {
        std::ostringstream resumeOutput;
        TestContext resumeContext{ resumeOutput, std::cin };
        auto previousThreadGlobals = resumeContext.SetForCurrentThread();
        resumeContext.Args.AddArg(Execution::Args::Type::ResumeId, testResumeId.u8string());

        ResumeCommand resume({});
        resume.Execute(resumeContext);
        INFO(resumeOutput.str());
        REQUIRE(resumeContext.IsTerminated());
        REQUIRE(resumeContext.GetTerminationHR() == APPINSTALLER_CLI_ERROR_RESUME_LIMIT_EXCEEDED);

        REQUIRE(resumeOutput.str().find(Resource::LocString(Resource::String::ResumeLimitExceeded('1')).get()) != std::string::npos);
        REQUIRE(resumeOutput.str().find("winget resume -g " + testResumeId.u8string() + " --ignore-resume-limit") != std::string::npos);
    }
}
