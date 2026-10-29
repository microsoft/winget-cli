// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestHooks.h"
#include "WorkflowCommon.h"
#include <COMContext.h>

using namespace AppInstaller::CLI;
using namespace TestCommon;
using namespace std::string_view_literals;

TEST_CASE("WorkflowBase_EnsureNotRunningWithNonDefaultFullToken", "[WorkflowBase]")
{
    const bool fullToken = GENERATE(false, true);
    const bool comContext = GENERATE(false, true);
    CAPTURE(fullToken, comContext);
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride{ fullToken };

    std::ostringstream output;
    std::unique_ptr<Execution::Context> context;
    if (comContext)
    {
        context = std::make_unique<Execution::COMContext>();
    }
    else
    {
        context = std::make_unique<Execution::Context>(output, std::cin);
    }

    *context <<
        Workflow::EnsureNotRunningWithNonDefaultFullToken <<
        [](Execution::Context& nextContext)
        {
            nextContext.Add<Execution::Data::OperationReturnCode>(0);
        };

    if (fullToken)
    {
        REQUIRE_TERMINATED_WITH((*context), APPINSTALLER_CLI_ERROR_ADMIN_CONTEXT_ACTION_PROHIBITED);
        REQUIRE_FALSE(context->Contains(Execution::Data::OperationReturnCode));
        if (!comContext)
        {
            REQUIRE(output.str().find("non-administrator context") != std::string::npos);
        }
    }
    else
    {
        REQUIRE_FALSE(context->IsTerminated());
        REQUIRE(context->Contains(Execution::Data::OperationReturnCode));
        REQUIRE(output.str().empty());
    }
}

TEST_CASE("WorkflowBase_EnsureSupportForInstalledPackageOperation", "[WorkflowBase]")
{
    const bool fullToken = GENERATE(false, true);
    const auto installedType = GENERATE("exe", "msi", "wix", "portable", "msix", "msstore", "unknown");
    const auto installedScope = GENERATE("user", "machine");
    CAPTURE(fullToken, installedType, installedScope);
    TestHook::SetIsRunningWithNonDefaultFullToken_Override tokenOverride{ fullToken };

    std::ostringstream output;
    Execution::Context context{ output, std::cin };
    context.Add<Execution::Data::InstalledPackageVersion>(std::make_shared<TestPackageVersion>(
        AppInstaller::Manifest::Manifest{},
        TestPackageVersion::MetadataMap{
            { AppInstaller::Repository::PackageVersionMetadata::InstalledType, installedType },
            { AppInstaller::Repository::PackageVersionMetadata::InstalledScope, installedScope },
        }));
    context.Args.AddArg(Execution::Args::Type::InstallScope,
        std::string_view{ installedScope } == "user" ? "machine"sv : "user"sv);

    context << Workflow::EnsureSupportForInstalledPackageOperation;

    const bool blocked = fullToken && std::string_view{ installedScope } == "user" &&
        std::string_view{ installedType } != "msix" && std::string_view{ installedType } != "msstore";
    if (blocked)
    {
        REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_ADMIN_CONTEXT_ACTION_PROHIBITED);
    }
    else
    {
        REQUIRE_FALSE(context.IsTerminated());
        REQUIRE(output.str().empty());
    }
}
