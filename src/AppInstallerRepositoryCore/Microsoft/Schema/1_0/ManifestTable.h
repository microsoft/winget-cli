// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"
#include <initializer_list>
#include <optional>
#include <string_view>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        // Selects a manifest by the given value id.
        std::optional<SQLite::rowid_t> ManifestTableSelectByValueId(SQLite::Connection& connection, std::string_view valueName, SQLite::rowid_t id);

        // Gets the requested ids for the manifest with the given rowid.
        SQLite::Statement ManifestTableGetIdsById_Statement(
            SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<std::string_view> values);

        // Gets the requested values for the manifest with the given rowid.
        SQLite::Statement ManifestTableGetValuesById_Statement(
            SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<SQLite::Builder::QualifiedColumn> columns);

        // Update the value of a single column for the manifest with the given rowid.
        void ManifestTableUpdateIdById(SQLite::Connection& connection, std::string_view valueName, SQLite::rowid_t value, SQLite::rowid_t id);
    }

    // Info on the manifest columns.
    struct ManifestColumnInfo
    {
        std::string_view Name;
        bool PrimaryKey;
        bool Unique;
    };

    // A value that is 1:1 with the manifest.
    struct ManifestOneToOneValue
    {
        std::string_view Name;
        SQLite::rowid_t Value;
    };

    // A table that represents a single manifest
    struct ManifestTable
    {
        // Creates the table.
        static void Create(SQLite::Connection& connection, std::initializer_list<ManifestColumnInfo> values);

        // Insert the given values into the table.
        static SQLite::rowid_t Insert(SQLite::Connection& connection, std::initializer_list<ManifestOneToOneValue> values);

        // Select the first rowid of the manifest with the given value.
        template <typename Table>
        static std::optional<SQLite::rowid_t> SelectByValueId(SQLite::Connection& connection, SQLite::rowid_t id)
        {
            return details::ManifestTableSelectByValueId(connection, Table::ValueName(), id);
        }

        // Gets the ids requested for the manifest with the given rowid.
        template <typename... Tables>
        static auto GetIdsById(SQLite::Connection& connection, SQLite::rowid_t id)
        {
            return details::ManifestTableGetIdsById_Statement(connection, id, { Tables::ValueName()... }).GetRow<Tables::id_t...>();
        }

        // Gets the values requested for the manifest with the given rowid.
        template <typename... Tables>
        static auto GetValuesById(SQLite::Connection& connection, SQLite::rowid_t id)
        {
            return details::ManifestTableGetValuesById_Statement(connection, id, { SQLite::Builder::QualifiedColumn{ Tables::TableName(), Tables::ValueName() }... }).GetRow<Tables::value_t...>();
        }

        // Update the value of a single column for the manifest with the given rowid.
        template <typename Table>
        static void UpdateIdById(SQLite::Connection& connection, SQLite::rowid_t id, SQLite::rowid_t value)
        {
            details::ManifestTableUpdateIdById(connection, Table::ValueName(), value, id);
        }

        // Deletes the manifest row with the given rowid.
        static void DeleteById(SQLite::Connection& connection, SQLite::rowid_t id);

        // Determines if the table is empty.
        static bool IsEmpty(SQLite::Connection& connection);
    };
}
