// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionContext.h"
#include "COMContext.h"
#include "Argument.h"
#include "winget/UserSettings.h"

namespace AppInstaller::CLI::Execution
{
    using namespace Settings;

    namespace
    {
        // Type to contain the CTRL signal and window messages handler.
        struct SignalTerminationHandler
        {
            static SignalTerminationHandler& Instance()
            {
                static SignalTerminationHandler s_instance;
                return s_instance;
            }

            void AddContext(Context* context)
            {
                std::lock_guard<std::mutex> lock{ m_contextsLock };

                auto itr = std::find(m_contexts.begin(), m_contexts.end(), context);
                THROW_HR_IF(E_NOT_VALID_STATE, itr != m_contexts.end());
                m_contexts.push_back(context);
            }

            void RemoveContext(Context* context)
            {
                std::lock_guard<std::mutex> lock{ m_contextsLock };

                auto itr = std::find(m_contexts.begin(), m_contexts.end(), context);
                THROW_HR_IF(E_NOT_VALID_STATE, itr == m_contexts.end());
                m_contexts.erase(itr);
            }

            void StartAppShutdown()
            {
                // Lifetime manager sends CTRL-C after the WM_QUERYENDSESSION is processed.
                // If we disable the CTRL-C handler, the default handler will kill us.
                TerminateContexts(CancelReason::AppShutdown, true);

#ifndef AICLI_DISABLE_TEST_HOOKS
                m_appShutdownEvent.SetEvent();
#endif
            }

#ifndef AICLI_DISABLE_TEST_HOOKS
            HWND GetWindowHandle() { return m_windowHandle.get(); }

            bool WaitForAppShutdownEvent()
            {
                return m_appShutdownEvent.wait(60000);
            }
#endif

        private:
            SignalTerminationHandler()
            {
                // Create message only window.
                m_messageQueueReady.create();
                m_windowThread = std::thread(&SignalTerminationHandler::CreateWindowAndStartMessageLoop, this);
                if (!m_messageQueueReady.wait(100))
                {
                    AICLI_LOG(CLI, Warning, << "Timeout creating winget window");
                }

                // Set up ctrl-c handler.
                LOG_IF_WIN32_BOOL_FALSE(SetConsoleCtrlHandler(StaticCtrlHandlerFunction, TRUE));

#ifndef AICLI_DISABLE_TEST_HOOKS
                m_appShutdownEvent.create();
#endif
            }

            ~SignalTerminationHandler()
            {
                // At this point the thread is gone, but it will get angry
                // if there's no call to join.
                if (m_windowThread.joinable())
                {
                    m_windowThread.join();
                }
            }

            static BOOL WINAPI StaticCtrlHandlerFunction(DWORD ctrlType)
            {
                return Instance().CtrlHandlerFunction(ctrlType);
            }

            static LRESULT WINAPI WindowMessageProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
            {
                AICLI_LOG(CLI, Verbose, << "Received window message type: " << uMsg);
                switch (uMsg)
                {
                case WM_QUERYENDSESSION:
                    SignalTerminationHandler::Instance().StartAppShutdown();
                    return TRUE;
                case WM_ENDSESSION:
                case WM_CLOSE:
                    DestroyWindow(hWnd);
                    break;
                case WM_DESTROY:
                    PostQuitMessage(0);
                    break;
                default:
                    return DefWindowProc(hWnd, uMsg, wParam, lParam);
                }
                return FALSE;
            }

            BOOL CtrlHandlerFunction(DWORD ctrlType)
            {
                // TODO: Move this to be logged per active context when we have thread static globals
                AICLI_LOG(CLI, Info, << "Got CTRL type: " << ctrlType);

                switch (ctrlType)
                {
                case CTRL_C_EVENT:
                case CTRL_BREAK_EVENT:
                    return TerminateContexts(CancelReason::CtrlCSignal, false);
                    // According to MSDN, we should never receive these due to having gdi32/user32 loaded in our process.
                    // But handle them as a force terminate anyway.
                case CTRL_CLOSE_EVENT:
                case CTRL_LOGOFF_EVENT:
                case CTRL_SHUTDOWN_EVENT:
                    return TerminateContexts(CancelReason::CtrlCSignal, true);
                default:
                    return FALSE;
                }
            }

