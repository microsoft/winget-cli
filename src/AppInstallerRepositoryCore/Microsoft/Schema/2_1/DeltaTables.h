// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V2_1::Delta
{
    // Describes a 2.0 table that associates string values with packages.
    struct ValueTableInfo
    {
        std::string_view TableName;
        std::string_view ValueName;
    };

    // The tables that store a value directly alongside the package that it refers to.
    std::vector<ValueTableInfo> SystemReferenceTables();

    // The tables that store values in a data table, associated with packages through a map table.
    std::vector<ValueTableInfo> OneToManyTables();

    // Gets the name of the delta table that mirrors the given 2.0 table.
    // The delta tables are named distinctly so that a delta database can be attached alongside
    // a baseline, and so that the merged views can take the 2.0 names for themselves.
    std::string GetTableName(std::string_view baseTableName);

    // Gets the name of the delta map table that mirrors the map table of the given 2.0 table.
    std::string GetMapTableName(std::string_view baseTableName);

    // The column that records a row as representing the removal of the data that it identifies.
    std::string_view IsRemovedColumnName();

    // Creates the full set of delta tables in the given database.
    void CreateTables(SQLite::Connection& connection);

    // Performs the necessary packaging steps for the delta tables.
    void PrepareTablesForPackaging(SQLite::Connection& connection);
}
