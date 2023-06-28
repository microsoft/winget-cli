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

    Resource::LocString TestCommand::ShortDescription() const
    {
        return Utility::LocIndString("Waits infinitely"sv);
    }

    Resource::LocString TestCommand::LongDescription() const
    {
        return Utility::LocIndString("Waits infinitely. Use this if you want winget to wait forever while something is going on"sv);
    }

    void TestAppShutdownCommand::ExecuteInternal(Execution::Context& context) const
    {
        auto windowHandle = context.GetWindowHandle();

        if (windowHandle == NULL)
        {
            context.Reporter.Info() << "Window was not created" << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR);
        }

        AICLI_LOG(CLI, Info, << "Waiting for app shutdown event");
        context.Reporter.Info() << "Waiting for app shutdown event" << std::endl;
        bool result = context.WaitForAppShutdownEvent();
        if (!result)
        {
            AICLI_LOG(CLI, Info, << "Failed getting app shutdown event");
            context.Reporter.Info() << "Failed getting app shutdown event" << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR);
        }

        AICLI_LOG(CLI, Info, << "Succeeded waiting for app shutdown event");
        context.Reporter.Info() << "Succeeded waiting for app shutdown event" << std::endl;
    }

    Resource::LocString TestAppShutdownCommand::ShortDescription() const
    {
        return Utility::LocIndString("Test command to verify appshutdown event."sv);
    }

    Resource::LocString TestAppShutdownCommand::LongDescription() const
    {
        return Utility::LocIndString("Test command for appshutdown. Verifies the window was created and waits for the app shutdown event"sv);
    }

}

#endif
