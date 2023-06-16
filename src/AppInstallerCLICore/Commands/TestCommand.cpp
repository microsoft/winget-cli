// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#ifndef AICLI_DISABLE_TEST_HOOKS

#include "TestCommand.h"

namespace AppInstaller::CLI
{
    std::vector<std::unique_ptr<Command>> TestCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<TestAppShutdownCommand>(FullName()),
            });
    }

    void TestCommand::ExecuteInternal(Execution::Context& context) const
    {
        UNREFERENCED_PARAMETER(context);
        Sleep(INFINITE);
    }

    void TestAppShutdownCommand::ExecuteInternal(Execution::Context& context) const
    {
        auto windowHandle = context.GetWindowHandle();

        if (windowHandle == NULL)
        {
            context.Reporter.Info() << "Window was not created" << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR);
        }

        context.Reporter.Info() << "Waiting for app shutdown event" << std::endl;
        bool result = context.WaitForAppShutdownEvent();
        if (!result)
        {
            context.Reporter.Info() << "Failed getting app shutdown event" << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR);
        }

        context.Reporter.Info() << "Succeeded waiting for app shutdown event" << std::endl;
    }
}

#endif
