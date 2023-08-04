// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include <Commands/InstallCommand.h>
#include <Commands/ResumeCommand.h>
#include <Microsoft/CheckpointIndex.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>
#include <CheckpointManager.h>

using namespace std::string_literals;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Settings;
using namespace TestCommon;

namespace
{
    // IMPORTANT:
    // Since checkpoint manager is a static singleton class, the deconstructor is not called until after the tests are done.
    // This will cause issues with the teardown process, as the temp directory will try to remove itself while the checkpoint manager still holds
    // the handle to the index file. To resolve this, clean up for the Checkpoint Manager has been disabled and any tests that involve an index must call
    // 'Checkpoint::CheckpointManager::Instance().ManualReset()'.
    auto mockCheckpointManagerCleanUp = TestHook::MockCheckpointManagerCleanUp_Override();
}

TEST_CASE("ResumeFlow_InvalidGuid", "[Resume]")
{
    std::ostringstream resumeOutput;
    TestContext context{ resumeOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();

    context.Args.AddArg(Execution::Args::Type::ResumeGuid, "badGuid"sv);

    ResumeCommand resume({});
    context.SetExecutingCommand(&resume);
    resume.Execute(context);
    INFO(resumeOutput.str());

    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_INVALID_RESUME_GUID);
    auto expectedMessage = Resource::String::InvalidResumeGuidError(AppInstaller::Utility::LocIndString{ "badGuid"s });
    REQUIRE(resumeOutput.str().find(Resource::LocString(expectedMessage).get()) != std::string::npos);
}

TEST_CASE("ResumeFlow_IndexNotFound", "[Resume]")
{
    std::string tempGuidString = "{ec3a098c-a815-4d52-8866-946c03093a37}";

    std::ostringstream resumeOutput;
    TestContext context{ resumeOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ResumeGuid, tempGuidString);

    ResumeCommand resume({});
    context.SetExecutingCommand(&resume);
    resume.Execute(context);
    INFO(resumeOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_RESUME_GUID_NOT_FOUND);
    REQUIRE(resumeOutput.str().find(Resource::LocString(Resource::String::ResumeGuidNotFoundError).get()) != std::string::npos);
}

TEST_CASE("ResumeFlow_InvalidClientVersion", "[Resume]")
{
    TestCommon::TempDirectory tempCheckpointIndexDirectory("TempCheckpointIndexDirectory", true);

    const auto& tempCheckpointIndexDirectoryPath = tempCheckpointIndexDirectory.GetPath();
    TestHook::SetCheckpointIndexDirectory_Override checkpointIndexDirectoryOverride(tempCheckpointIndexDirectoryPath);

    // Create temp guid and populate with invalid client version.
    std::string tempGuidString = "{b157d11f-4487-4e03-9447-9f9d50d66d8e}";
    std::string tempFileName = tempGuidString + ".db";
    auto tempIndexPath = tempCheckpointIndexDirectoryPath / tempFileName;

    INFO("Using temporary file named: " << tempIndexPath);

    {
        CheckpointIndex index = CheckpointIndex::CreateNew(tempIndexPath.u8string());
        index.SetClientVersion("1.2.3.4");
    }

    std::ostringstream resumeOutput;
    TestContext context{ resumeOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ResumeGuid, tempGuidString);

    ResumeCommand resume({});
    context.SetExecutingCommand(&resume);
    resume.Execute(context);
    INFO(resumeOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_CLIENTVERSION_MISMATCH);
    REQUIRE(resumeOutput.str().find(Resource::LocString(Resource::String::ClientVersionMismatchError).get()) != std::string::npos);
    
    // Manually reset index to allow for proper clean up. 
    Checkpoint::CheckpointManager::Instance().ManualReset();
}

