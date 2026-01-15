// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/ShutdownMonitoring.h"
#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerRuntime.h>
#include <winget/COMStaticStorage.h>

using namespace std::chrono_literals;

namespace AppInstaller::ShutdownMonitoring
{
    std::shared_ptr<TerminationSignalHandler> TerminationSignalHandler::Instance()
    {
        struct Singleton : public WinRT::COMStaticStorageBase<TerminationSignalHandler>
        {
            Singleton() : COMStaticStorageBase(L"WindowsPackageManager.TerminationSignalHandler") {}
        };

        static Singleton s_instance;
        return s_instance.Get();
    }

    void TerminationSignalHandler::AddListener(ICancellable* cancellable)
    {
        std::lock_guard<std::mutex> lock{ m_listenersLock };

        auto itr = std::find(m_listeners.begin(), m_listeners.end(), cancellable);
        THROW_HR_IF(E_NOT_VALID_STATE, itr != m_listeners.end());
        m_listeners.push_back(cancellable);
    }

    void TerminationSignalHandler::RemoveListener(ICancellable* cancellable)
    {
        std::lock_guard<std::mutex> lock{ m_listenersLock };

        auto itr = std::find(m_listeners.begin(), m_listeners.end(), cancellable);
        if (itr == m_listeners.end())
        {
            AICLI_LOG(CLI, Warning, << "TerminationSignalHandler::RemoveListener did not find requested object");
        }
        else
        {
            m_listeners.erase(itr);
        }
    }

