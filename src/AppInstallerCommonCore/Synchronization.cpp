// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerSynchronization.h>
#include <AppInstallerStrings.h>


namespace AppInstaller::Synchronization
{
    // The amount of time that we wait in between checking for cancellation
    constexpr std::chrono::milliseconds s_CrossProcessInstallLock_WaitLoopTime = 250ms;

    CrossProcessLock::CrossProcessLock(std::string_view name) : CrossProcessLock(Utility::ConvertToUTF16(name))
    {
    }

    CrossProcessLock::CrossProcessLock(const std::wstring& name)
    {
        m_mutex.create(name.c_str(), 0, SYNCHRONIZE);
    }

    CrossProcessLock::~CrossProcessLock()
    {
        Release();
    }

    bool CrossProcessLock::Acquire(IProgressCallback& progress)
    {
        while (!progress.IsCancelledBy(CancelReason::Any))
        {
            auto lock = m_mutex.acquire(nullptr, static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(s_CrossProcessInstallLock_WaitLoopTime).count()));

            if (lock)
            {
                m_lockThreadId = GetCurrentThreadId();
                m_lock = std::move(lock);
                return true;
            }
        }

        return false;
    }

    void CrossProcessLock::Release()
    {
        if (m_lock)
        {
            // Ensure that we are in fact always releasing on the same thread that acquired the lock.
            // This is to force crashes rather than deadlocks in the event that we make a design error that leads to that.
            FAIL_FAST_IF(m_lockThreadId != GetCurrentThreadId());
            m_lock.reset();
        }
    }

    bool CrossProcessLock::TryAcquireNoWait()
    {
        auto lock = m_mutex.acquire(nullptr, 0);

        if (lock)
        {
            m_lockThreadId = GetCurrentThreadId();
            m_lock = std::move(lock);
            return true;
        }

        return false;
    }

    CrossProcessLock::operator bool() const
    {
        return static_cast<bool>(m_lock);
    }
}
