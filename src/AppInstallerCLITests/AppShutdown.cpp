// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Commands/TestCommand.h"

using namespace AppInstaller::CLI;

TEST_CASE("AppShutdown_WindowMessage", "[appShutdown]")
{
    {
        std::ostringstream output;
        Execution::Context context{ output, std::cin };
        context.Args.AddArg(Execution::Args::Type::Force);

        TestAppShutdownCommand appShutdownCmd({});
        appShutdownCmd.Execute(context);

        REQUIRE(context.IsTerminated());
        REQUIRE(S_OK == context.GetTerminationHR());
    }
}
