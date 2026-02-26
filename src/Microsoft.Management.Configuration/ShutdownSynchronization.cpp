// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ShutdownSynchronization.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ShutdownAwareAsyncCancellation::ShutdownAwareAsyncCancellation()
    {
        m_defaultPromise = std::make_unique<ShutdownAwareAsyncCancellationPromise>();
        m_cancellation = std::make_unique<AppInstaller::WinRT::AsyncCancellation>(winrt::impl::cancellation_token<ShutdownAwareAsyncCancellationPromise>{ m_defaultPromise.get() });
        RegisterWithShutdownSynchronization();
    }

    ShutdownAwareAsyncCancellation::~ShutdownAwareAsyncCancellation()
    {
        if (m_cancellation)
        {
            ShutdownSynchronization::Instance().RegisterWorkEnd(m_cancellation->GetWeak());
        }
    }

    bool ShutdownAwareAsyncCancellation::IsCancelled() const noexcept
    {
        return m_cancellation->IsCancelled();
    }

    void ShutdownAwareAsyncCancellation::ThrowIfCancelled() const
    {
        m_cancellation->ThrowIfCancelled();
    }

    void ShutdownAwareAsyncCancellation::Callback(winrt::delegate<>&& callback) const noexcept
    {
        m_cancellation->Callback(std::move(callback));
    }

    void ShutdownAwareAsyncCancellation::RegisterWithShutdownSynchronization()
    {
        ShutdownSynchronization::Instance().RegisterWorkBegin(m_cancellation->GetWeak());
    }

    Windows::Foundation::AsyncStatus ShutdownAwareAsyncCancellationPromise::Status() noexcept
    {
        return m_status.load(std::memory_order_acquire);
    }

    void ShutdownAwareAsyncCancellationPromise::cancellation_callback(winrt::delegate<>&& cancel) noexcept
    {
        {
            slim_lock_guard const guard(m_lock);

            if (m_status.load(std::memory_order_relaxed) != Windows::Foundation::AsyncStatus::Canceled)
            {
                m_cancel = std::move(cancel);
                return;
            }
        }

        if (cancel)
        {
            cancel();
        }
    }

    bool ShutdownAwareAsyncCancellationPromise::enable_cancellation_propagation(bool) noexcept
    {
        THROW_HR(E_NOTIMPL);
    }

    void ShutdownAwareAsyncCancellationPromise::Cancel() noexcept
    {
        winrt::delegate<> cancel;

        {
            slim_lock_guard const guard(m_lock);

            if (m_status.load(std::memory_order_relaxed) == Windows::Foundation::AsyncStatus::Started)
            {
                m_status.store(Windows::Foundation::AsyncStatus::Canceled, std::memory_order_relaxed);
                cancel = std::move(m_cancel);
            }
        }

        if (cancel)
        {
            cancel();
        }
    }

    ShutdownSynchronization& ShutdownSynchronization::Instance()
    {
        static ShutdownSynchronization s_instance;
        return s_instance;
    }

    void ShutdownSynchronization::BlockNewWork()
    {
        m_disabled = true;
    }

    void ShutdownSynchronization::RegisterWorkBegin(CancellableWeakPtr&& ptr)
    {
        if (m_disabled)
        {
            THROW_HR(E_ABORT);
        }

        std::lock_guard<std::mutex> lock{ m_workLock };
        m_work.emplace(std::move(ptr));
        m_noActiveWork.ResetEvent();
    }

    void ShutdownSynchronization::RegisterWorkEnd(CancellableWeakPtr&& ptr)
    {
        std::lock_guard<std::mutex> lock{ m_workLock };

        auto itr = m_work.find(ptr);
        if (itr != m_work.end())
        {
            m_work.erase(itr);

            if (m_work.empty())
            {
                m_noActiveWork.SetEvent();
            }
        }
    }

    void ShutdownSynchronization::CancelAllWork()
    {
        std::lock_guard<std::mutex> lock{ m_workLock };

        for (auto itr = m_work.begin(); itr != m_work.end(); ++itr)
        {
            if (auto locked = itr->lock())
            {
                locked->Cancel();
            }
            else
            {
                m_work.erase(itr);
            }
        }

        if (m_work.empty())
        {
            m_noActiveWork.SetEvent();
        }
    }

    void ShutdownSynchronization::Wait()
    {
        for (;;)
        {
            {
                std::lock_guard<std::mutex> lock{ m_workLock };

                // Check for any inactive work before waiting
                for (auto itr = m_work.begin(); itr != m_work.end(); ++itr)
                {
                    if (!itr->lock())
                    {
                        m_work.erase(itr);
                    }
                }

                if (m_work.empty())
                {
                    break;
                }
            }

            if (m_noActiveWork.wait(250))
            {
                break;
            }
        }
    }
}