    void TerminationSignalHandler::EnableListener(bool enabled, ICancellable* cancellable)
    {
        if (enabled)
        {
            Instance()->AddListener(cancellable);
        }
        else
        {
            Instance()->RemoveListener(cancellable);
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    HWND TerminationSignalHandler::GetWindowHandle() const
    {
        return m_windowHandle.get();
    }
#endif

    TerminationSignalHandler::TerminationSignalHandler()
    {
        // Create message only window.
        m_messageQueueReady.create();
        m_windowThread = std::thread(&TerminationSignalHandler::CreateWindowAndStartMessageLoop, this);
        if (!m_messageQueueReady.wait(100))
        {
            AICLI_LOG(CLI, Warning, << "Timeout creating winget window");
        }

        // Set up ctrl-c handler.
        LOG_IF_WIN32_BOOL_FALSE(SetConsoleCtrlHandler(StaticCtrlHandlerFunction, TRUE));
    }

    TerminationSignalHandler::~TerminationSignalHandler()
    {
        SetConsoleCtrlHandler(StaticCtrlHandlerFunction, FALSE);

        // std::thread requires that any managed thread (joinable) be joined or detached before destructing
        if (m_windowThread.joinable())
        {
            if (m_windowHandle)
            {
                // Inform the thread that it should stop.
                PostMessageW(m_windowHandle.get(), WM_DESTROY, 0, 0);
            }

            m_windowThread.join();
        }
    }

    void TerminationSignalHandler::StartAppShutdown()
    {
        AICLI_LOG(CLI, Info, << "Initiating shutdown procedure");

        // Lifetime manager sends CTRL-C after the WM_QUERYENDSESSION is processed.
        // If we disable the CTRL-C handler, the default handler will kill us.
        InformListeners(CancelReason::AppShutdown, true);
    }

    BOOL WINAPI TerminationSignalHandler::StaticCtrlHandlerFunction(DWORD ctrlType)
    {
        return Instance()->CtrlHandlerFunction(ctrlType);
    }

    LRESULT WINAPI TerminationSignalHandler::WindowMessageProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_QUERYENDSESSION:
            AICLI_LOG(CLI, Verbose, << "Received WM_QUERYENDSESSION");
            Instance()->StartAppShutdown();
            return TRUE;
        case WM_ENDSESSION:
        case WM_CLOSE:
            AICLI_LOG(CLI, Verbose, << "Received window message type: " << uMsg);
            // We delay as long as needed during the WM_ENDSESSION as we will be terminated on return.
            ServerShutdownSynchronization::WaitForShutdown();
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

    BOOL TerminationSignalHandler::CtrlHandlerFunction(DWORD ctrlType)
    {
        // TODO: Move this to be logged per active context when we have thread static globals
        AICLI_LOG(CLI, Info, << "Got CTRL type: " << ctrlType);

        switch (ctrlType)
        {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            return InformListeners(CancelReason::CtrlCSignal, false);
            // According to MSDN, we should never receive these due to having gdi32/user32 loaded in our process.
            // But handle them as a force terminate anyway.
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            return InformListeners(CancelReason::CtrlCSignal, true);
        default:
            return FALSE;
        }
    }

    // Terminates the currently attached contexts.
    // Returns FALSE if no contexts attached; TRUE otherwise.
    BOOL TerminationSignalHandler::InformListeners(CancelReason reason, bool force)
    {
        std::lock_guard<std::mutex> lock{ m_listenersLock };

        if (m_listeners.empty())
        {
            return FALSE;
        }

        for (auto& listener : m_listeners)
        {
            listener->Cancel(reason, force);
        }

        return TRUE;
    }

    void TerminationSignalHandler::CreateWindowAndStartMessageLoop()
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
        wcex.lpfnWndProc = TerminationSignalHandler::WindowMessageProcedure;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInstance;
        wcex.lpszClassName = windowClass;

        if (!RegisterClassEx(&wcex))
        {
            LOG_LAST_ERROR_MSG("Failed registering window class");
            return;
        }

        // Unregister the window class on exiting the thread
        auto classUnregister = wil::scope_exit([&]()
            {
                UnregisterClassW(windowClass, hInstance);
            });

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

        HWND windowHandle = m_windowHandle.get();
        if (windowHandle == nullptr)
        {
            LOG_LAST_ERROR_MSG("Failed creating window");
            return;
        }

        // We must destroy the window first so that the class unregister can succeed
        auto destroyWindow = wil::scope_exit([&]()
            {
                DestroyWindow(windowHandle);
            });

        ShowWindow(windowHandle, SW_HIDE);

        // Force message queue to be created.
        MSG msg;
        PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
        m_messageQueueReady.SetEvent();

        // Message loop, we send WM_DESTROY to terminate it
        BOOL getMessageResult;
        while ((getMessageResult = GetMessage(&msg, windowHandle, 0, 0)) != 0)
        {
            if (getMessageResult == -1)
            {
                LOG_LAST_ERROR();
                break;
            }
            else if (msg.message == WM_DESTROY)
            {
                break;
            }
            else
            {
                DispatchMessage(&msg);
            }
        }
    }

    void ServerShutdownSynchronization::Initialize(ShutdownCompleteCallback callback)
    {
        Instance().m_callback = callback;
    }

    void ServerShutdownSynchronization::AddComponent(const ComponentSystem& component)
    {
        ServerShutdownSynchronization& instance = Instance();
        std::lock_guard<std::mutex> lock{ instance.m_componentsLock };

        for (const auto& item : instance.m_components)
        {
            if (item.BlockNewWork == component.BlockNewWork ||
                item.BeginShutdown == component.BeginShutdown ||
                item.Wait == component.Wait)
            {
                return;
            }
        }

        instance.m_components.push_back(component);
    }

    bool ServerShutdownSynchronization::WaitForShutdown(std::optional<DWORD> timeout)
    {
        ServerShutdownSynchronization& instance = Instance();

        if (timeout)
        {
            return instance.m_shutdownComplete.wait(timeout.value());
        }
        else
        {
            {
                std::lock_guard<std::mutex> lock{ instance.m_threadLock };
                if (!instance.m_shutdownThread.joinable())
                {
                    AICLI_LOG(Core, Warning, << "Attempt to wait for shutdown when shutdown has not been initiated.");
                    return false;
                }
            }

            return instance.m_shutdownComplete.wait();
        }
    }

    void ServerShutdownSynchronization::Cancel(CancelReason reason, bool)
    {
        std::lock_guard<std::mutex> lock{ m_threadLock };

        if (!m_shutdownThread.joinable())
        {
            m_shutdownThread = std::thread(&ServerShutdownSynchronization::SynchronizeShutdown, this, reason);
        }
    }

    ServerShutdownSynchronization::ServerShutdownSynchronization()
    {
        TerminationSignalHandler::Instance()->AddListener(this);
    }

    ServerShutdownSynchronization::~ServerShutdownSynchronization()
    {
        TerminationSignalHandler::Instance()->RemoveListener(this);
        if (m_shutdownThread.joinable())
        {
            m_shutdownThread.detach();
        }
    }

    ServerShutdownSynchronization& ServerShutdownSynchronization::Instance()
    {
        static ServerShutdownSynchronization s_instance;
        return s_instance;
    }

    void ServerShutdownSynchronization::SynchronizeShutdown(CancelReason reason) try
    {
        auto setShutdownComplete = wil::scope_exit([this]() { this->m_shutdownComplete.SetEvent(); });

        std::vector<ComponentSystem> components;
        {
            std::lock_guard<std::mutex> lock{ m_componentsLock };
            components = m_components;
        }

        AICLI_LOG(CLI, Verbose, << "ServerShutdownSynchronization :: BlockNewWork");
        for (const auto& component : components)
        {
            if (component.BlockNewWork)
            {
                component.BlockNewWork(reason);
            }
        }

        AICLI_LOG(CLI, Verbose, << "ServerShutdownSynchronization :: BeginShutdown");
        for (const auto& component : components)
        {
            if (component.BeginShutdown)
            {
                component.BeginShutdown(reason);
            }
        }

        AICLI_LOG(CLI, Verbose, << "ServerShutdownSynchronization :: Wait");
        for (const auto& component : components)
        {
            if (component.Wait)
            {
                component.Wait();
            }
        }

        AICLI_LOG(CLI, Verbose, << "ServerShutdownSynchronization :: ShutdownCompleteCallback");
        ShutdownCompleteCallback callback = m_callback;
        if (callback)
        {
            callback();
        }
    }
    CATCH_LOG();
}