            // Terminates the currently attached contexts.
            // Returns FALSE if no contexts attached; TRUE otherwise.
            BOOL TerminateContexts(CancelReason reason, bool force)
            {
                if (m_contexts.empty())
                {
                    return FALSE;
                }

                {
                    std::lock_guard<std::mutex> lock{ m_contextsLock };
                    for (auto& context : m_contexts)
                    {
                        context->Cancel(reason, force);
                    }
                }

                return TRUE;
            }

            void CreateWindowAndStartMessageLoop()
            {
                PCWSTR windowClass = L"wingetWindow";
                HINSTANCE hInstance = GetModuleHandle(NULL);
                if (hInstance == NULL)
                {
                    LOG_LAST_ERROR_MSG("Failed getting module handle");
                    return;
                }

                WNDCLASSEX wcex = {};
                wcex.cbSize = sizeof(wcex);

                wcex.style = CS_NOCLOSE;
                wcex.lpfnWndProc = SignalTerminationHandler::WindowMessageProcedure;
                wcex.cbClsExtra = 0;
                wcex.cbWndExtra = 0;
                wcex.hInstance = hInstance;
                wcex.lpszClassName = windowClass;

                if (!RegisterClassEx(&wcex))
                {
                    LOG_LAST_ERROR_MSG("Failed registering window class");
                    return;
                }

                m_windowHandle = wil::unique_hwnd(CreateWindow(
                    windowClass,
                    L"WingetMessageOnlyWindow",
                    WS_OVERLAPPEDWINDOW,
                    0, /* x */
                    0, /* y */
                    0, /* nWidth */
                    0, /* nHeight */
                    NULL, /* hWndParent */
                    NULL, /* hMenu */
                    hInstance,
                    NULL)); /* lpParam */

                if (m_windowHandle == nullptr)
                {
                    LOG_LAST_ERROR_MSG("Failed creating window");
                    return;
                }

                ShowWindow(m_windowHandle.get(), SW_HIDE);

                // Force message queue to be created.
                MSG msg;
                PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
                m_messageQueueReady.SetEvent();

                // Message loop
                BOOL getMessageResult;
                while ((getMessageResult = GetMessage(&msg, m_windowHandle.get(), 0, 0)) != 0)
                {
                    if (getMessageResult == -1)
                    {
                        LOG_LAST_ERROR();
                    }
                    else
                    {
                        DispatchMessage(&msg);
                    }
                }
            }

#ifndef AICLI_DISABLE_TEST_HOOKS
            wil::unique_event m_appShutdownEvent;
#endif

            std::mutex m_contextsLock;
            std::vector<Context*> m_contexts;
            wil::unique_event m_messageQueueReady;
            wil::unique_hwnd m_windowHandle;
            std::thread m_windowThread;
        };

        void SetSignalTerminationHandlerContext(bool add, Context* context)
        {
            THROW_HR_IF(E_POINTER, context == nullptr);

            if (add)
            {
                SignalTerminationHandler::Instance().AddContext(context);
            }
            else
            {
                SignalTerminationHandler::Instance().RemoveContext(context);
            }
        }
    }

    Context::~Context()
    {
        if (m_disableSignalTerminationHandlerOnExit)
        {
            EnableSignalTerminationHandler(false);
        }
    }

    std::unique_ptr<Context> Context::CreateSubContext()
    {
        auto clone = std::make_unique<Context>(Reporter, m_threadGlobals);
        clone->m_flags = m_flags;
        clone->m_executingCommand = m_executingCommand;
        // If the parent is hooked up to the CTRL signal, have the clone be as well
        if (m_disableSignalTerminationHandlerOnExit)
        {
            clone->EnableSignalTerminationHandler();
        }
        CopyArgsToSubContext(clone.get());
        return clone;
    }

    void Context::CopyArgsToSubContext(Context* subContext)
    {
        auto argProperties = ArgumentCommon::GetFromExecArgs(Args);
        for (const auto& arg : argProperties)
        {
            if (WI_IsFlagSet(arg.TypeCategory, ArgTypeCategory::CopyFlagToSubContext))
            {
                subContext->Args.AddArg(arg.Type);
            }
            else if (WI_IsFlagSet(arg.TypeCategory, ArgTypeCategory::CopyValueToSubContext))
            {
                subContext->Args.AddArg(arg.Type, Args.GetArg(arg.Type));
            }
        }
    }

