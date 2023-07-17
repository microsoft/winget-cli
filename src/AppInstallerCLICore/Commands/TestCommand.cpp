// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#ifndef AICLI_DISABLE_TEST_HOOKS

#include "TestCommand.h"

namespace AppInstaller::CLI
{
    namespace
    {
        void LogAndReport(Execution::Context& context, const std::string& message)
        {
            AICLI_LOG(CLI, Info, << message);
            context.Reporter.Info() << message << std::endl;
        }
    }

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

    std::vector<Argument> TestAppShutdownCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::Force)
        };
    }

    void TestAppShutdownCommand::ExecuteInternal(Execution::Context& context) const
    {
        auto windowHandle = Execution::GetWindowHandle();

        if (windowHandle == NULL)
        {
            LogAndReport(context, "Window was not created");
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR);
        }

        if (context.Args.Contains(Execution::Args::Type::Force))
        {
            LogAndReport(context, "Sending WM_QUERYENDSESSION message");
            THROW_LAST_ERROR_IF(!SendMessageTimeout(
                windowHandle,
                WM_QUERYENDSESSION,
                NULL,
                ENDSESSION_CLOSEAPP,
                (SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT),
                5000,
                NULL));
        }

        LogAndReport(context, "Waiting for app shutdown event");
        bool result = Execution::WaitForAppShutdownEvent();
        if (!result)
        {
            LogAndReport(context, "Failed getting app shutdown event");
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR);
        }

        LogAndReport(context, "Succeeded waiting for app shutdown event");
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
