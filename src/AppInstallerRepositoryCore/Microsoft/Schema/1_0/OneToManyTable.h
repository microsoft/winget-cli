// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        // Create the tables.
        void CreateOneToManyTable(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName);

        // Ensures that the value exists and inserts mapping entries.
        void OneToManyTableEnsureExistsAndInsert(SQLite::Connection& connection,
            std::string_view tableName, std::string_view valueName, 
            const std::vector<std::string>& values, SQLite::rowid_t manifestId);

        // Updates the mapping table to represent the given values for the manifest.
        bool OneToManyTableUpdateIfNeededByManifestId(SQLite::Connection& connection,
            std::string_view tableName, std::string_view valueName,
            const std::vector<std::string>& values, SQLite::rowid_t manifestId);

        // Deletes the mapping rows for the given manifest, then removes any unused data rows.
        void OneToManyTableDeleteIfNotNeededByManifestId(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t manifestId);

        // Removes data that is no longer needed for an index that is to be published.
        void OneToManyTablePrepareForPackaging(SQLite::Connection& connection, std::string_view tableName);

        // Determines if the table is empty.
        bool OneToManyTableIsEmpty(SQLite::Connection& connection, std::string_view tableName);
    }

    // A table that represents a value that is 1:N with a primary entry.
    template <typename TableInfo>
    struct OneToManyTable
    {
        // Creates the table.
        static void Create(SQLite::Connection& connection)
        {
            details::CreateOneToManyTable(connection, TableInfo::TableName(), TableInfo::ValueName());
        }

        // Ensures that all values exist in the data table, and inserts into the mapping table for the given manifest id.
        static void EnsureExistsAndInsert(SQLite::Connection& connection, const std::vector<std::string>& values, SQLite::rowid_t manifestId)
        {
            details::OneToManyTableEnsureExistsAndInsert(connection, TableInfo::TableName(), TableInfo::ValueName(), values, manifestId);
        }

        // Updates the mapping table to represent the given values for the manifest.
        static bool UpdateIfNeededByManifestId(SQLite::Connection& connection, const std::vector<std::string>& values, SQLite::rowid_t manifestId)
        {
            return details::OneToManyTableUpdateIfNeededByManifestId(connection, TableInfo::TableName(), TableInfo::ValueName(), values, manifestId);
        }

        // Deletes the mapping rows for the given manifest, then removes any unused data rows.
        static void DeleteIfNotNeededByManifestId(SQLite::Connection& connection, SQLite::rowid_t manifestId)
        {
            details::OneToManyTableDeleteIfNotNeededByManifestId(connection, TableInfo::TableName(), TableInfo::ValueName(), manifestId);
        }

        // Removes data that is no longer needed for an index that is to be published.
        static void PrepareForPackaging(SQLite::Connection& connection)
        {
            details::OneToManyTablePrepareForPackaging(connection, TableInfo::TableName());
        }

        // Determines if the table is empty.
        static bool IsEmpty(SQLite::Connection& connection)
        {
            return details::OneToManyTableIsEmpty(connection, TableInfo::TableName());
        }
    };
}
