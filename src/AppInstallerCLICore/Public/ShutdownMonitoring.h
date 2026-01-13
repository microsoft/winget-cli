// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Windows.h>
#include <AppInstallerProgress.h>
#include <wil/resource.h>
#include <memory>
#include <mutex>
#include <optional>

namespace AppInstaller::ShutdownMonitoring
{
    // Type to contain the CTRL signal and window messages handler.
    struct TerminationSignalHandler
    {
        TerminationSignalHandler();

        ~TerminationSignalHandler();

        // Gets the singleton handler.
        static std::shared_ptr<TerminationSignalHandler> Instance();

        // Add a termination listener.
        void AddListener(ICancellable* cancellable);

        // Remove a termination listener.
        void RemoveListener(ICancellable* cancellable);

        // Add or remove the listener based on `enabled`.
        static void EnableListener(bool enabled, ICancellable* cancellable);

        // Gets whether the signal handler is enabled.
        static bool Enabled();

        // Sets whether the signal handler is enabled; the default is true.
        // When set to false, the signal handler instance will not create signal listeners when created.
        static void Enabled(bool enabled);

#ifndef AICLI_DISABLE_TEST_HOOKS
        // Gets the window handle for the message window.
        HWND GetWindowHandle() const;
#endif

    private:
        void StartAppShutdown();

        static BOOL WINAPI StaticCtrlHandlerFunction(DWORD ctrlType);

        static LRESULT WINAPI WindowMessageProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        BOOL CtrlHandlerFunction(DWORD ctrlType);

        // Terminates the currently attached contexts.
        // Returns FALSE if no contexts attached; TRUE otherwise.
        BOOL InformListeners(CancelReason reason, bool force);

        void CreateWindowAndStartMessageLoop();

        std::mutex m_listenersLock;
        std::vector<ICancellable*> m_listeners;
        wil::unique_event m_messageQueueReady;
        wil::unique_hwnd m_windowHandle;
        std::thread m_windowThread;
    };

    // Coordinates shutdown across server components
    struct ServerShutdownSynchronization
    {
        using ShutdownCompleteCallback = void (*)();

        // Initializes the monitoring system and sets up a callback to be invoked when shutdown is completed.
        static void Initialize(ShutdownCompleteCallback callback, bool createTerminationSignalHandler = true);

        // "Interface" for a single component to synchronize with.
        struct ComponentSystem
        {
            // Initiate the shutdown process.
            // Components are expected to set flags to prevent any further work from beginning and return as quickly as possible.
            void (*BlockNewWork)(CancelReason reason) = nullptr;

            // Components are expected to cancel active or pending work as asynchronously as possible.
            void (*BeginShutdown)(CancelReason reason) = nullptr;

            // Components wait until all active and pending work have completed their 
            void (*Wait)() = nullptr;
        };

        // Adds a component to the system.
        static void AddComponent(const ComponentSystem& component);

        // Waits for the shutdown to complete.
        static bool WaitForShutdown(std::optional<DWORD> timeout = std::nullopt);

    private:
        ServerShutdownSynchronization() = default;
        ~ServerShutdownSynchronization();

        friend TerminationSignalHandler;

        static ServerShutdownSynchronization& Instance();

        // Runs the actual shutdown process and invokes the callback.
        void SynchronizeShutdown(CancelReason reason);

        // Listens for a termination signal.
        void Signal(CancelReason reason);

        ShutdownCompleteCallback m_callback = nullptr;
        std::mutex m_componentsLock;
        std::vector<ComponentSystem> m_components;
        std::mutex m_threadLock;
        std::thread m_shutdownThread;
        wil::slim_event_manual_reset m_shutdownComplete;
    };
}
