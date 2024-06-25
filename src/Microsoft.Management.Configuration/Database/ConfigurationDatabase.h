// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSet.h"
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteDynamicStorage.h>
#include <memory>
#include <vector>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Forward declaration of internal interface.
    struct IConfigurationDatabase;

    // Allows access to the configuration database.
    struct ConfigurationDatabase
    {
        using ConfigurationSetPtr = winrt::com_ptr<implementation::ConfigurationSet>;

        ConfigurationDatabase();

        ConfigurationDatabase(const ConfigurationDatabase&) = delete;
        ConfigurationDatabase& operator=(const ConfigurationDatabase&) = delete;

        ConfigurationDatabase(ConfigurationDatabase&&);
        ConfigurationDatabase& operator=(ConfigurationDatabase&&);

        ~ConfigurationDatabase();

        // Ensures that the database connection is established and the schema interface is created appropriately.
        // If `createIfNeeded` is false, this function will not create the database if it does not exist.
        // If not connected, any read methods will return empty results and any write methods will throw.
        void EnsureOpened(bool createIfNeeded = true);

        // Gets all of the configuration sets from the database.
        std::vector<ConfigurationSetPtr> GetSetHistory() const;

        // Writes the given set to the database history, attempting to merge with a matching set if one exists unless preferNewHistory is true.
        void WriteSetHistory(const Configuration::ConfigurationSet& configurationSet, bool preferNewHistory);

        // Removes the given set from the database history if it is present.
        void RemoveSetHistory(const Configuration::ConfigurationSet& configurationSet);

        // Adds a new queue item for the given configuration set and object name.
        void AddQueueItem(const Configuration::ConfigurationSet& configurationSet, const std::string& objectName);

        // Sets the queue item with the given object name as active.
        void SetActiveQueueItem(const std::string& objectName);

        // Data about a queue item.
        struct QueueItem
        {
            GUID SetInstanceIdentifier{};
            std::string ObjectName;
            std::chrono::system_clock::time_point QueuedAt;
            bool Active = false;
        };

        // Gets all queue items in queue order (item at index 0 is active/next).
        std::vector<QueueItem> GetQueueItems() const;

        // Removes the queue item with the given object name.
        void RemoveQueueItem(const std::string& objectName);

    private:
        std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage> m_connection;
        mutable std::unique_ptr<IConfigurationDatabase> m_database;

        using TransactionLock = decltype(m_connection->TryBeginTransaction({}));

        // Begins a transaction, which may require upgrading to a newer schema version.
        TransactionLock BeginTransaction(std::string_view name) const;
    };
}
