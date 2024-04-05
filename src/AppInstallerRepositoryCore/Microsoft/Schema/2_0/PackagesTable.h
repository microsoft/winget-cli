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
        // Gets the requested values for the manifest with the given rowid.
        SQLite::Statement PackagesTableGetValuesById_Statement(
            const SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<SQLite::Builder::QualifiedColumn> columns,
            std::initializer_list<std::string_view> manifestColumnNames,
            bool stepAndVerify = true);

        // Builds the search select statement base on the given values.
        std::vector<int> PackagesTableBuildSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            std::initializer_list<SQLite::Builder::QualifiedColumn> columns,
            std::initializer_list<bool> isOneToOnes,
            std::string_view manifestAlias,
            std::string_view valueAlias,
            bool useLike);

        // Prepares a statement to update the value of a single column for the manifest with the given rowid.
        // The first bind value will be the value to set.
        // The second bind value will be the manifest rowid to modify.
        SQLite::Statement PackagesTableUpdateValueIdById_Statement(SQLite::Connection& connection, std::string_view valueName);
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

        // Gets the values requested for the package with the given rowid.
        template <typename... Columns>
        static auto GetValuesById(const SQLite::Connection& connection, SQLite::rowid_t rowid)
        {
            return details::PackagesTableGetValuesById_Statement(connection, rowid, { Columns::Name... }).GetRow<typename SQLite::Builder::TypeInfo<Columns::Type>::value_t...>();
        }

        // Gets the value requested for the package with the given rowid, if it exists.
        template <typename Column>
        static std::optional<typename SQLite::Builder::TypeInfo<Column::Type>::value_t> GetValueById(const SQLite::Connection& connection, SQLite::rowid_t id)
        {
            auto statement = details::PackagesTableGetValuesById_Statement(connection, id, { Column::Name }, false);
            if (statement.Step()) { return statement.GetColumn<typename SQLite::Builder::TypeInfo<Column::Type>::value_t>(0); }
            else { return std::nullopt; }
        }

        // Builds the search select statement base on the given values.
        // If more than one table is provided, no value will be captured.
        // The return value is the bind indices of the values to match against.
        template <typename... Columns>
        static std::vector<int> BuildSearchStatement(SQLite::Builder::StatementBuilder& builder, std::string_view manifestAlias, std::string_view valueAlias, bool useLike)
        {
            return details::PackagesTableBuildSearchStatement(builder, { SQLite::Builder::QualifiedColumn{ Table::TableName(), Table::ValueName() }... }, { Table::IsOneToOne()... }, manifestAlias, valueAlias, useLike);
        }

        // Update the value of a single column for the package with the given rowid.
        template <typename Column>
        static void UpdateValueIdById(SQLite::Connection& connection, SQLite::rowid_t id, const typename SQLite::Builder::TypeInfo<Column::Type>::value_t& value)
        {
            auto stmt = details::PackagesTableUpdateValueIdById_Statement(connection, Column::Name);
            stmt.Bind(1, value);
            stmt.Bind(2, id);
            stmt.Execute();
        }

        // Removes data that is no longer needed for an index that is to be published.
        static void PrepareForPackaging(SQLite::Connection& connection, std::initializer_list<std::string_view> values);

        // Determines if the table is empty.
        static bool IsEmpty(SQLite::Connection& connection);
    };
}