    void Context::EnableSignalTerminationHandler(bool enabled)
    {
        SetSignalTerminationHandlerContext(enabled, this);
        m_disableSignalTerminationHandlerOnExit = enabled;
    }

    void Context::UpdateForArgs()
    {
        if (Args.Contains(Args::Type::NoVT))
        {
            Reporter.SetStyle(VisualStyle::NoVT);
        }
        else if (Args.Contains(Args::Type::RetroStyle))
        {
            Reporter.SetStyle(VisualStyle::Retro);
        }
        else if (Args.Contains(Args::Type::RainbowStyle))
        {
            Reporter.SetStyle(VisualStyle::Rainbow);
        }
        else
        {
            Reporter.SetStyle(User().Get<Setting::ProgressBarVisualStyle>());
        }
    }

    void Context::Terminate(HRESULT hr, std::string_view file, size_t line)
    {
        if (hr == APPINSTALLER_CLI_ERROR_CTRL_SIGNAL_RECEIVED)
        {
            ++m_CtrlSignalCount;
            // Use a more recognizable error
            hr = E_ABORT;

            // If things aren't terminating fast enough for the user, they will probably press CTRL+C again.
            // In that case, we should forcibly terminate.
            // Unless we want to spin a separate thread for all work, we have to just exit here.
            if (m_CtrlSignalCount >= 2)
            {
                Reporter.CloseOutputStream(true);
                Logging::Telemetry().LogCommandTermination(hr, file, line);
                std::exit(hr);
            }
        }
        else if (hr == APPINSTALLER_CLI_ERROR_APPTERMINATION_RECEIVED)
        {
            AICLI_LOG(CLI, Info, << "Got app termination signal");
            hr = E_ABORT;
        }

        Logging::Telemetry().LogCommandTermination(hr, file, line);

        if (!m_isTerminated)
        {
            SetTerminationHR(hr);
        }
    }

    void Context::SetTerminationHR(HRESULT hr)
    {
        m_terminationHR = hr;
        m_isTerminated = true;
    }

    void Context::Cancel(CancelReason reason, bool bypassUser)
    {
        HRESULT hr = E_ABORT;
        if (reason == CancelReason::CtrlCSignal)
        {
            hr = APPINSTALLER_CLI_ERROR_CTRL_SIGNAL_RECEIVED;
        }
        else if (reason == CancelReason::AppShutdown)
        {
            hr = APPINSTALLER_CLI_ERROR_APPTERMINATION_RECEIVED;
        }

        Terminate(hr);
        Reporter.CancelInProgressTask(bypassUser, reason);
    }

    void Context::SetExecutionStage(Workflow::ExecutionStage stage)
    {
        if (m_executionStage == stage)
        {
            return;
        }
        else if (m_executionStage > stage)
        {
            THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), "Reporting ExecutionStage to an earlier Stage without allowBackward as true");
        }

        m_executionStage = stage;
        GetThreadGlobals().GetTelemetryLogger().SetExecutionStage(static_cast<uint32_t>(m_executionStage));
    }

    AppInstaller::ThreadLocalStorage::WingetThreadGlobals& Context::GetThreadGlobals()
    {
        return m_threadGlobals;
    }

    std::unique_ptr<AppInstaller::ThreadLocalStorage::PreviousThreadGlobals> Context::SetForCurrentThread()
    {
        return m_threadGlobals.SetForCurrentThread();
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    bool Context::ShouldExecuteWorkflowTask(const Workflow::WorkflowTask& task)
    {
        return (m_shouldExecuteWorkflowTask ? m_shouldExecuteWorkflowTask(task) : true);
    }

    HWND GetWindowHandle()
    {
        return SignalTerminationHandler::Instance().GetWindowHandle();
    }

    bool WaitForAppShutdownEvent()
    {
        return SignalTerminationHandler::Instance().WaitForAppShutdownEvent();
    }
#endif
}
