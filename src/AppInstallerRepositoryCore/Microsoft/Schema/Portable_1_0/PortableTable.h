// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteStatementBuilder.h>
#include "Microsoft/Schema/IPortableIndex.h"
#include <string_view>

namespace AppInstaller::Repository::Microsoft::Schema::Portable_V1_0
{
    // A table the represents a single portable file
    struct PortableTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        // Gets a value indicating whether the portable file with rowid id exists.
        static bool ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id);

        // Deletes the portable file row with the given rowid
        static void DeleteById(SQLite::Connection& connection, SQLite::rowid_t id);

        // Gets a value indicating whether the table is empty.
        static bool IsEmpty(SQLite::Connection& connection);

        // Selects the portable file by filepath from the table, returning the rowid if it exists.
        static std::optional<SQLite::rowid_t> SelectByFilePath(const SQLite::Connection& connection, const std::filesystem::path& path);

        // Selects the portable file by rowid from the table, returning the portable file object if it exists.
        static std::optional<Portable::PortableFileEntry> GetPortableFileById(const SQLite::Connection& connection, SQLite::rowid_t id);
        
        // Adds the portable file into the table.
        static SQLite::rowid_t AddPortableFile(SQLite::Connection& connection, const Portable::PortableFileEntry& file);

        // Removes the portable file from the table by id.
        static void RemovePortableFileById(SQLite::Connection& connection, SQLite::rowid_t id);

        // Updates the portable file in the table by id.
        static bool UpdatePortableFileById(SQLite::Connection& connection, SQLite::rowid_t id, const Portable::PortableFileEntry& file);

        // Gets all portable files recorded in the index.
        static std::vector<Portable::PortableFileEntry> GetAllPortableFiles(SQLite::Connection& connection);
    };
}