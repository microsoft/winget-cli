// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/AsyncTokens.h>
#include <AppInstallerLanguageUtilities.h>
#include <wil/resource.h>
#include <atomic>
#include <set>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Promise type implementation for cancellation_token
    struct ShutdownAwareAsyncCancellationBase
    {
        Windows::Foundation::AsyncStatus Status() noexcept;
        void cancellation_callback(winrt::delegate<>&& cancel) noexcept;
        bool enable_cancellation_propagation(bool value) noexcept;
        void Cancel() noexcept;

    private:
        slim_mutex m_lock;
        winrt::delegate<> m_cancel;
        std::atomic<Windows::Foundation::AsyncStatus> m_status{ Windows::Foundation::AsyncStatus::Started };
    };

    // An AsyncCancellation that registers with ShutdownSynchronization.
    struct ShutdownAwareAsyncCancellation : public AppInstaller::WinRT::AsyncCancellation, ShutdownAwareAsyncCancellationBase
    {
        // Creates a cancellable object without an external cancellation token.
        ShutdownAwareAsyncCancellation();

        // Create a cancellation object from the winrt token.
        template <typename Promise>
        ShutdownAwareAsyncCancellation(winrt::impl::cancellation_token<Promise>&& token) : AsyncCancellation(std::move(token))
        {
            RegisterWithShutdownSynchronization();
        }

        // Removes the shutdown registration.
        ~ShutdownAwareAsyncCancellation();

    protected:
        void RegisterWithShutdownSynchronization();

        AppInstaller::DestructionToken m_destruction{ true };
    };

    // An AsyncProgress that registers with ShutdownSynchronization.
    template <typename ResultT, typename ProgressT>
    struct ShutdownAwareAsyncProgress : public AppInstaller::WinRT::AsyncProgress<ResultT, ProgressT>, ShutdownAwareAsyncCancellationBase
    {
        // Creates a cancellable object without an external cancellation token.
        ShutdownAwareAsyncProgress() :
            AsyncProgress(winrt::impl::cancellation_token<ShutdownAwareAsyncCancellationBase>{ this })
        {
            RegisterWithShutdownSynchronization();
        }

        // Create a progress object from the winrt token.
        template <typename Promise>
        ShutdownAwareAsyncProgress(winrt::impl::progress_token<Promise, ProgressT>&& progress, winrt::impl::cancellation_token<Promise>&& cancellation) :
            AsyncProgress(std::move(progress), std::move(cancellation))
        {
            RegisterWithShutdownSynchronization();
        }

        // Create a progress object from an EventHandler.
        template <typename Promise>
        ShutdownAwareAsyncProgress(winrt::Windows::Foundation::EventHandler<ProgressT>&& progress, winrt::impl::cancellation_token<Promise>&& cancellation) :
            AsyncProgress(std::move(progress), std::move(cancellation))
        {
            RegisterWithShutdownSynchronization();
        }

        // Removes the shutdown registration.
        ~ShutdownAwareAsyncProgress()
        {
            if (m_destruction)
            {
                ShutdownSynchronization::Instance().RegisterWorkEnd(GetWeak());
            }
        }

    protected:
        void RegisterWithShutdownSynchronization()
        {
            ShutdownSynchronization::Instance().RegisterWorkBegin(GetWeak());
        }

        AppInstaller::DestructionToken m_destruction{ true };
    };

    struct ShutdownSynchronization
    {
        using CancellableWeakPtr = std::weak_ptr<AppInstaller::WinRT::details::AsyncCancellationTypeErasure>;

        ShutdownSynchronization() = default;

        static ShutdownSynchronization& Instance();

        // Signals that new work should be blocked.
        void BlockNewWork();

        // Call to register the begin and end of work.
        void RegisterWorkBegin(CancellableWeakPtr&& ptr);
        void RegisterWorkEnd(CancellableWeakPtr&& ptr);

        // Cancels all currently registered work.
        void CancelAllWork();

        // Waits for outstanding work to be completed.
        void Wait();

    private:
        std::atomic_bool m_disabled{ false };
        std::mutex m_workLock;
        std::set<CancellableWeakPtr, std::owner_less<CancellableWeakPtr>> m_work;
        wil::slim_event_manual_reset m_noActiveWork{ true };
    };
}
