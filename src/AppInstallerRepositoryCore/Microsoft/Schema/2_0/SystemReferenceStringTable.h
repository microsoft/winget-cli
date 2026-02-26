// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteStatementBuilder.h>
#include <AppInstallerStrings.h>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    namespace details
    {
        // Returns the primary column name.
        std::string_view SystemReferenceStringTableGetPrimaryColumnName();

        // Create the table.
        void SystemReferenceStringTableCreate(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName);

        // Drops the table.
        void SystemReferenceStringTableDrop(SQLite::Connection& connection, std::string_view tableName);

        // Gets all values associated with the given primary id.
        std::vector<std::string> SystemReferenceStringTableGetValuesByPrimaryId(
            const SQLite::Connection& connection,
            std::string_view tableName,
            std::string_view valueName,
            SQLite::rowid_t primaryId);

        // Ensures that the value exists and inserts mapping entries.
        void SystemReferenceStringTableEnsureExists(
            SQLite::Connection& connection,
            std::string_view tableName,
            std::string_view valueName, 
            const std::vector<std::string>& values,
            SQLite::rowid_t primaryId);

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
         bool SystemReferenceStringTableCheckConsistency(const SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, bool log);

        // Determines if the table is empty.
        bool SystemReferenceStringTableIsEmpty(SQLite::Connection& connection, std::string_view tableName);

        // Builds the search select statement base on the given value.
        // The return value is the bind index of the value to match against.
        int SystemReferenceStringTableBuildSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            std::string_view tableName,
            std::string_view valueName,
            std::string_view primaryAlias,
            std::string_view valueAlias,
            bool useLike);

        // Builds the search select statement base on the given value.
        // The return value is the bind index of the value to match against.
        std::vector<int> SystemReferenceStringTableBuildPairedSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            std::string_view tableName,
            std::string_view valueName,
            std::string_view pairedTableName,
            std::string_view pairedValueName,
            std::string_view primaryAlias,
            std::string_view valueAlias,
            bool useLike);
    }

    // A table that represents a value that is 1:N with a primary entry.
    template <typename TableInfo>
    struct SystemReferenceStringTable
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

        // Creates the table.
        static void Create(SQLite::Connection& connection)
        {
            details::SystemReferenceStringTableCreate(connection, TableInfo::TableName(), TableInfo::ValueName());
        }

        // Drops the table.
        static void Drop(SQLite::Connection& connection)
        {
            details::SystemReferenceStringTableDrop(connection, TableInfo::TableName());
        }

        // Gets all values associated with the given primary id.
        static std::vector<std::string> GetValuesByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId)
        {
            return details::SystemReferenceStringTableGetValuesByPrimaryId(connection, TableInfo::TableName(), TableInfo::ValueName(), primaryId);
        }

        // Ensures that all values exist in the data table, and inserts into the mapping table for the given primary id.
        static void EnsureExists(SQLite::Connection& connection, const std::vector<std::string>& values, SQLite::rowid_t primaryId)
        {
            details::SystemReferenceStringTableEnsureExists(connection, TableInfo::TableName(), TableInfo::ValueName(), values, primaryId);
        }

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        static bool CheckConsistency(const SQLite::Connection& connection, bool log)
        {
            return details::SystemReferenceStringTableCheckConsistency(connection, TableInfo::TableName(), TableInfo::ValueName(), log);
        }

        // Determines if the table is empty.
        static bool IsEmpty(SQLite::Connection& connection)
        {
            return details::SystemReferenceStringTableIsEmpty(connection, TableInfo::TableName());
        }

        // Builds the search select statement base on the given value.
        // The return value is the bind index of the value to match against.
        static int BuildSearchStatement(SQLite::Builder::StatementBuilder& builder, std::string_view primaryAlias, std::string_view valueAlias, bool useLike)
        {
            return details::SystemReferenceStringTableBuildSearchStatement(builder, TableInfo::TableName(), TableInfo::ValueName(), primaryAlias, valueAlias, useLike);
        }

        // Builds the search select statement base on the given value.
        // The return value is the bind index of the value to match against.
        template<typename PairedTable>
        static std::vector<int> BuildPairedSearchStatement(SQLite::Builder::StatementBuilder& builder, std::string_view primaryAlias, std::string_view valueAlias, bool useLike)
        {
            return details::SystemReferenceStringTableBuildPairedSearchStatement(builder, TableInfo::TableName(), TableInfo::ValueName(), PairedTable::TableName(), PairedTable::ValueName(), primaryAlias, valueAlias, useLike);
        }
    };
}
