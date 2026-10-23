// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestHooks.h"
#include "TestSource.h"
#include "WorkflowCommon.h"
#include <winget/ManifestYamlParser.h>
#include <ExecutionContext.h>
#include <Workflows/ShellExecuteInstallerHandler.h>
#include <Workflows/FontFlow.h>
#include <Workflows/RepairFlow.h>

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

TEST_CASE("RepairFlow_FontUsesInstalledMetadata", "[RepairFlow][workflow]")
{
    const HRESULT repairResult = GENERATE(S_OK, APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_FOUND);
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
    TestPackageVersion::MetadataMap metadata
    {
        { PackageVersionMetadata::InstalledType, "Font" },
        { PackageVersionMetadata::InstalledScope, "User" },
    };
    context.Add<Data::InstalledPackageVersion>(TestPackageVersion::Make(manifest, metadata));
    context.Override({ AppInstaller::CLI::Workflow::FontRepairImpl, [repairResult](TestContext& repairContext)
        {
            repairContext.Add<Data::OperationReturnCode>(static_cast<DWORD>(repairResult));
        }, 1 });

    context <<
        AppInstaller::CLI::Workflow::SelectApplicableInstallerIfNecessary <<
        AppInstaller::CLI::Workflow::RepairSinglePackage;

    REQUIRE_FALSE(context.Contains(Data::Installer));
    INFO(output.str());
    if (SUCCEEDED(repairResult))
    {
        REQUIRE_FALSE(context.IsTerminated());
        REQUIRE(output.str().find("Repair operation completed successfully.") != std::string::npos);
    }
    else
    {
        REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_EXEC_REPAIR_FAILED);
    }
}

TEST_CASE("RepairFlow_FontBlocksElevatedUserRepair", "[RepairFlow][workflow]")
{
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(true);
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
    TestPackageVersion::MetadataMap metadata
    {
        { PackageVersionMetadata::InstalledType, "Font" },
        { PackageVersionMetadata::InstalledScope, "User" },
    };
    context.Add<Data::InstalledPackageVersion>(TestPackageVersion::Make(manifest, metadata));
    context << AppInstaller::CLI::Workflow::FontRepairImpl;
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_ADMIN_CONTEXT_ACTION_PROHIBITED);
}

TEST_CASE("RepairFlow_ArchiveFontNeedsNoExternalRepairInfo", "[RepairFlow][workflow]")
{
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
    manifest.Installers[0].BaseInstallerType = InstallerTypeEnum::Zip;
    manifest.Installers[0].NestedInstallerType = InstallerTypeEnum::Font;
    context.Add<Data::Installer>(manifest.Installers[0]);
    context << AppInstaller::CLI::Workflow::GetRepairInfo;
    REQUIRE_FALSE(context.IsTerminated());
}
