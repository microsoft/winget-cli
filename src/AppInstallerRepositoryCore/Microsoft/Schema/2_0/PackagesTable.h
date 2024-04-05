// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteStatementBuilder.h>
#include <initializer_list>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    namespace details
    {
        template<typename Table>
        std::string_view GetManifestTableColumnName()
        {
            if constexpr (std::is_base_of<VirtualTableBase, Table>())
            {
                return Table::ManifestColumnName();
            }
            else
            {
                return Table::ValueName();
            }
        }

        // Selects a manifest by the given value id.
        std::optional<SQLite::rowid_t> ManifestTableSelectByValueIds(
            const SQLite::Connection& connection,
            std::initializer_list<std::string_view> values,
            std::initializer_list<SQLite::rowid_t> ids);

        // Gets the requested ids for the manifest with the given rowid.
        SQLite::Statement ManifestTableGetIdsById_Statement(
            const SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<std::string_view> values,
            bool stepAndVerify = true);

        // Gets the requested values for the manifest with the given rowid.
        SQLite::Statement ManifestTableGetValuesById_Statement(
            const SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<SQLite::Builder::QualifiedColumn> columns,
            std::initializer_list<std::string_view> manifestColumnNames,
            bool stepAndVerify = true);

        // Gets all values for rows that match the given ids.
        SQLite::Statement ManifestTableGetAllValuesByIds_Statement(
            const SQLite::Connection& connection,
            std::initializer_list<SQLite::Builder::QualifiedColumn> valueColumns,
            std::initializer_list<SQLite::Builder::QualifiedColumn> joinColumns,
            std::initializer_list<std::string_view> idColumns,
            std::initializer_list<SQLite::rowid_t> ids);

        // Gets all values for rows that match the given ids.
        std::vector<std::pair<SQLite::rowid_t, std::string>> ManifestTableGetAllValuesByIds(
            const SQLite::Connection& connection,
            std::initializer_list<SQLite::Builder::QualifiedColumn> valueColumns,
            std::initializer_list<SQLite::Builder::QualifiedColumn> joinColumns,
            std::initializer_list<std::string_view> idColumns,
            std::initializer_list<SQLite::rowid_t> ids);

        // Builds the search select statement base on the given values.
        std::vector<int> ManifestTableBuildSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            std::initializer_list<SQLite::Builder::QualifiedColumn> columns,
            std::initializer_list<bool> isOneToOnes,
            std::string_view manifestAlias,
            std::string_view valueAlias,
            bool useLike);

        // Prepares a statement to update the value of a single column for the manifest with the given rowid.
        // The first bind value will be the value to set.
        // The second bind value will be the manifest rowid to modify.
        SQLite::Statement ManifestTableUpdateValueIdById_Statement(SQLite::Connection& connection, std::string_view valueName);

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        bool ManifestTableCheckConsistency(const SQLite::Connection& connection, const SQLite::Builder::QualifiedColumn& target, std::string_view manifestColumnName, bool log);
    }

    // A table in which each row represents a single package.
    struct PackagesTable
    {
        // Get the table name.
        static std::string_view TableName();

        struct IdColumn
        {
            static constexpr std::string_view Name = "id"sv;
            static constexpr SQLite::Builder::Type Type = SQLite::Builder::Type::Text;
            static constexpr bool PrimaryKey = true;
            static constexpr bool AllowNull = false;
        };

        struct NameColumn
        {
            static constexpr std::string_view Name = "name"sv;
            static constexpr SQLite::Builder::Type Type = SQLite::Builder::Type::Text;
            static constexpr bool PrimaryKey = false;
            static constexpr bool AllowNull = false;
        };

        struct MonikerColumn
        {
            static constexpr std::string_view Name = "moniker"sv;
            static constexpr SQLite::Builder::Type Type = SQLite::Builder::Type::Text;
            static constexpr bool PrimaryKey = false;
            static constexpr bool AllowNull = true;
        };

        struct LatestVersionColumn
        {
            static constexpr std::string_view Name = "latest_version"sv;
            static constexpr SQLite::Builder::Type Type = SQLite::Builder::Type::Text;
            static constexpr bool PrimaryKey = false;
            static constexpr bool AllowNull = false;
        };

        struct ARPMinVersionColumn
        {
            static constexpr std::string_view Name = "arp_min_version"sv;
            static constexpr SQLite::Builder::Type Type = SQLite::Builder::Type::Text;
            static constexpr bool PrimaryKey = false;
            static constexpr bool AllowNull = true;
        };

        struct ARPMaxVersionColumn
        {
            static constexpr std::string_view Name = "arp_max_version"sv;
            static constexpr SQLite::Builder::Type Type = SQLite::Builder::Type::Text;
            static constexpr bool PrimaryKey = false;
            static constexpr bool AllowNull = true;
        };

        struct HashColumn
        {
            static constexpr std::string_view Name = "hash"sv;
            static constexpr SQLite::Builder::Type Type = SQLite::Builder::Type::Blob;
            static constexpr bool PrimaryKey = false;
            static constexpr bool AllowNull = true;
        };

        // Info on the columns.
        struct ColumnInfo
        {
            template<typename Column>
            ColumnInfo()
            {
                Name = Column::Name;
                Type = Column::Type;
                PrimaryKey = Column::PrimaryKey;
                AllowNull = Column::AllowNull;
            }

            std::string_view Name;
            SQLite::Builder::Type Type;
            bool PrimaryKey = false;
            bool AllowNull = false;
        };

        // Creates the table.
        static void Create(SQLite::Connection& connection, std::initializer_list<ColumnInfo> values);

        // Alters the table, adding the column provided.
        static void AddColumn(SQLite::Connection& connection, const ColumnInfo& value);

        // A string value for the package.
        struct NameValuePair
        {
            std::string_view Name;
            std::string_view Value;
        };

        // Insert the given values into the table.
        static SQLite::rowid_t Insert(SQLite::Connection& connection, std::initializer_list<NameValuePair> values);

        // Gets a value indicating whether the package with rowid exists.
        static bool ExistsById(const SQLite::Connection& connection, SQLite::rowid_t rowid);

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
            return details::ManifestTableGetIdsById_Statement(connection, id, { details::GetManifestTableColumnName<Tables>()...}).GetRow<typename Tables::id_t...>();
        }

        // Gets the id requested for the manifest with the given rowid, if it exists.
        template <typename Table>
        static std::optional<typename Table::id_t> GetIdById(const SQLite::Connection& connection, SQLite::rowid_t id)
        {
            auto statement = details::ManifestTableGetIdsById_Statement(connection, id, { details::GetManifestTableColumnName<Table>() }, false);
            if (statement.Step()) { return statement.GetColumn<typename Table::id_t>(0); }
            else { return std::nullopt; }
        }

        // Gets the values requested for the manifest with the given rowid.
        template <typename... Tables>
        static auto GetValuesById(const SQLite::Connection& connection, SQLite::rowid_t id)
        {
            return details::ManifestTableGetValuesById_Statement(connection, id, { SQLite::Builder::QualifiedColumn{ Tables::TableName(), Tables::ValueName() }... }, { details::GetManifestTableColumnName<Tables>()... }).GetRow<typename Tables::value_t...>();
        }

        // Gets the value requested for the manifest with the given rowid, if it exists.
        template <typename Table>
        static std::optional<typename Table::value_t> GetValueById(const SQLite::Connection& connection, SQLite::rowid_t id)
        {
            auto statement = details::ManifestTableGetValuesById_Statement(connection, id, { SQLite::Builder::QualifiedColumn{ Table::TableName(), Table::ValueName() } }, { details::GetManifestTableColumnName<Table>() }, false);
            if (statement.Step()) { return statement.GetColumn<typename Table::value_t>(0); }
            else { return std::nullopt; }
        }

        // Gets the row ids and values for rows that match the given ids.
        template <typename ValueTable, typename... IdTables>
        static std::vector<std::pair<SQLite::rowid_t, typename ValueTable::value_t>> GetAllValuesByIds(const SQLite::Connection& connection, std::initializer_list<SQLite::rowid_t> ids)
        {
            return details::ManifestTableGetAllValuesByIds(connection,
                { SQLite::Builder::QualifiedColumn{ ValueTable::TableName(), SQLite::RowIDName }, SQLite::Builder::QualifiedColumn{ ValueTable::TableName(), ValueTable::ValueName() } },
                { SQLite::Builder::QualifiedColumn{ ValueTable::TableName(), ValueTable::ValueName() } },
                { IdTables::ValueName()... }, ids);
        }

        // Gets all values for rows that match the given id.
        template <typename IdTable, typename... ValueTables>
        static std::vector<std::tuple<SQLite::rowid_t, typename ValueTables::value_t...>> GetAllValuesById(const SQLite::Connection& connection, SQLite::rowid_t id)
        {
            auto stmt = details::ManifestTableGetAllValuesByIds_Statement(connection,
                { SQLite::Builder::QualifiedColumn{ TableName(), SQLite::RowIDName }, SQLite::Builder::QualifiedColumn{ ValueTables::TableName(), ValueTables::ValueName() }... },
                { SQLite::Builder::QualifiedColumn{ ValueTables::TableName(), ValueTables::ValueName() }... },
                { IdTable::ValueName() }, { id });
            std::vector<std::tuple<SQLite::rowid_t, typename ValueTables::value_t...>> result;
            while (stmt.Step())
            {
                result.emplace_back(stmt.GetRow<SQLite::rowid_t, typename ValueTables::value_t...>());
            }
            return result;
        }

        // Builds the search select statement base on the given values.
        // If more than one table is provided, no value will be captured.
        // The return value is the bind indices of the values to match against.
        template <typename... Table>
        static std::vector<int> BuildSearchStatement(SQLite::Builder::StatementBuilder& builder, std::string_view manifestAlias, std::string_view valueAlias, bool useLike)
        {
            return details::ManifestTableBuildSearchStatement(builder, { SQLite::Builder::QualifiedColumn{ Table::TableName(), Table::ValueName() }... }, { Table::IsOneToOne()... }, manifestAlias, valueAlias, useLike);
        }

        // Update the value of a single column for the manifest with the given rowid.
        template <typename Table>
        static void UpdateValueIdById(SQLite::Connection& connection, SQLite::rowid_t id, const typename Table::id_t& value)
        {
            auto stmt = details::ManifestTableUpdateValueIdById_Statement(connection, details::GetManifestTableColumnName<Table>());
            stmt.Bind(1, value);
            stmt.Bind(2, id);
            stmt.Execute();
        }

        // Deletes the manifest row with the given rowid.
        static void DeleteById(SQLite::Connection& connection, SQLite::rowid_t id);

        // Removes data that is no longer needed for an index that is to be published.
        static void PrepareForPackaging(SQLite::Connection& connection, std::initializer_list<std::string_view> values);

        // Removes data that is no longer needed for an index that is to be published.
        static void PrepareForPackaging_deprecated(SQLite::Connection& connection, std::initializer_list<std::string_view> values);

        // Checks if the row id is present in the column denoted by the value supplied.
        static bool IsValueReferenced(const SQLite::Connection& connection, std::string_view valueName, SQLite::rowid_t valueRowId);

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        template <typename Table>
        static bool CheckConsistency(const SQLite::Connection& connection, bool log)
        {
            return details::ManifestTableCheckConsistency(
                connection, SQLite::Builder::QualifiedColumn{ Table::TableName(), Table::ValueName() }, details::GetManifestTableColumnName<Table>(), log);
        }

        // Determines if the table is empty.
        static bool IsEmpty(SQLite::Connection& connection);
    };
}
