// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#ifndef AICLI_DISABLE_TEST_HOOKS

#include "TestCommand.h"
#include "AppInstallerRuntime.h"

namespace AppInstaller::CLI
{
    namespace
    {
        void LogAndReport(Execution::Context& context, const std::string& message)
        {
            AICLI_LOG(CLI, Info, << message);
            context.Reporter.Info() << message << std::endl;
        }

        HRESULT WaitForShutdown(Execution::Context& context)
        {
            LogAndReport(context, "Waiting for app shutdown event");
            if (!Execution::WaitForAppShutdownEvent())
            {
                LogAndReport(context, "Failed getting app shutdown event");
                return APPINSTALLER_CLI_ERROR_INTERNAL_ERROR;
            }

            LogAndReport(context, "Succeeded waiting for app shutdown event");
            return S_OK;
        }

        HRESULT AppShutdownWindowMessage(Execution::Context& context)
        {
            auto windowHandle = Execution::GetWindowHandle();

            if (windowHandle == NULL)
            {
                LogAndReport(context, "Window was not created");
                return APPINSTALLER_CLI_ERROR_INTERNAL_ERROR;
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

            HRESULT hr = WaitForShutdown(context);

            if (context.Args.Contains(Execution::Args::Type::Force))
            {
                LogAndReport(context, "Sending WM_ENDSESSION message");
                THROW_LAST_ERROR_IF(!SendMessageTimeout(
                    windowHandle,
                    WM_ENDSESSION,
                    NULL,
                    ENDSESSION_CLOSEAPP,
                    (SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT),
                    5000,
                    NULL));
            }

            return hr;
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
        HRESULT hr = E_FAIL;

        // Only package context and admin won't create the window message.
        if (!Runtime::IsRunningInPackagedContext() || !Runtime::IsRunningAsAdmin())
        {
            hr = AppShutdownWindowMessage(context);
        }
        else
        {
            hr = WaitForShutdown(context);
        }

        AICLI_TERMINATE_CONTEXT(hr);
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
