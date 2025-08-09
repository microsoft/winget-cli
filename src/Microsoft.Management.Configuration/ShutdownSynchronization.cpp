// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ShutdownSynchronization.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ShutdownAwareAsyncCancellation::ShutdownAwareAsyncCancellation() : AsyncCancellation(winrt::impl::cancellation_token<ShutdownAwareAsyncCancellationBase>{ this }) {}

    ShutdownAwareAsyncCancellation::~ShutdownAwareAsyncCancellation()
    {
        if (m_destruction)
        {
            ShutdownSynchronization::Instance().RegisterWorkEnd(GetWeak());
        }
    }

    Windows::Foundation::AsyncStatus ShutdownAwareAsyncCancellationBase::Status() noexcept
    {
        return m_status.load(std::memory_order_acquire);
    }

    void ShutdownAwareAsyncCancellationBase::cancellation_callback(winrt::delegate<>&& cancel) noexcept
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

    bool ShutdownAwareAsyncCancellationBase::enable_cancellation_propagation(bool) noexcept
    {
        THROW_HR(E_NOTIMPL);
    }

    void ShutdownAwareAsyncCancellationBase::Cancel() noexcept
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

    void ShutdownAwareAsyncCancellation::RegisterWithShutdownSynchronization()
    {
        ShutdownSynchronization::Instance().RegisterWorkBegin(GetWeak());
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
