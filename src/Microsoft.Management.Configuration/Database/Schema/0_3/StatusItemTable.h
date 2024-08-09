// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Database/Schema/IConfigurationDatabase.h"
#include <winget/SQLiteWrapper.h>
#include <vector>
#include <tuple>

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_3
{
    struct StatusItemTable
    {
        StatusItemTable(AppInstaller::SQLite::Connection& connection);

        // Creates the table.
        void Create();

        // Removes the status for the target set.
        void RemoveForSet(AppInstaller::SQLite::rowid_t target);

        // Gets all status items that have changed since the given change identifier, in order of changes (last item is the new latest change to pass next).
        std::vector<IConfigurationDatabase::StatusItemTuple> GetStatusSince(int64_t changeIdentifier);

        // Gets the latest change identifier and the set status items.
        std::tuple<int64_t, std::vector<IConfigurationDatabase::StatusItemTuple>> GetStatusBaseline();

        // Updates a set's state.
        void UpdateSetState(const guid& setInstanceIdentifier, ConfigurationSetState state);

        // Updates whether a set is in the queue or not.
        void UpdateSetInQueue(const guid& setInstanceIdentifier, bool inQueue);

        // Updates a unit's state.
        void UpdateUnitState(const guid& setInstanceIdentifier, const IConfigurationDatabase::ConfigurationSetChangeDataPtr& changeData);

        // Gets a set's state.
        ConfigurationSetState GetSetState(const guid& instanceIdentifier);

        // Gets a set's latest apply begin time.
        std::chrono::system_clock::time_point GetSetApplyBegun(const GUID& instanceIdentifier);

        // Gets a set's latest apply end time.
        std::chrono::system_clock::time_point GetSetApplyEnded(const GUID& instanceIdentifier);

        // Gets a unit's state.
        ConfigurationUnitState GetUnitState(const guid& instanceIdentifier);

        // Gets a unit's latest result information.
        std::optional<std::tuple<HRESULT, std::string, std::string, ConfigurationUnitResultSource>> GetUnitResultInformation(const guid& instanceIdentifier);

    private:
        AppInstaller::SQLite::Connection& m_connection;
    };
}
