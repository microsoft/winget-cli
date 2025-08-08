// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <atomic>
#include <wil/resource.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ShutdownSynchronization
    {
        ShutdownSynchronization() = default;

        static ShutdownSynchronization& Instance();

        // Signals that new work should be blocked.
        void BlockNewWork();

        // Checks if new work is blocked.
        bool IsNewWorkBlocked() const;

        // Call to register the begin and end of work.
        void RegisterWorkBegin();
        void RegisterWorkEnd();

        // Waits for outstanding work to be completed.
        void Wait();

    private:
        std::atomic_bool m_disabled{ false };
        std::atomic_int64_t m_workCount{ 0 };
        wil::slim_event_manual_reset m_noActiveWork;
    };
}
