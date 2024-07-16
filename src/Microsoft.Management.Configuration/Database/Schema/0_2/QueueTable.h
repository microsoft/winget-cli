// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <vector>
#include <tuple>

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_2
{
    struct QueueTable
    {
        QueueTable(AppInstaller::SQLite::Connection& connection);

        // Creates the queue table.
        void Create();

        // Adds the process column to the table.
        void AddProcessColumn();

        // Adds a new queue item for the given configuration set and object name.
        void AddQueueItemWithoutProcess(const GUID& instanceIdentifier, const std::string& objectName);

        // Adds a new queue item for the given configuration set and object name.
        void AddQueueItemWithProcess(const GUID& instanceIdentifier, const std::string& objectName);

        // Sets the queue item with the given object name as active.
        void SetActiveQueueItem(const std::string& objectName);

        // Gets all queue items in queue order (item at index 0 is active/next).
        std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, DWORD, bool>> GetQueueItemsWithoutProcess();

        // Gets all queue items in queue order (item at index 0 is active/next).
        std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, DWORD, bool>> GetQueueItemsWithProcess();

        // Removes the queue item with the given object name.
        void RemoveQueueItem(const std::string& objectName);

    private:
        AppInstaller::SQLite::Connection& m_connection;
    };
}
