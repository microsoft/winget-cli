// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"
#include "Database/Schema/IConfigurationDatabase.h"
#include <winget/SQLiteWrapper.h>

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_1
{
    struct UnitInfoTable
    {
        UnitInfoTable(AppInstaller::SQLite::Connection& connection);

        // Creates the unit info table.
        void Create();

        // Adds the given configuration unit to the table.
        void Add(const Configuration::ConfigurationUnit& configurationUnit, AppInstaller::SQLite::rowid_t setRowId, hstring schemaVersion);

        // Updates the units for the target set.
        void UpdateForSet(AppInstaller::SQLite::rowid_t target, const Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit>& units, hstring schemaVersion);

        // Removes the units from the target set.
        void RemoveForSet(AppInstaller::SQLite::rowid_t target);

        // Gets all of the units for the given set.
        std::vector<IConfigurationDatabase::ConfigurationUnitPtr> GetAllUnitsForSet(AppInstaller::SQLite::rowid_t setRowId, std::string_view schemaVersion);

    private:
        AppInstaller::SQLite::Connection& m_connection;
    };
}
