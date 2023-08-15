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

TEST_CASE("ResumeFlow_InvalidGuid", "[Resume]")
{
    std::ostringstream resumeOutput;
    TestContext context{ resumeOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();

    context.Args.AddArg(Execution::Args::Type::ResumeId, "badGuid"sv);

    ResumeCommand resume({});
    context.SetExecutingCommand(&resume);
    resume.Execute(context);
    INFO(resumeOutput.str());

    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_INVALID_RESUME_ID);
    auto expectedMessage = Resource::String::InvalidResumeIdError(AppInstaller::Utility::LocIndString{ "badGuid"s });
    REQUIRE(resumeOutput.str().find(Resource::LocString(expectedMessage).get()) != std::string::npos);
}

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
    TestCommon::TempDirectory tempCheckpointIndexDirectory("TempCheckpointIndexDirectory", true);

    const auto& tempCheckpointIndexDirectoryPath = tempCheckpointIndexDirectory.GetPath();
    TestHook::SetCheckpointIndexDirectory_Override checkpointIndexDirectoryOverride(tempCheckpointIndexDirectoryPath);

    // Create temp guid and populate with invalid client version.
    std::string tempGuidString = "{b157d11f-4487-4e03-9447-9f9d50d66d8e}";
    std::string tempFileName = tempGuidString + ".db";
    auto tempIndexPath = tempCheckpointIndexDirectoryPath / tempFileName;
    std::string_view invalidClientVersion = "1.2.3.4"sv;

    INFO("Using temporary file named: " << tempIndexPath);

    {
        CheckpointIndex index = CheckpointIndex::CreateNew(tempIndexPath.u8string());
        index.SetClientVersion(invalidClientVersion);
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

TEST_CASE("ResumeFlow_EmptyIndex", "Resume")
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
    TestCommon::TempDirectory tempCheckpointIndexDirectory("TempCheckpointIndexDirectory", false);

    const auto& tempCheckpointIndexDirectoryPath = tempCheckpointIndexDirectory.GetPath();
    TestHook::SetCheckpointIndexDirectory_Override checkpointIndexDirectoryOverride(tempCheckpointIndexDirectoryPath);

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
    for (const auto& entry : std::filesystem::directory_iterator(tempCheckpointIndexDirectoryPath))
    {
        checkpointFiles.emplace_back(entry.path());
    }

    REQUIRE(checkpointFiles.size() == 0);
}

// TODO: This test will need to be updated once saving the resume state is restricted to certain HRs.
TEST_CASE("ResumeFlow_InstallFailure", "[Resume]")
{
    TestCommon::TempDirectory tempCheckpointIndexDirectory("TempCheckpointIndexDirectory", false);

    const auto& tempCheckpointIndexDirectoryPath = tempCheckpointIndexDirectory.GetPath();
    TestHook::SetCheckpointIndexDirectory_Override checkpointIndexDirectoryOverride(tempCheckpointIndexDirectoryPath);

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

    // Only one checkpoint file should be created.
    std::vector<std::filesystem::path> checkpointFiles;
    for (const auto& entry : std::filesystem::directory_iterator(tempCheckpointIndexDirectoryPath))
    {
        checkpointFiles.emplace_back(entry.path());
    }

    REQUIRE(checkpointFiles.size() == 1);

    std::filesystem::path checkpointIndexPath = checkpointFiles[0];
    REQUIRE(std::filesystem::exists(checkpointIndexPath));
}
