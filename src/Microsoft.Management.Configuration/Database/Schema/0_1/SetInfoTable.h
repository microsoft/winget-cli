// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"
#include "Database/Schema/IConfigurationDatabase.h"
#include <winget/SQLiteWrapper.h>
#include <optional>
#include <vector>

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_1
{
    struct SetInfoTable
    {
        SetInfoTable(AppInstaller::SQLite::Connection& connection);

        static std::string_view TableName();
        static std::string_view InstanceIdentifierColumn();

        // Creates the set info table.
        void Create();

        // Adds the given configuration set to the table.
        // Returns the row id of the added set.
        AppInstaller::SQLite::rowid_t Add(const Configuration::ConfigurationSet& configurationSet);

        // Updates the set with the target row id using the given set.
        void Update(AppInstaller::SQLite::rowid_t target, const Configuration::ConfigurationSet& configurationSet);

        // Removes the set with the target row id.
        void Remove(AppInstaller::SQLite::rowid_t target);

        // Gets all of the sets from the table.
        std::vector<IConfigurationDatabase::ConfigurationSetPtr> GetAllSets();

        // Gets the row id of the set with the given instance identifier.
        std::optional<AppInstaller::SQLite::rowid_t> GetSetRowId(const GUID& instanceIdentifier);

        // Gets the set with the given instance identifier.
        IConfigurationDatabase::ConfigurationSetPtr GetSet(const GUID& instanceIdentifier);

        // Gets a set's first apply time.
        std::chrono::system_clock::time_point GetSetFirstApply(const GUID& instanceIdentifier);

    private:
        AppInstaller::SQLite::Connection& m_connection;
    };
}
