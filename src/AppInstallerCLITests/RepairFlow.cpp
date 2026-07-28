// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestHooks.h"
#include "TestSource.h"
#include "WorkflowCommon.h"
#include <winget/ManifestYamlParser.h>
#include <ExecutionContext.h>
#include <Workflows/ShellExecuteInstallerHandler.h>

using namespace TestCommon;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;

// Tests that when the process is running with a non-default full (elevated UAC) token,
// repairing a package that was installed at user scope is blocked with
// APPINSTALLER_CLI_ERROR_ADMIN_CONTEXT_ACTION_PROHIBITED.
// This single test covers both the CLI (RepairCommand) and COM (COMRepairCommand) code paths
// because they both invoke the same ShellExecuteRepairImpl workflow step.
TEST_CASE("RepairFlow_AdminContextWithUserScopeInstall", "[RepairFlow][workflow]")
{
    // Simulate running with an elevated (non-default full) token.
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(true);

    std::ostringstream repairOutput;
    TestContext context{ repairOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();

    // Build an installed package version with Burn type installed at user scope.
    // ShellExecuteRepairImpl blocks Burn (and other Exe-family) packages installed at user scope.
    auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
    TestPackageVersion::MetadataMap metadata
    {
        { PackageVersionMetadata::InstalledType, "Burn" },
        { PackageVersionMetadata::InstalledScope, "User" },
    };
    context.Add<Data::InstalledPackageVersion>(TestPackageVersion::Make(manifest, metadata));

    // Provide a dummy repair string; the admin-context check fires before the command is executed.
    context.Add<Data::RepairString>(std::string("C:\\repair.exe /silent"));

    context << AppInstaller::CLI::Workflow::ShellExecuteRepairImpl;

    INFO(repairOutput.str());
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_ADMIN_CONTEXT_ACTION_PROHIBITED);
}