TEST_CASE("ResumeFlow_EmptyCheckpointIndex", "Resume")
{
    TestCommon::TempDirectory tempCheckpointIndexDirectory("TempCheckpointIndexDirectory", false);

    const auto& tempCheckpointIndexDirectoryPath = tempCheckpointIndexDirectory.GetPath();
    TestHook::SetCheckpointIndexDirectory_Override checkpointIndexDirectoryOverride(tempCheckpointIndexDirectoryPath);

    // Create temp guid and populate with invalid client version.
    std::string tempGuidString = "{43ca664c-3eae-4f73-99ee-18cf83912c02}";
    std::string tempFileName = tempGuidString + ".db";
    auto tempIndexPath = tempCheckpointIndexDirectoryPath / tempFileName;

    INFO("Using temporary file named: " << tempIndexPath);

    {
        CheckpointIndex index = CheckpointIndex::CreateNew(tempGuidString);
        index.SetClientVersion(AppInstaller::Runtime::GetClientVersion());
    }

    std::ostringstream resumeOutput;
    TestContext context{ resumeOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ResumeGuid, tempGuidString);

    ResumeCommand resume({});
    context.SetExecutingCommand(&resume);
    resume.Execute(context);
    INFO(resumeOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_INVALID_RESUME_STATE);
    REQUIRE(resumeOutput.str().find(Resource::LocString(Resource::String::ResumeStateDataNotFoundError).get()) != std::string::npos);

    // Manually reset index to allow for proper clean up. 
    Checkpoint::CheckpointManager::Instance().ManualReset();
}

TEST_CASE("ResumeFlow_VerifyStateRemovedForSuccess", "[Resume]")
{
    TestCommon::TempDirectory tempCheckpointIndexDirectory("TempCheckpointIndexDirectory", false);

    const auto& tempCheckpointIndexDirectoryPath = tempCheckpointIndexDirectory.GetPath();
    TestHook::SetCheckpointIndexDirectory_Override checkpointIndexDirectoryOverride(tempCheckpointIndexDirectoryPath);

    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFResume>(true);

    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);

    const auto& testManifestPath = TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string();
    context.Args.AddArg(Execution::Args::Type::Manifest, testManifestPath);

    InstallCommand install({});
    context.SetExecutingCommand(&install);
    install.Execute(context);

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);

    // Only one checkpoint file should be created.
    std::vector<std::filesystem::path> checkpointFiles;
    for (const auto& entry : std::filesystem::directory_iterator(tempCheckpointIndexDirectoryPath))
    {
        checkpointFiles.emplace_back(entry.path());
    }

    REQUIRE(checkpointFiles.size() == 1);

    std::filesystem::path checkpointIndexPath = checkpointFiles[0];
    REQUIRE(std::filesystem::exists(checkpointIndexPath));

    {
        CheckpointIndex index = CheckpointIndex::Open(checkpointIndexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
        REQUIRE(index.IsEmpty());
    }

    // Manually reset index to allow for proper index clean up. 
    Checkpoint::CheckpointManager::Instance().ManualReset();
}

TEST_CASE("ResumeFlow_VerifyStateSavedForFailure", "[Resume]")
{
    TestCommon::TempDirectory tempCheckpointIndexDirectory("TempCheckpointIndexDirectory", false);

    const auto& tempCheckpointIndexDirectoryPath = tempCheckpointIndexDirectory.GetPath();
    TestHook::SetCheckpointIndexDirectory_Override checkpointIndexDirectoryOverride(tempCheckpointIndexDirectoryPath);

    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFResume>(true);

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

    // Only one checkpoint file should be created.
    std::vector<std::filesystem::path> checkpointFiles;
    for (const auto& entry : std::filesystem::directory_iterator(tempCheckpointIndexDirectoryPath))
    {
        checkpointFiles.emplace_back(entry.path());
    }

    REQUIRE(checkpointFiles.size() == 1);

    std::filesystem::path checkpointIndexPath = checkpointFiles[0];
    REQUIRE(std::filesystem::exists(checkpointIndexPath));

    {
        CheckpointIndex index = CheckpointIndex::Open(checkpointIndexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
        REQUIRE_FALSE(index.IsEmpty());
        int contextId = index.GetFirstContextId();
        REQUIRE(index.GetStringArgument(contextId, "manifest"sv) == testManifestPath);
        REQUIRE(index.GetStringArgument(contextId, "location"sv) == "installLocation"sv);
    }

    // Manually reset index to allow for proper index clean up. 
    Checkpoint::CheckpointManager::Instance().ManualReset();
}
