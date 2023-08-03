// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include <Commands/InstallCommand.h>
#include <Commands/ResumeCommand.h>
#include <Microsoft/CheckpointIndex.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerVersions.h>

using namespace AppInstaller::CLI;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Settings;
using namespace TestCommon;

TEST_CASE("Resume_VerifyIndexCreatedForTerminatedWorkflow", "[Resume]")
{
    // TODO: Update test when only certain termination HRs persist the checkpoint index.

    TestCommon::TempDirectory tempCheckpointIndexDirectory("TempCheckpointIndexDirectory", false);
    const auto& tempCheckpointIndexDirectoryPath = tempCheckpointIndexDirectory.GetPath();
    TestHook::SetCheckpointIndexDirectory_Override checkpointIndexDirectoryOverride(tempCheckpointIndexDirectoryPath);

    TestCommon::TempDirectory tempDirectory("TempDirectory", false);

    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFResume>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();

    const auto& testManifestPath = TestDataFile("InstallFlowTest_UnsupportedArguments.yaml").GetPath().u8string();
    context.Args.AddArg(Execution::Args::Type::Manifest, testManifestPath);
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);

    InstallCommand install({});
    context.SetExecutingCommand(&install);
    install.Execute(context);

    // There should only be one file in the temp directory.
    std::filesystem::path checkpointIndexPath;
    for (const auto& entry : std::filesystem::directory_iterator(tempCheckpointIndexDirectoryPath))
    {
        checkpointIndexPath = entry.path();
    }

    REQUIRE(std::filesystem::exists(checkpointIndexPath));

    {
        // Open up the index and verify the values.
        CheckpointIndex index = CheckpointIndex::Open(checkpointIndexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
        int contextId = index.GetFirstContextId();
        REQUIRE(contextId == 0);
        REQUIRE(index.GetCommandName(contextId) == "install"sv);
        REQUIRE(index.GetStringArgument(contextId, "manifest"sv) == testManifestPath);
        REQUIRE(index.GetStringArgument(contextId, "location"sv) == tempDirectory.GetPath().u8string());
    }
}

TEST_CASE("Resume_InstallFlowSucceeded", "[Resume]")
{
    GUID testGuid;
    CoCreateGuid(&testGuid);

    std::filesystem::path testIndexPath = CheckpointIndex::GetCheckpointIndexPath(testGuid);

    // Create checkpoint index that succeeds. Verify that checkpoint index is cleaned up.
    {
        CheckpointIndex checkpointIndex = CheckpointIndex::CreateNew(testIndexPath.u8string());

    }

}
