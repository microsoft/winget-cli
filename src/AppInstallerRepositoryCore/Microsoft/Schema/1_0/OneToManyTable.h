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
        // Returns the map table name for a given table.
        std::string OneToManyTableGetMapTableName(std::string_view tableName);

        // Returns the manifest column name.
        std::string_view OneToManyTableGetManifestColumnName();

        // Create the tables.
        void CreateOneToManyTable(SQLite::Connection& connection, bool useNamedIndeces, std::string_view tableName, std::string_view valueName);

        // Gets all values associated with the given manifest id.
        std::vector<std::string> OneToManyTableGetValuesByManifestId(
            const SQLite::Connection& connection,
            std::string_view tableName,
            std::string_view valueName,
            SQLite::rowid_t manifestId);

        // Ensures that the value exists and inserts mapping entries.
        void OneToManyTableEnsureExistsAndInsert(SQLite::Connection& connection,
            std::string_view tableName, std::string_view valueName, 
            const std::vector<Utility::NormalizedString>& values, SQLite::rowid_t manifestId);

        // Updates the mapping table to represent the given values for the manifest.
        bool OneToManyTableUpdateIfNeededByManifestId(SQLite::Connection& connection,
            std::string_view tableName, std::string_view valueName,
            const std::vector<Utility::NormalizedString>& values, SQLite::rowid_t manifestId);

        // Deletes the mapping rows for the given manifest, then removes any unused data rows.
        void OneToManyTableDeleteIfNotNeededByManifestId(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t manifestId);

        // Removes data that is no longer needed for an index that is to be published.
        void OneToManyTablePrepareForPackaging(SQLite::Connection& connection, std::string_view tableName, bool useNamedIndeces, bool preserveManifestIndex, bool preserveValuesIndex);

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
         bool OneToManyTableCheckConsistency(const SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, bool log);

        // Determines if the table is empty.
        bool OneToManyTableIsEmpty(SQLite::Connection& connection, std::string_view tableName);
    }

    // A table that represents a value that is 1:N with a primary entry.
    template <typename TableInfo>
    struct OneToManyTable
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

        // Value indicating type.
        static constexpr bool IsOneToOne()
        {
            return false;
        }

        // Creates the table with named indeces.
        static void Create(SQLite::Connection& connection)
        {
            details::CreateOneToManyTable(connection, true, TableInfo::TableName(), TableInfo::ValueName());
        }

        // Creates the table with standard primary keys.
        static void Create_deprecated(SQLite::Connection& connection)
        {
            details::CreateOneToManyTable(connection, false, TableInfo::TableName(), TableInfo::ValueName());
        }

        // Gets all values associated with the given manifest id.
        static std::vector<std::string> GetValuesByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId)
        {
            return details::OneToManyTableGetValuesByManifestId(connection, TableInfo::TableName(), TableInfo::ValueName(), manifestId);
        }

        // Ensures that all values exist in the data table, and inserts into the mapping table for the given manifest id.
        static void EnsureExistsAndInsert(SQLite::Connection& connection, const std::vector<Utility::NormalizedString>& values, SQLite::rowid_t manifestId)
        {
            details::OneToManyTableEnsureExistsAndInsert(connection, TableInfo::TableName(), TableInfo::ValueName(), values, manifestId);
        }

        // Updates the mapping table to represent the given values for the manifest.
        static bool UpdateIfNeededByManifestId(SQLite::Connection& connection, const std::vector<Utility::NormalizedString>& values, SQLite::rowid_t manifestId)
        {
            return details::OneToManyTableUpdateIfNeededByManifestId(connection, TableInfo::TableName(), TableInfo::ValueName(), values, manifestId);
        }

        // Deletes the mapping rows for the given manifest, then removes any unused data rows.
        static void DeleteIfNotNeededByManifestId(SQLite::Connection& connection, SQLite::rowid_t manifestId)
        {
            details::OneToManyTableDeleteIfNotNeededByManifestId(connection, TableInfo::TableName(), TableInfo::ValueName(), manifestId);
        }

        // Removes data that is no longer needed for an index that is to be published.
        static void PrepareForPackaging(SQLite::Connection& connection, bool preserveManifestIndex, bool preserveValuesIndex = false)
        {
            details::OneToManyTablePrepareForPackaging(connection, TableInfo::TableName(), true, preserveManifestIndex, preserveValuesIndex);
        }

        // Removes data that is no longer needed for an index that is to be published.
        static void PrepareForPackaging_deprecated(SQLite::Connection& connection)
        {
            details::OneToManyTablePrepareForPackaging(connection, TableInfo::TableName(), false, false, false);
        }

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        static bool CheckConsistency(const SQLite::Connection& connection, bool log)
        {
            return details::OneToManyTableCheckConsistency(connection, TableInfo::TableName(), TableInfo::ValueName(), log);
        }

        // Determines if the table is empty.
        static bool IsEmpty(SQLite::Connection& connection)
        {
            return details::OneToManyTableIsEmpty(connection, TableInfo::TableName());
        }
    };
}
