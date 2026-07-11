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
        // Info on the columns.
        struct ColumnInfo
        {
            template<typename Column>
            static constexpr ColumnInfo Create()
            {
                ColumnInfo result;
                result.Name = Column::Name;
                result.Type = Column::Type;
                result.PrimaryKey = Column::PrimaryKey;
                result.AllowNull = Column::AllowNull;
                return result;
            }

            std::string_view Name;
            SQLite::Builder::Type Type = {};
            bool PrimaryKey = false;
            bool AllowNull = false;
        };

        // Creates the table.
        void PackagesTableCreate(SQLite::Connection& connection, std::initializer_list<ColumnInfo> values);

        // Gets the requested values for the manifest with the given rowid.
        SQLite::Statement PackagesTableGetValuesById_Statement(
            const SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<std::string_view> columns,
            bool stepAndVerify = true);

        // Prepares a statement to update the value of a single column for the manifest with the given rowid.
        // The first bind value will be the value to set.
        // The second bind value will be the manifest rowid to modify.
        SQLite::Statement PackagesTableUpdateValueIdById_Statement(SQLite::Connection& connection, std::string_view valueName);

        // Removes data that is no longer needed for an index that is to be published.
        void PackagesTablePrepareForPackaging(SQLite::Connection& connection, std::initializer_list<ColumnInfo> values);

        // Checks for embedded nulls in the database.
        bool PackagesTableCheckConsistency(const SQLite::Connection& connection, std::initializer_list<std::string_view> values, bool log);
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

        using ColumnInfo = details::ColumnInfo;

        // Creates the table.
        template <typename... Columns>
        static void Create(SQLite::Connection& connection)
        {
            details::PackagesTableCreate(connection, { ColumnInfo::Create<Columns>()... });
        }

        // Drops the table.
        static void Drop(SQLite::Connection& connection);

        // Determine if the table currently exists in the database.
        static bool Exists(const SQLite::Connection& connection);

        // Alters the table, adding the column provided.
        static void AddColumn(SQLite::Connection& connection, const ColumnInfo& value);

        // A string value for the package.
        struct NameValuePair
        {
            std::string_view Name;
            std::string Value;
        };

        // Insert the given values into the table.
        static SQLite::rowid_t Insert(SQLite::Connection& connection, const std::vector<NameValuePair>& values);

        // Gets a value indicating whether the package with rowid exists.
        static bool ExistsById(const SQLite::Connection& connection, SQLite::rowid_t rowid);

        // Gets all row ids from the table.
        static std::vector<SQLite::rowid_t> GetAllRowIds(const SQLite::Connection& connection, std::string_view orderByColumn, size_t limit = 0);

        // Gets the total number of rows in the table.
        static uint64_t GetCount(const SQLite::Connection& connection);

        // Gets the values requested for the package with the given rowid.
        template <typename... Columns>
        static auto GetValuesById(const SQLite::Connection& connection, SQLite::rowid_t rowid)
        {
            return details::PackagesTableGetValuesById_Statement(connection, rowid, { Columns::Name... }).GetRow<typename SQLite::Builder::TypeInfo<Columns::Type, Columns::AllowNull>::value_t...>();
        }

        // Gets the value requested for the package with the given rowid, if it exists.
        template <typename Column>
        static std::optional<typename SQLite::Builder::TypeInfo<Column::Type, false>::value_t> GetValueById(const SQLite::Connection& connection, SQLite::rowid_t rowid)
        {
            auto statement = details::PackagesTableGetValuesById_Statement(connection, rowid, { Column::Name }, false);
            if (statement.Step()) { return statement.GetColumn<typename SQLite::Builder::TypeInfo<Column::Type, Column::AllowNull>::value_t>(0); }
            else { return std::nullopt; }
        }

        // Builds the search select statement base on the given value.
        // The return value is the bind index of the value to match against.
        static int BuildSearchStatement(SQLite::Builder::StatementBuilder& builder, std::string_view valueName, std::string_view primaryAlias, std::string_view valueAlias, bool useLike);

        // Update the value of a single column for the package with the given rowid.
        template <typename Column>
        static void UpdateValueIdById(SQLite::Connection& connection, SQLite::rowid_t id, const typename SQLite::Builder::TypeInfo<Column::Type, Column::AllowNull>::value_t& value)
        {
            auto stmt = details::PackagesTableUpdateValueIdById_Statement(connection, Column::Name);
            stmt.Bind(1, value);
            stmt.Bind(2, id);
            stmt.Execute();
        }

        // Removes data that is no longer needed for an index that is to be published.
        template <typename... Columns>
        static void PrepareForPackaging(SQLite::Connection& connection)
        {
            details::PackagesTablePrepareForPackaging(connection, { ColumnInfo::Create<Columns>()... });
        }

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        template <typename... Columns>
        static bool CheckConsistency(const SQLite::Connection& connection, bool log)
        {
            return details::PackagesTableCheckConsistency(connection, { Columns::Name... }, log);
        }

        // Determines if the table is empty.
        static bool IsEmpty(SQLite::Connection& connection);
    };
}
