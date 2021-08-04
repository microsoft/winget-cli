// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionContext.h"
#include "COMContext.h"
#include "winget/UserSettings.h"

namespace AppInstaller::CLI::Execution
{
    using namespace Settings;

    namespace
    {
        // Type to contain the CTRL signal handler.
        struct CtrlHandler
        {
            static CtrlHandler& Instance()
            {
                static CtrlHandler s_instance;
                return s_instance;
            }

            void AddContext(Context* context)
            {
                std::lock_guard<std::mutex> lock{ m_contextsLock };

                // TODO: COMContexts are currently only used specifically for install operations, which Windows does not reliably support concurrently.
                // As a temporary fix, this location which already has locking and is tracking the contexts is convenient to prevent those
                // installs from happening concurrently. Future work will provide a more robust synchronization mechanism which can queue those requests
                // rather than failing.
                for (auto& existingContext : m_contexts)
                {
                    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INSTALL_ALREADY_RUNNING), (dynamic_cast<COMContext*>(existingContext) != 0));
                }

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
            CtrlHandler()
            {
                LOG_IF_WIN32_BOOL_FALSE(SetConsoleCtrlHandler(StaticCtrlHandlerFunction, TRUE));
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

            std::mutex m_contextsLock;
            std::vector<Context*> m_contexts;
        };

        void SetCtrlHandlerContext(bool add, Context* context)
        {
            THROW_HR_IF(E_POINTER, context == nullptr);

            if (add)
            {
                CtrlHandler::Instance().AddContext(context);
            }
            else
            {
                CtrlHandler::Instance().RemoveContext(context);
            }
        }
    }

    Context::~Context()
    {
        if (m_disableCtrlHandlerOnExit)
        {
            EnableCtrlHandler(false);
        }
    }

    std::unique_ptr<Context> Context::Clone()
    {
        auto clone = std::make_unique<Context>(Reporter);
        clone->m_flags = m_flags;
        // If the parent is hooked up to the CTRL signal, have the clone be as well
        if (m_disableCtrlHandlerOnExit)
        {
            clone->EnableCtrlHandler();
        }
        return clone;
    }

    void Context::EnableCtrlHandler(bool enabled)
    {
        SetCtrlHandlerContext(enabled, this);
        m_disableCtrlHandlerOnExit = enabled;
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
                Logging::Telemetry().LogCommandTermination(hr, file, line);
                std::exit(hr);
            }
        }

        Logging::Telemetry().LogCommandTermination(hr, file, line);

        m_isTerminated = true;
        m_terminationHR = hr;
    }

    void Context::Cancel(bool exitIfStuck, bool bypassUser)
    {
        Terminate(exitIfStuck ? APPINSTALLER_CLI_ERROR_CTRL_SIGNAL_RECEIVED : E_ABORT);
        Reporter.CancelInProgressTask(bypassUser);
    }

    void Context::SetExecutionStage(Workflow::ExecutionStage stage, bool allowBackward)
    {
        if (m_executionStage == stage)
        {
            return;
        }
        else if (m_executionStage > stage && !allowBackward)
        {
            THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), "Reporting ExecutionStage to an earlier Stage without allowBackward as true");
        }

        m_executionStage = stage;
        Logging::SetExecutionStage(static_cast<uint32_t>(m_executionStage));
    }
}
