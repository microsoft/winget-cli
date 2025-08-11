// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/AsyncTokens.h>
#include <wil/resource.h>
#include <atomic>
#include <set>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Promise type implementation for cancellation_token
    struct ShutdownAwareAsyncCancellationPromise
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
    struct ShutdownAwareAsyncCancellation
    {
        // Creates a cancellable object without an external cancellation token.
        ShutdownAwareAsyncCancellation();

        // Create a cancellation object from the winrt token.
        template <typename Promise>
        ShutdownAwareAsyncCancellation(winrt::impl::cancellation_token<Promise>&& token)
        {
            m_cancellation = std::make_unique<AppInstaller::WinRT::AsyncCancellation>(std::move(token));
            RegisterWithShutdownSynchronization();
        }

        // Removes the shutdown registration.
        ~ShutdownAwareAsyncCancellation();

        // Returns true if the operation has been cancelled, false if not.
        bool IsCancelled() const noexcept;

        // Throws the appropriate exception if the operation has been cancelled.
        void ThrowIfCancelled() const;

        // Sets a callback that will be invoked on cancellation.
        void Callback(winrt::delegate<>&& callback) const noexcept;

    protected:
        void RegisterWithShutdownSynchronization();

        std::unique_ptr<ShutdownAwareAsyncCancellationPromise> m_defaultPromise;
        std::unique_ptr<AppInstaller::WinRT::AsyncCancellation> m_cancellation;
    };

    // An AsyncProgress that registers with ShutdownSynchronization.
    template <typename ResultT, typename ProgressT>
    struct ShutdownAwareAsyncProgress
    {
        // Creates a cancellable object without an external cancellation token.
        ShutdownAwareAsyncProgress()
        {
            m_defaultPromise = std::make_unique<ShutdownAwareAsyncCancellationPromise>();
            m_progress = std::make_unique<AppInstaller::WinRT::AsyncProgress<ResultT, ProgressT>>(winrt::impl::cancellation_token<ShutdownAwareAsyncCancellationPromise>{ m_defaultPromise.get() });
            RegisterWithShutdownSynchronization();
        }

        // Create a progress object from the winrt token.
        template <typename Promise>
        ShutdownAwareAsyncProgress(winrt::impl::progress_token<Promise, ProgressT>&& progress, winrt::impl::cancellation_token<Promise>&& cancellation)
        {
            m_progress = std::make_unique<AppInstaller::WinRT::AsyncProgress<ResultT, ProgressT>>(std::move(progress), std::move(cancellation));
            RegisterWithShutdownSynchronization();
        }

        // Create a progress object from an EventHandler.
        template <typename Promise>
        ShutdownAwareAsyncProgress(winrt::Windows::Foundation::EventHandler<ProgressT>&& progress, winrt::impl::cancellation_token<Promise>&& cancellation)
        {
            m_progress = std::make_unique<AppInstaller::WinRT::AsyncProgress<ResultT, ProgressT>>(std::move(progress), std::move(cancellation));
            RegisterWithShutdownSynchronization();
        }

        ShutdownAwareAsyncProgress(const ShutdownAwareAsyncProgress&) = delete;
        ShutdownAwareAsyncProgress& operator=(const ShutdownAwareAsyncProgress&) = delete;

        ShutdownAwareAsyncProgress(ShutdownAwareAsyncProgress&&) = default;
        ShutdownAwareAsyncProgress& operator=(ShutdownAwareAsyncProgress&&) = default;

        // Removes the shutdown registration.
        ~ShutdownAwareAsyncProgress()
        {
            if (m_progress)
            {
                ShutdownSynchronization::Instance().RegisterWorkEnd(m_progress->GetWeak());
            }
        }

        AppInstaller::WinRT::AsyncCancellation& GetCancellation()
        {
            return *m_progress;
        }

        // Returns true if the operation has been cancelled, false if not.
        bool IsCancelled() const noexcept
        {
            return m_progress->IsCancelled();
        }

        // Throws the appropriate exception if the operation has been cancelled.
        void ThrowIfCancelled() const
        {
            m_progress->ThrowIfCancelled();
        }

        // Sets a callback that will be invoked on cancellation.
        void Callback(winrt::delegate<>&& callback) const noexcept
        {
            m_progress->Callback(std::move(callback));
        }

        // Sends progress if this object is not empty.
        void Progress(ProgressT const& progress) const
        {
            m_progress->Progress(progress);
        }

        // Sets the result onto the progress object if it is not empty.
        void Result(ResultT const& result) const
        {
            m_progress->Result(result);
        }

    protected:
        void RegisterWithShutdownSynchronization()
        {
            ShutdownSynchronization::Instance().RegisterWorkBegin(m_progress->GetWeak());
        }

        std::unique_ptr<ShutdownAwareAsyncCancellationPromise> m_defaultPromise;
        std::unique_ptr<AppInstaller::WinRT::AsyncProgress<ResultT, ProgressT>> m_progress;
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
