// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"
#include "ConfigurationSet.h"
#include "ConfigurationUnit.h"
#include "ConfigurationSetChangeData.h"
#include <winget/SQLiteVersion.h>
#include <winget/SQLiteDynamicStorage.h>
#include <wil/cppwinrt_wrl.h>
#include <memory>
#include <tuple>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Interface for interacting with the configuration database.
    struct IConfigurationDatabase
    {
        using ConfigurationSetPtr = winrt::com_ptr<implementation::ConfigurationSet>;
        using ConfigurationUnitPtr = winrt::com_ptr<implementation::ConfigurationUnit>;
        using ConfigurationSetChangeDataPtr = winrt::com_ptr<implementation::ConfigurationSetChangeData>;

        virtual ~IConfigurationDatabase() = default;

        // Gets the latest schema version for the configuration database.
        static AppInstaller::SQLite::Version GetLatestVersion();

        // Creates the version appropriate database object for the given storage.
        static std::unique_ptr<IConfigurationDatabase> CreateFor(const std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage>& storage, bool allowMigration = false);

        // Gets the schema version from the current interface.
        virtual const AppInstaller::SQLite::Version& GetSchemaVersion() = 0;

        // Version 0.1

        // Acts on a database that has been created but contains no tables beyond metadata.
        virtual void InitializeDatabase() = 0;

        // Adds the given set to the database.
        virtual void AddSet(const Configuration::ConfigurationSet& configurationSet) = 0;

        // Updates the set with the given row id using the given set.
        virtual void UpdateSet(AppInstaller::SQLite::rowid_t target, const Configuration::ConfigurationSet& configurationSet) = 0;

        // Removes the set with the given row id from the database.
        virtual void RemoveSet(AppInstaller::SQLite::rowid_t target) = 0;

        // Gets all of the sets in the database.
        virtual std::vector<ConfigurationSetPtr> GetSets() = 0;

        // Gets the row id of the set with the given instance identifier, if present.
        virtual std::optional<AppInstaller::SQLite::rowid_t> GetSetRowId(const GUID& instanceIdentifier) = 0;

        // Version 0.2

        // Migrates from the current interface given.
        // Returns true if supported (or is already same schema version); false if not.
        // Throws on errors that occur during an attempted migration.
        virtual bool MigrateFrom(IConfigurationDatabase* current) = 0;

        // Adds a new queue item for the given configuration set and object name.
        virtual void AddQueueItem(const GUID& instanceIdentifier, const std::string& objectName);

        // Sets the queue item with the given object name as active.
        virtual void SetActiveQueueItem(const std::string& objectName);

        // Gets all queue items in queue order (item at index 0 is active/next).
        virtual std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, DWORD, bool>> GetQueueItems();

        // Removes the queue item with the given object name.
        virtual void RemoveQueueItem(const std::string& objectName);

        // Version 0.3

        // Gets a set from the database.
        virtual ConfigurationSetPtr GetSet(const GUID& instanceIdentifier) = 0;

        // The tuple returned for status items.
        using StatusItemTuple = std::tuple<
            int64_t,
            std::chrono::system_clock::time_point,
            GUID,
            bool,
            std::optional<GUID>,
            int32_t,
            std::optional<HRESULT>,
            std::string,
            std::string,
            ConfigurationUnitResultSource>;

        // Gets all status items that have changed since the given change identifier, in order of changes (last item is the new latest change to pass next).
        virtual std::vector<StatusItemTuple> GetStatusSince(int64_t changeIdentifier);

        // Gets the latest change identifier and the set status items.
        virtual std::tuple<int64_t, std::vector<StatusItemTuple>> GetStatusBaseline();

        // Adds a new listener to the database.
        virtual void AddListener(const std::string& objectName);

        // Removes a listener from the database.
        virtual void RemoveListener(const std::string& objectName);

        // Gets all of the change listeners in the database.
        virtual std::vector<std::tuple<std::string, std::chrono::system_clock::time_point, DWORD>> GetChangeListeners();

        // Updates a set's state.
        virtual void UpdateSetState(const guid& setInstanceIdentifier, ConfigurationSetState state);

        // Updates whether a set is in the queue or not.
        virtual void UpdateSetInQueue(const guid& setInstanceIdentifier, bool inQueue);

        // Updates a unit's state.
        virtual void UpdateUnitState(const guid& setInstanceIdentifier, const ConfigurationSetChangeDataPtr& changeData);

        // Gets a set's state.
        virtual ConfigurationSetState GetSetState(const guid& instanceIdentifier);

        // Gets a set's first apply time.
        virtual std::chrono::system_clock::time_point GetSetFirstApply(const guid& instanceIdentifier);

        // Gets a set's latest apply begin time.
        virtual std::chrono::system_clock::time_point GetSetApplyBegun(const guid& instanceIdentifier);

        // Gets a set's latest apply end time.
        virtual std::chrono::system_clock::time_point GetSetApplyEnded(const guid& instanceIdentifier);

        // Gets a unit's state.
        virtual ConfigurationUnitState GetUnitState(const guid& instanceIdentifier);

        // Gets a unit's latest result information.
        virtual std::optional<std::tuple<HRESULT, std::string, std::string, ConfigurationUnitResultSource>> GetUnitResultInformation(const guid& instanceIdentifier);
    };
}
