// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    // A table that represents the parts of a path
    struct PathPartTable
    {
        // The id type
        using id_t = SQLite::rowid_t;

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        // Creates the table with standard primary keys.
        static void Create_deprecated(SQLite::Connection& connection);

        // Gets the table name.
        static std::string_view TableName();

        // Gets the value name.
        static std::string_view ValueName();

        // Ensure that the given relative path exists within the path parts table.
        // If createIfNotFound is true, the function will add the parts as needed.
        //      The result bool will indicate whether it was necessary to add the path (true),
        //      or it was already present (false).
        // If createIfNotFound is false, the function will simply determine if the path is present.
        //      The result bool will indicate whether the path was found (true), or not (false).
        // In all cases except createIfNotFound == false and result bool == false, the int64_t value
        // will be valid and the rowid of the final path part in the path.
        static std::tuple<bool, SQLite::rowid_t> EnsurePathExists(SQLite::Connection& connection, const std::filesystem::path& relativePath, bool createIfNotFound);

        // Gets the path string using the given id as the leaf.
        static std::optional<std::string> GetPathById(const SQLite::Connection& connection, SQLite::rowid_t id);

        // Removes the path that terminates at the given id.
        // Will not remove a path part if it is referenced.
        static void RemovePathById(SQLite::Connection& connection, SQLite::rowid_t id);

        // Removes data that is no longer needed for an index that is to be published.
        static void PrepareForPackaging(SQLite::Connection& connection);

        // Removes data that is no longer needed for an index that is to be published.
        static void PrepareForPackaging_deprecated(SQLite::Connection& connection);

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        static bool CheckConsistency(const SQLite::Connection& connection, bool log);

        // Determines if the table is empty.
        static bool IsEmpty(SQLite::Connection& connection);
    };
}
