// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetChangeData.h"
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteDynamicStorage.h>
#include <winrt/Microsoft.Management.Configuration.h>
#include <memory>
#include <optional>
#include <vector>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Forward declarations
    struct ConfigurationSet;
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

        // Gets the set with the given identifier.
        ConfigurationSetPtr GetSet(const GUID& instanceIdentifier) const;

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
            DWORD ProcessId{};
            bool Active = false;
        };

        // Gets all queue items in queue order (item at index 0 is active/next).
        std::vector<QueueItem> GetQueueItems() const;

        // Removes the queue item with the given object name.
        void RemoveQueueItem(const std::string& objectName);

        // A status line item.
        struct StatusItem
        {
            int64_t ChangeIdentifier;
            std::chrono::system_clock::time_point ChangeTime;
            GUID SetInstanceIdentifier;
            bool InQueue;
            std::optional<GUID> UnitInstanceIdentifier;
            int32_t State;
            std::optional<HRESULT> ResultCode;
            std::string ResultDescription;
            std::string ResultDetails;
            ConfigurationUnitResultSource ResultSource;
        };

        // Gets all changed status items after the given change identifier.
        std::vector<StatusItem> GetStatusSince(int64_t changeIdentifier) const;

        // The status baseline data.
        struct StatusBaseline
        {
            int64_t ChangeIdentifier = 0;
            std::vector<StatusItem> SetStatus;
        };

        // Gets the current status baseline.
        StatusBaseline GetStatusBaseline() const;

        // Data about a status change listener.
        struct StatusChangeListener
        {
            std::string ObjectName;
            std::chrono::system_clock::time_point Started;
            DWORD ProcessId{};
        };

        // Adds a listener to the database.
        void AddListener(const std::string& objectName);

        // Removes a listener from the database.
        void RemoveListener(const std::string& objectName);

        // Gets all listeners in the database.
        std::vector<StatusChangeListener> GetChangeListeners() const;

        // Updates the set state in the database.
        void UpdateSetState(const guid& setInstanceIdentifier, ConfigurationSetState state);

        // Updates the set "in queue" state in the database.
        void UpdateSetInQueue(const guid& setInstanceIdentifier, bool inQueue);

        // Updates the unit state in the database.
        void UpdateUnitState(const guid& setInstanceIdentifier, const com_ptr<implementation::ConfigurationSetChangeData>& changeData);

        // Read various status values.
        ConfigurationSetState GetSetState(const guid& instanceIdentifier);
        std::chrono::system_clock::time_point GetSetFirstApply(const guid& instanceIdentifier);
        std::chrono::system_clock::time_point GetSetApplyBegun(const guid& instanceIdentifier);
        std::chrono::system_clock::time_point GetSetApplyEnded(const guid& instanceIdentifier);
        ConfigurationUnitState GetUnitState(const guid& instanceIdentifier);
        IConfigurationUnitResultInformation GetUnitResultInformation(const guid& instanceIdentifier);

    private:
        std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage> m_connection;
        mutable std::shared_ptr<IConfigurationDatabase> m_database;

        using TransactionLock = decltype(m_connection->TryBeginTransaction({}, true));

        // Begins a transaction, which may require upgrading to a newer schema version.
        TransactionLock BeginTransaction(std::string_view name, bool forWrite, std::shared_ptr<IConfigurationDatabase>& database) const;

        // Performs the boilerplate setup for a read, then executes the given operation.
        template <typename OperationT>
        auto ExecuteReadOperation(std::string_view operationName, OperationT&& operation, bool requireDatabase = false) const;

        // Performs the boilerplate setup for a write, then executes the given operation.
        template <typename OperationT>
        void ExecuteWriteOperation(std::string_view operationName, OperationT&& operation, bool silentlyIgnoreNoDatabase = false);
    };
}
