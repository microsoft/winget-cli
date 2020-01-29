// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include <filesystem>
#include <string_view>
#include <tuple>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    // A table that represents a single manifest
    struct PathPartTable
    {
        // Creates the table.
        static void Create(SQLite::Connection& connection);

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

        // Removes the path that terminates at the given id.
        // Will not remove a path part if it is referenced.
        static void RemovePathById(SQLite::Connection& connection, SQLite::rowid_t id);
    };
}
