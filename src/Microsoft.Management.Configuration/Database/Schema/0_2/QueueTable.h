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

        // Adds a new queue item for the given configuration set and object name.
        void AddQueueItem(const GUID& instanceIdentifier, const std::string& objectName);

        // Sets the queue item with the given object name as active.
        void SetActiveQueueItem(const std::string& objectName);

        // Gets all queue items in queue order (item at index 0 is active/next).
        std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, bool>> GetQueueItems();

        // Removes the queue item with the given object name.
        void RemoveQueueItem(const std::string& objectName);

    private:
        AppInstaller::SQLite::Connection& m_connection;
    };
}
