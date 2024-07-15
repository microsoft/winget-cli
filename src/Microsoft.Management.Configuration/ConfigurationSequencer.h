// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Database/ConfigurationDatabase.h"
#include <winget/AsyncTokens.h>
#include <wil/resource.h>
#include <winrt/Microsoft.Management.Configuration.h>


namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Allows for sequencing of configuration set applications.
    struct ConfigurationSequencer
    {
        ConfigurationSequencer(ConfigurationDatabase& database);

        ConfigurationSequencer(const ConfigurationSequencer&) = delete;
        ConfigurationSequencer& operator=(const ConfigurationSequencer&) = delete;

        ConfigurationSequencer(ConfigurationSequencer&&) = delete;
        ConfigurationSequencer& operator=(ConfigurationSequencer&&) = delete;

        ~ConfigurationSequencer();

        // Enters the current sequencer into the queue of operations.
        // Returns true to indicate that this operation has been queued and must wait.
        // Returns false to indicate that this operation is able to proceed (queued directly to the front).
        bool Enqueue(const Configuration::ConfigurationSet& configurationSet);

        // Waits for this operation to reach the front of the queue.
        // Registers a cancellation callback so that we can halt our waiting.
        void Wait(AppInstaller::WinRT::AsyncCancellation& cancellation);

    private:
        // Determines the effective queue position of this operation; removing queue entries that are not longer active.
        // 0 is the front of the queue.
        size_t GetQueuePosition();

        using QueueObjectType = wil::unique_event;

        ConfigurationDatabase& m_database;
        guid m_setInstanceIdentifier{};
        std::string m_queueItemObjectName;
        QueueObjectType m_queueItemObject;
        wil::unique_mutex m_applyMutex;
        wil::mutex_release_scope_exit m_applyMutexScope;
    };
}
