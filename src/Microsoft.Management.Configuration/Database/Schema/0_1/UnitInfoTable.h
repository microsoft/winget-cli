// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"
#include <winget/SQLiteWrapper.h>

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_1
{
    struct UnitInfoTable
    {
        UnitInfoTable(AppInstaller::SQLite::Connection& connection);

        // Creates the unit info table.
        void Create();

        // Adds the given configuration unit to the table.
        void Add(const Configuration::ConfigurationUnit& configurationUnit, AppInstaller::SQLite::rowid_t setRowId);

        // Contains the column numbers for the statement returned by GetAllUnitsForSetStatement.
        struct GetAllUnitsForSetStatementColumns
        {
            constexpr static int RowID = 0;
            constexpr static int InstanceIdentifier = 1;
        };

        // Gets all of the units for the given set.
        std::vector<IConfigurationDatabase::ConfigurationUnitPtr> GetAllUnitsForSet(AppInstaller::SQLite::rowid_t setRowId);

    private:
        AppInstaller::SQLite::Connection& m_connection;
    };
}
