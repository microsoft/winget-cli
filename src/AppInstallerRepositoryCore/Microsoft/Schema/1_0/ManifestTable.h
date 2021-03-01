// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"
#include <initializer_list>
#include <optional>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        // Selects a manifest by the given value id.
        std::optional<SQLite::rowid_t> ManifestTableSelectByValueIds(
            const SQLite::Connection& connection,
            std::initializer_list<std::string_view> values,
            std::initializer_list<SQLite::rowid_t> ids);

        // Gets the requested ids for the manifest with the given rowid.
        SQLite::Statement ManifestTableGetIdsById_Statement(
            const SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<std::string_view> values);

        // Gets the requested values for the manifest with the given rowid.
        SQLite::Statement ManifestTableGetValuesById_Statement(
            const SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<SQLite::Builder::QualifiedColumn> columns);

        // Gets all values for rows that match the given ids.
        SQLite::Statement ManifestTableGetAllValuesByIds_Statement(
            const SQLite::Connection& connection,
            std::initializer_list<SQLite::Builder::QualifiedColumn> valueColumns,
            std::initializer_list<std::string_view> idColumns,
            std::initializer_list<SQLite::rowid_t> ids);

        // Gets all values for rows that match the given ids.
        std::vector<std::string> ManifestTableGetAllValuesByIds(
            const SQLite::Connection& connection,
            std::initializer_list<SQLite::Builder::QualifiedColumn> valueColumns,
            std::initializer_list<std::string_view> idColumns,
            std::initializer_list<SQLite::rowid_t> ids);

        // Builds the search select statement base on the given values.
        int ManifestTableBuildSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            const SQLite::Builder::QualifiedColumn& column,
            bool isOneToOne,
            std::string_view manifestAlias,
            std::string_view valueAlias,
            bool useLike);

        // Update the value of a single column for the manifest with the given rowid.
        void ManifestTableUpdateValueIdById(SQLite::Connection& connection, std::string_view valueName, SQLite::rowid_t value, SQLite::rowid_t id);

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        bool ManifestTableCheckConsistency(const SQLite::Connection& connection, const SQLite::Builder::QualifiedColumn& target, bool log);
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
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection, std::initializer_list<ManifestColumnInfo> values);

        // Creates the table with standard primary keys.
        static void Create_deprecated(SQLite::Connection& connection, std::initializer_list<ManifestColumnInfo> values);

        // Insert the given values into the table.
        static SQLite::rowid_t Insert(SQLite::Connection& connection, std::initializer_list<ManifestOneToOneValue> values);

        // Gets a value indicating whether the manifest with rowid id exists.
        static bool ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id);

        // Select the first rowid of the manifest with the given value.
        template <typename... Tables>
        static std::optional<SQLite::rowid_t> SelectByValueIds(const SQLite::Connection& connection, std::initializer_list<SQLite::rowid_t> ids)
        {
            static_assert(sizeof...(Tables) >= 1);
            return details::ManifestTableSelectByValueIds(connection, { Tables::ValueName()... }, ids);
        }

        // Gets the ids requested for the manifest with the given rowid.
        template <typename... Tables>
        static auto GetIdsById(const SQLite::Connection& connection, SQLite::rowid_t id)
        {
            return details::ManifestTableGetIdsById_Statement(connection, id, { Tables::ValueName()... }).GetRow<Tables::id_t...>();
        }

        // Gets the values requested for the manifest with the given rowid.
        template <typename... Tables>
        static auto GetValuesById(const SQLite::Connection& connection, SQLite::rowid_t id)
        {
            return details::ManifestTableGetValuesById_Statement(connection, id, { SQLite::Builder::QualifiedColumn{ Tables::TableName(), Tables::ValueName() }... }).GetRow<Tables::value_t...>();
        }

        // Gets the values for rows that match the given ids.
        template <typename ValueTable, typename... IdTables>
        static std::vector<typename ValueTable::value_t> GetAllValuesByIds(const SQLite::Connection& connection, std::initializer_list<SQLite::rowid_t> ids)
        {
            return details::ManifestTableGetAllValuesByIds(connection, { SQLite::Builder::QualifiedColumn{ ValueTable::TableName(), ValueTable::ValueName() } }, { IdTables::ValueName()... }, ids);
        }

        // Gets all values for rows that match the given id.
        template <typename IdTable, typename... ValueTables>
        static std::vector<std::tuple<typename ValueTables::value_t...>> GetAllValuesById(const SQLite::Connection& connection, SQLite::rowid_t id)
        {
            auto stmt = details::ManifestTableGetAllValuesByIds_Statement(connection, { SQLite::Builder::QualifiedColumn{ ValueTables::TableName(), ValueTables::ValueName() }... }, { IdTable::ValueName() }, { id });
            std::vector<std::tuple<typename ValueTables::value_t...>> result;
            while (stmt.Step())
            {
                result.emplace_back(stmt.GetRow<ValueTables::value_t...>());
            }
            return result;
        }

        // Builds the search select statement base on the given values.
        template <typename Table>
        static int BuildSearchStatement(SQLite::Builder::StatementBuilder& builder, std::string_view manifestAlias, std::string_view valueAlias, bool useLike)
        {
            return details::ManifestTableBuildSearchStatement(builder, SQLite::Builder::QualifiedColumn{ Table::TableName(), Table::ValueName() }, Table::IsOneToOne(), manifestAlias, valueAlias, useLike);
        }

        // Update the value of a single column for the manifest with the given rowid.
        template <typename Table>
        static void UpdateValueIdById(SQLite::Connection& connection, SQLite::rowid_t id, SQLite::rowid_t value)
        {
            details::ManifestTableUpdateValueIdById(connection, Table::ValueName(), value, id);
        }

        // Deletes the manifest row with the given rowid.
        static void DeleteById(SQLite::Connection& connection, SQLite::rowid_t id);

        // Removes data that is no longer needed for an index that is to be published.
        static void PrepareForPackaging(SQLite::Connection& connection, std::initializer_list<std::string_view> values);

        // Removes data that is no longer needed for an index that is to be published.
        static void PrepareForPackaging_deprecated(SQLite::Connection& connection, std::initializer_list<std::string_view> values);

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        template <typename Table>
        static bool CheckConsistency(const SQLite::Connection& connection, bool log)
        {
            return details::ManifestTableCheckConsistency(connection, SQLite::Builder::QualifiedColumn{ Table::TableName(), Table::ValueName() }, log);
        }

        // Determines if the table is empty.
        static bool IsEmpty(SQLite::Connection& connection);
    };
}
