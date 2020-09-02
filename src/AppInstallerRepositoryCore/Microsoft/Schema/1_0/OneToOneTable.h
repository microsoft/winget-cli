// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        // Creates the table.
        void CreateOneToOneTable(SQLite::Connection& connection, bool useNamedIndeces, std::string_view tableName, std::string_view valueName);

        // Selects the value from the table, returning the rowid if it exists.
        std::optional<SQLite::rowid_t> OneToOneTableSelectIdByValue(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, std::string_view value, bool useLike = false);

        // Selects the value from the table, returning the rowid if it exists.
        std::optional<std::string> OneToOneTableSelectValueById(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t id);

        // Gets all row ids from the table.
        std::vector<SQLite::rowid_t> OneToOneTableGetAllRowIds(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, size_t limit);

        // Ensures that the values exists in the table.
        SQLite::rowid_t OneToOneTableEnsureExists(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, std::string_view value, bool overwriteLikeMatch = false);

        // Removes the given row by its rowid if it is no longer referenced.
        void OneToOneTableDeleteIfNotNeededById(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t id);

        // Removes data that is no longer needed for an index that is to be published.
        void OneToOneTablePrepareForPackaging(SQLite::Connection& connection, bool useNamedIndeces, std::string_view tableName);

        // Gets the total number of rows in the table.
        uint64_t OneToOneTableGetCount(SQLite::Connection& connection, std::string_view tableName);

        // Determines if the table is empty.
        bool OneToOneTableIsEmpty(SQLite::Connection& connection, std::string_view tableName);
    }

    // A table that represents a value that is 1:1 with a primary entry.
    template <typename TableInfo>
    struct OneToOneTable
    {
        // The value type.
        using value_t = std::string;

        // The id type
        using id_t = SQLite::rowid_t;

        // Creates the table with named indeces.
        static void Create(SQLite::Connection& connection)
        {
            details::CreateOneToOneTable(connection, true, TableInfo::TableName(), TableInfo::ValueName());
        }

        // Creates the table with standard primary keys.
        static void Create_deprecated(SQLite::Connection& connection)
        {
            details::CreateOneToOneTable(connection, false, TableInfo::TableName(), TableInfo::ValueName());
        }

        // The name of the table.
        static constexpr std::string_view TableName()
        {
            return TableInfo::TableName();
        }

        // The value name of the table.
        static constexpr std::string_view ValueName()
        {
            return TableInfo::ValueName();
        }

        // Value indicating type.
        static constexpr bool IsOneToOne()
        {
            return true;
        }

        // Selects the value from the table, returning the rowid if it exists.
        static std::optional<SQLite::rowid_t> SelectIdByValue(SQLite::Connection& connection, std::string_view value, bool useLike = false)
        {
            return details::OneToOneTableSelectIdByValue(connection, TableInfo::TableName(), TableInfo::ValueName(), value, useLike);
        }

        // Selects the value from the table, returning it if it exists.
        static std::optional<value_t> SelectValueById(SQLite::Connection& connection, id_t id)
        {
            return details::OneToOneTableSelectValueById(connection, TableInfo::TableName(), TableInfo::ValueName(), id);
        }

        // Gets all row ids from the table.
        static std::vector<SQLite::rowid_t> GetAllRowIds(SQLite::Connection& connection, size_t limit = 0)
        {
            return details::OneToOneTableGetAllRowIds(connection, TableInfo::TableName(), TableInfo::ValueName(), limit);
        }

        // Ensures that the given value exists in the table, returning the rowid.
        static SQLite::rowid_t EnsureExists(SQLite::Connection& connection, std::string_view value, bool overwriteLikeMatch = false)
        {
            return details::OneToOneTableEnsureExists(connection, TableInfo::TableName(), TableInfo::ValueName(), value, overwriteLikeMatch);
        }

        // Removes the given row by its rowid if it is no longer referenced.
        static void DeleteIfNotNeededById(SQLite::Connection& connection, SQLite::rowid_t id)
        {
            return details::OneToOneTableDeleteIfNotNeededById(connection, TableInfo::TableName(), TableInfo::ValueName(), id);
        }

        // Removes data that is no longer needed for an index that is to be published.
        static void PrepareForPackaging(SQLite::Connection& connection)
        {
            details::OneToOneTablePrepareForPackaging(connection, true, TableInfo::TableName());
        }

        // Removes data that is no longer needed for an index that is to be published.
        static void PrepareForPackaging_deprecated(SQLite::Connection& connection)
        {
            details::OneToOneTablePrepareForPackaging(connection, false, TableInfo::TableName());
        }

        // Gets the total number of rows in the table.
        static uint64_t GetCount(SQLite::Connection& connection)
        {
            return details::OneToOneTableGetCount(connection, TableInfo::TableName());
        }

        // Determines if the table is empty.
        static bool IsEmpty(SQLite::Connection& connection)
        {
            return details::OneToOneTableIsEmpty(connection, TableInfo::TableName());
        }
    };
}
