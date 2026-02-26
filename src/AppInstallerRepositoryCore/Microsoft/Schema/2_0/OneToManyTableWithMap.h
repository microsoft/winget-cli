// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteStatementBuilder.h>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    // Allow the different schema version to indicate which they are.
    enum class OneToManyTableSchema
    {
        // Uses a named unique index for data table.
        // Map table has primary key and no rowid.
        // Column order is consistent with primary key order.
        Version_2_0,
    };

    namespace details
    {
        // Returns the map table name for a given table.
        std::string OneToManyTableGetMapTableName(std::string_view tableName);

        // Returns the primary column name.
        std::string_view OneToManyTableGetManifestColumnName();

        // Create the tables.
        void CreateOneToManyTableWithMap(SQLite::Connection& connection, OneToManyTableSchema schemaVersion, std::string_view tableName, std::string_view valueName);

        // Drops the tables.
        void DropOneToManyTableWithMap(SQLite::Connection& connection, std::string_view tableName);

        // Gets all values associated with the given primary id.
        std::vector<std::string> OneToManyTableWithMapGetValuesByPrimaryId(
            const SQLite::Connection& connection,
            std::string_view tableName,
            std::string_view valueName,
            SQLite::rowid_t primaryId);

        // Ensures that the value exists and inserts mapping entries.
        void OneToManyTableWithMapEnsureExistsAndInsert(SQLite::Connection& connection,
            std::string_view tableName, std::string_view valueName, 
            const std::vector<std::string>& values, SQLite::rowid_t primaryId);

        // Removes data that is no longer needed for an index that is to be published.
        void OneToManyTableWithMapPrepareForPackaging(SQLite::Connection& connection, std::string_view tableName);

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
         bool OneToManyTableWithMapCheckConsistency(const SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, bool log);

        // Determines if the table is empty.
        bool OneToManyTableWithMapIsEmpty(SQLite::Connection& connection, std::string_view tableName);

        // Builds the search select statement base on the given value.
        // The return value is the bind index of the value to match against.
        int OneToManyTableWithMapBuildSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            std::string_view tableName,
            std::string_view valueName,
            std::string_view primaryAlias,
            std::string_view valueAlias,
            bool useLike);
    }

    // A table that represents a value that is 1:N with a primary entry.
    template <typename TableInfo>
    struct OneToManyTableWithMap
    {
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

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection, OneToManyTableSchema schemaVersion)
        {
            details::CreateOneToManyTableWithMap(connection, schemaVersion, TableInfo::TableName(), TableInfo::ValueName());
        }

        // Drops the tables.
        static void Drop(SQLite::Connection& connection)
        {
            details::DropOneToManyTableWithMap(connection, TableInfo::TableName());
        }

        // Gets all values associated with the given primary id.
        static std::vector<std::string> GetValuesByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId)
        {
            return details::OneToManyTableWithMapGetValuesByPrimaryId(connection, TableInfo::TableName(), TableInfo::ValueName(), primaryId);
        }

        // Ensures that all values exist in the data table, and inserts into the mapping table for the given primary id.
        static void EnsureExistsAndInsert(SQLite::Connection& connection, const std::vector<std::string>& values, SQLite::rowid_t primaryId)
        {
            details::OneToManyTableWithMapEnsureExistsAndInsert(connection, TableInfo::TableName(), TableInfo::ValueName(), values, primaryId);
        }

        // Removes data that is no longer needed for an index that is to be published.
        // Preserving the primary index will improve the efficiency of finding the values associated with a primary.
        // Preserving the values index will improve searching when it is primarily done by equality.
        static void PrepareForPackaging(SQLite::Connection& connection)
        {
            details::OneToManyTableWithMapPrepareForPackaging(connection, TableInfo::TableName());
        }

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        static bool CheckConsistency(const SQLite::Connection& connection, bool log)
        {
            return details::OneToManyTableWithMapCheckConsistency(connection, TableInfo::TableName(), TableInfo::ValueName(), log);
        }

        // Determines if the table is empty.
        static bool IsEmpty(SQLite::Connection& connection)
        {
            return details::OneToManyTableWithMapIsEmpty(connection, TableInfo::TableName());
        }

        // Builds the search select statement base on the given value.
        // The return value is the bind index of the value to match against.
        static int BuildSearchStatement(SQLite::Builder::StatementBuilder& builder, std::string_view primaryAlias, std::string_view valueAlias, bool useLike)
        {
            return details::OneToManyTableWithMapBuildSearchStatement(builder, TableInfo::TableName(), TableInfo::ValueName(), primaryAlias, valueAlias, useLike);
        }
    };
}
