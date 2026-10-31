// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestHooks.h"
#include "TestSource.h"
#include "WorkflowCommon.h"
#include <ExecutionContext.h>
#include <Workflows/RepairFlow.h>

using namespace TestCommon;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Repository;

TEST_CASE("RepairFlow_AdminContextRejectsUserScopeBeforePreparation", "[RepairFlow][workflow]")
{
    const auto installedType = GENERATE("Burn", "Msi");
    CAPTURE(installedType);
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride(true);

    std::ostringstream output;
    TestContext context{ output, std::cin };
    context.Add<Data::InstalledPackageVersion>(TestPackageVersion::Make(
        AppInstaller::Manifest::Manifest{},
        TestPackageVersion::MetadataMap{
            { PackageVersionMetadata::InstalledType, installedType },
            { PackageVersionMetadata::InstalledScope, "User" },
        }));

    context << AppInstaller::CLI::Workflow::RepairSinglePackage;

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_ADMIN_CONTEXT_ACTION_PROHIBITED);
}
