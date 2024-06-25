// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSequencer.h"
#include <AppInstallerStrings.h>


namespace winrt::Microsoft::Management::Configuration::implementation
{
    ConfigurationSequencer::ConfigurationSequencer(ConfigurationDatabase& database) : m_database(database) {}

    ConfigurationSequencer::~ConfigurationSequencer()
    {
        // Best effort attempt to remove our queue row
        try
        {
            m_database.RemoveQueueEntry(m_queueItemObjectName);
        }
        CATCH_LOG();
    }

    // This function creates necessary objects and records this operation into the table.
    // It then performs the equivalent of `Wait` with a timeout of 0.
    bool ConfigurationSequencer::Enqueue(const Configuration::ConfigurationSet& configurationSet)
    {
        // Create an arbitrarily named object
        std::wstring objectName = L"WinGetConfigQueue_" + AppInstaller::Utility::CreateNewGuidNameWString();
        m_queueItemObjectName = AppInstaller::Utility::ConvertToUTF8(objectName);
        m_queueItemObject.create(wil::EventOptions::None, objectName.c_str());

        m_database.AddQueueEntry(configurationSet, m_queueItemObjectName);

        // Create shared mutex
        constexpr PCWSTR applyMutexName = L"WinGetConfigQueueApplyMutex";

        for (int i = 0; !m_applyMutex && i < 2; ++i)
        {
            if (!m_applyMutex.try_create(applyMutexName, 0, SYNCHRONIZE))
            {
                m_applyMutex.try_open(applyMutexName, SYNCHRONIZE);
            }
        }

        THROW_LAST_ERROR_IF(!m_applyMutex);

        // Probe for an empty queue
        DWORD status = 0;
        m_applyMutexScope = m_applyMutex.acquire(&status, 0);

        if (status == WAIT_TIMEOUT)
        {
            return true;
        }

        return !IsFrontOfQueue();
    }

    // The configuration queue consists of a table in the shared database and cooperative handling of said table.
    // At any moment, the active processor must be holding a common named mutex.
    // Each active queue entry also holds their own arbitrarily named object, recorded in the table.
    // 
    // The general mechanism to wait is:
    //  1. Wait on common named mutex
    //  2. Check if first in queue, including probing arbitrary named objects of entries ahead of us
    //  3. If not first, wait for X * queue position, where X is sufficiently high to prevent contention on main mutex
    void ConfigurationSequencer::Wait(AppInstaller::WinRT::AsyncCancellation& cancellation)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !m_applyMutex);

        wil::unique_event cancellationEvent;
        cancellationEvent.create();

        HANDLE waitHandles[2];
        waitHandles[0] = cancellationEvent.get();
        waitHandles[1] = m_applyMutex.get();

        cancellation.Callback([&]() { cancellationEvent.SetEvent(); });
        wil::scope_exit([&cancellation]() { cancellation.Callback([]() {}); });

        for (;;)
        {
            DWORD waitResult = WaitForMultipleObjects(ARRAYSIZE(waitHandles), waitHandles, FALSE, INFINITE);
            THROW_LAST_ERROR_IF(waitResult == WAIT_FAILED);

            // TODO: HANDLE ALL THE OTHER RESULTS
        }
    }

    bool ConfigurationSequencer::IsFrontOfQueue()
    {

    }
}
