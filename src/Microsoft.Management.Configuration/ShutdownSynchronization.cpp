// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ShutdownSynchronization.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ShutdownSynchronization& ShutdownSynchronization::Instance()
    {
        static ShutdownSynchronization s_instance;
        return s_instance;
    }

    void ShutdownSynchronization::BlockNewWork()
    {
        m_disabled = true;
    }

    bool ShutdownSynchronization::IsNewWorkBlocked() const
    {
        return m_disabled;
    }

    void ShutdownSynchronization::RegisterWorkBegin()
    {
        m_workCount += 1;
        m_noActiveWork.ResetEvent();
    }

    void ShutdownSynchronization::RegisterWorkEnd()
    {
        if (m_workCount.fetch_sub(1) == 1)
        {
            m_noActiveWork.SetEvent();
        }
    }

    void ShutdownSynchronization::Wait()
    {
        m_noActiveWork.wait();
    }
}
