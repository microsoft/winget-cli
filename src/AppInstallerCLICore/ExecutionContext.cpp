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
        // This starts getting called by CreateWindowEx.
        // If it's a method in SignalTerminationHandler we will have problems.
        LRESULT WindowMessageProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            AICLI_LOG(CLI, Verbose, << "Received window message type: " << uMsg);
            switch (uMsg)
            {
            case WM_ENDSESSION:
            case WM_QUERYENDSESSION:
            case WM_CLOSE:
                DestroyWindow(hWnd);
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            default:
                return DefWindowProc(hWnd, uMsg, wParam, lParam);
            }
            return 0;
        }

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

        private:
            SignalTerminationHandler()
            {
                // Create message only window.
                wil::unique_event messageQueueReady;
                messageQueueReady.create();
                m_windowThread = std::thread(&SignalTerminationHandler::CreateWindowAndStartMessageLoop, this, std::ref(messageQueueReady));
                messageQueueReady.wait();

                // Set up ctrl-c handler.
                LOG_IF_WIN32_BOOL_FALSE(SetConsoleCtrlHandler(StaticCtrlHandlerFunction, TRUE));
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

            BOOL CtrlHandlerFunction(DWORD ctrlType)
            {
                switch (ctrlType)
                {
                case CTRL_C_EVENT:
                case CTRL_BREAK_EVENT:
                    return TerminateContexts(ctrlType, false);
                    // According to MSDN, we should never receive these due to having gdi32/user32 loaded in our process.
                    // But handle them as a force terminate anyway.
                case CTRL_CLOSE_EVENT:
                case CTRL_LOGOFF_EVENT:
                case CTRL_SHUTDOWN_EVENT:
                    return TerminateContexts(ctrlType, true);
                default:
                    return FALSE;
                }
            }

            // Terminates the currently attached contexts.
            // Returns FALSE if no contexts attached; TRUE otherwise.
            BOOL TerminateContexts(DWORD ctrlType, bool force)
            {
                if (m_contexts.empty())
                {
                    return FALSE;
                }

                {
                    std::lock_guard<std::mutex> lock{ m_contextsLock };

                    // TODO: Move this to be logged per active context when we have thread static globals
                    AICLI_LOG(CLI, Info, << "Got CTRL type: " << ctrlType);

                    for (auto& context : m_contexts)
                    {
                        context->Cancel(true, force);
                    }
                }

                return TRUE;
            }

            void CreateWindowAndStartMessageLoop(wil::unique_event& messageQueueReady)
            {
                PCWSTR windowClass = L"wingetWindow";
                HINSTANCE hInstance = GetModuleHandle(NULL);
                THROW_LAST_ERROR_IF_NULL(hInstance);

                WNDCLASSEX wcex = {};
                wcex.cbSize = sizeof(wcex);
                wcex.style = CS_NOCLOSE;
                wcex.lpfnWndProc = WindowMessageProcedure;
                wcex.cbClsExtra = 0;
                wcex.cbWndExtra = 0;
                wcex.hInstance = hInstance;
                wcex.lpszClassName = windowClass;
                THROW_LAST_ERROR_IF(!RegisterClassEx(&wcex));

                m_windowHandle = wil::unique_hwnd(CreateWindowEx(
                    0, /* dwExStyle */
                    windowClass,
                    L"WingetMessageOnlyWindow",
                    WS_CHILD, /* dwStyle */
                    0, /* x */
                    0, /* y */
                    0, /* nWidth */
                    0, /* nHeiht*/
                    HWND_MESSAGE, /* hWndParent */
                    NULL, /* hMenu */
                    NULL, /* hInstance */
                    NULL)); /* lpParam */
                THROW_LAST_ERROR_IF_NULL(m_windowHandle);

                ShowWindow(m_windowHandle.get(), SW_HIDE);

                // Force message queue to be created.
                MSG msg;
                PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
                messageQueueReady.SetEvent();

                // Message loop
                BOOL getMessageResult;
                while ((getMessageResult = GetMessage(&msg, m_windowHandle.get(), 0, 0)) != 0)
                {
                    THROW_LAST_ERROR_IF(getMessageResult == -1);
                    DispatchMessage(&msg);
                }
            }

            void CloseWindow()
            {
                if (m_windowThread.joinable())
                {
                    THROW_LAST_ERROR_IF(!PostMessage(m_windowHandle.get(), WM_CLOSE, 0, 0));
                    m_windowThread.join();
                }
            }

            std::mutex m_contextsLock;
            std::vector<Context*> m_contexts;
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

        Logging::Telemetry().LogCommandTermination(hr, file, line);

        m_isTerminated = true;
        m_terminationHR = hr;
    }

    void Context::SetTerminationHR(HRESULT hr)
    {
        m_terminationHR = hr;
        m_isTerminated = true;
    }

    void Context::Cancel(bool exitIfStuck, bool bypassUser)
    {
        Terminate(exitIfStuck ? APPINSTALLER_CLI_ERROR_CTRL_SIGNAL_RECEIVED : E_ABORT);
        Reporter.CancelInProgressTask(bypassUser);
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
#endif
}
