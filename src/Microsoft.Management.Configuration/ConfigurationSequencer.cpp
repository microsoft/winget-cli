// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSequencer.h"
#include "ConfigurationStatus.h"
#include <AppInstallerStrings.h>

using namespace std::chrono_literals;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ConfigurationSequencer::ConfigurationSequencer(ConfigurationDatabase& database) : m_database(database) {}

    ConfigurationSequencer::~ConfigurationSequencer()
    {
        // Best effort attempt to remove our queue row
        try
        {
            m_database.RemoveQueueItem(m_queueItemObjectName);

            auto status = ConfigurationStatus::Instance();
            status->UpdateSetState(m_setInstanceIdentifier, false);
        }
        CATCH_LOG();
    }

    // This function creates necessary objects and records this operation into the table.
    // It then performs the equivalent of `Wait` with a timeout of 0.
    bool ConfigurationSequencer::Enqueue(const Configuration::ConfigurationSet& configurationSet)
    {
        m_setInstanceIdentifier = configurationSet.InstanceIdentifier();

        // Create an arbitrarily named object
        std::wstring objectName = L"WinGetConfigQueue_" + AppInstaller::Utility::CreateNewGuidNameWString();
        m_queueItemObjectName = AppInstaller::Utility::ConvertToUTF8(objectName);
        m_queueItemObject.create(wil::EventOptions::None, objectName.c_str());

        m_database.AddQueueItem(configurationSet, m_queueItemObjectName);

        auto statusInstance = ConfigurationStatus::Instance();
        statusInstance->UpdateSetState(m_setInstanceIdentifier, true);

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
        THROW_LAST_ERROR_IF(status == WAIT_FAILED);

        if (status == WAIT_TIMEOUT)
        {
            return true;
        }

        if (GetQueuePosition() == 0)
        {
            m_database.SetActiveQueueItem(m_queueItemObjectName);
            return false;
        }
        else
        {
            m_applyMutexScope.reset();
            return true;
        }
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
        auto clearCancelCallback = wil::scope_exit([&cancellation]() { cancellation.Callback([]() {}); });

        for (;;)
        {
            DWORD waitResult = WaitForMultipleObjects(ARRAYSIZE(waitHandles), waitHandles, FALSE, INFINITE);
            THROW_LAST_ERROR_IF(waitResult == WAIT_FAILED);

            if (waitResult == WAIT_OBJECT_0)
            {
                // Cancellation
                break;
            }
            else if (waitResult == WAIT_OBJECT_0 + 1 || waitResult == WAIT_ABANDONED_0 + 1)
            {
                // We now hold the apply mutex
                wil::mutex_release_scope_exit applyMutexScope{ m_applyMutex.get() };

                size_t queuePosition = GetQueuePosition();
                if (queuePosition == 0)
                {
                    m_applyMutexScope = std::move(applyMutexScope);
                    m_database.SetActiveQueueItem(m_queueItemObjectName);
                    break;
                }
                else
                {
                    applyMutexScope.reset();
                    std::this_thread::sleep_for(queuePosition * 100ms);
                }
            }
        }
    }

    size_t ConfigurationSequencer::GetQueuePosition()
    {
        auto queueItems = m_database.GetQueueItems();

        // If we get no queue items at all, we assume that the database doesn't support queueing.
        if (queueItems.empty())
        {
            return 0;
        }

        size_t result = 0;
        bool found = false;

        for (const auto& item : queueItems)
        {
            if (item.ObjectName == m_queueItemObjectName)
            {
                found = true;
                break;
            }

            std::wstring objectName = AppInstaller::Utility::ConvertToUTF16(item.ObjectName);
            QueueObjectType itemObject;
            if (itemObject.try_open(objectName.c_str(), SYNCHRONIZE))
            {
                ++result;
            }
            else
            {
                // Best effort attempt to remove the dead queue row
                try
                {
                    m_database.RemoveQueueItem(item.ObjectName);
                }
                CATCH_LOG();
            }
        }

        THROW_HR_IF(E_NOT_SET, !found);

        return result;
    }
}
