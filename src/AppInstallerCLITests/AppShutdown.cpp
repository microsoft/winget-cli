// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <winget/Runtime.h>
#include "Commands/TestCommand.h"

using namespace AppInstaller::CLI;

TEST_CASE("AppShutdown_WindowMessage", "[appShutdown]")
{
    if (AppInstaller::Runtime::IsRunningAsAdmin() && AppInstaller::Runtime::IsRunningInPackagedContext())
    {
        WARN("Test can't run as admin in package context");
        return;
    }

    std::ostringstream output;
    Execution::Context context{ output, std::cin };
    context.Args.AddArg(Execution::Args::Type::Force);

    TestAppShutdownCommand appShutdownCmd({});
    appShutdownCmd.Execute(context);

    REQUIRE(context.IsTerminated());
    REQUIRE(S_OK == context.GetTerminationHR());
}
