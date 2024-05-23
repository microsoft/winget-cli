// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"
#include <winget/SQLiteWrapper.h>

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_1
{
    struct SetInfoTable
    {
        SetInfoTable(AppInstaller::SQLite::Connection& connection);

        // Creates the set info table.
        void Create();

        // Adds the given configuration set to the table.
        // Returns the row id of the added set.
        AppInstaller::SQLite::rowid_t Add(const Configuration::ConfigurationSet& configurationSet);

        // Contains the column numbers for the statement returned by GetAllSetsStatement.
        struct GetAllSetsStatementColumns
        {
            // Type: rowid_t
            constexpr static int RowID = 0;
            // Type: GUID
            constexpr static int InstanceIdentifier = 1;
            // Type: string
            constexpr static int Name = 2;
            // Type: string
            constexpr static int Origin = 3;
            // Type: string
            constexpr static int Path = 4;
            // Type: int64_t (unix epoch)
            constexpr static int FirstApply = 5;
            // Type: string
            constexpr static int SchemaVersion = 6;
            // Type: rowid_t (ValueSet)
            constexpr static int Metadata = 7;
            // Type: string (YAML)
            constexpr static int Parameters = 8;
            // Type: rowid_t (ValueSet)
            constexpr static int Variables = 9;
        };

        // Gets a prepared statement for reading all of the sets from the table.
        AppInstaller::SQLite::Statement GetAllSetsStatement();

    private:
        AppInstaller::SQLite::Connection& m_connection;
    };
}
