// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteVersion.h>
#include "winget/PortableFileEntry.h"
#include <filesystem>

namespace AppInstaller::Repository::Microsoft::Schema
{
    struct IPortableIndex
    {
        virtual ~IPortableIndex() = default;

        // Gets the schema version that this index interface is built for.
        virtual SQLite::Version GetVersion() const = 0;

        // Creates all of the version dependent tables within the database.
        virtual void CreateTable(SQLite::Connection& connection) = 0;

        // Version 1.0
        // Adds a portable file to the index.
        virtual SQLite::rowid_t AddPortableFile(SQLite::Connection& connection, const Portable::PortableFileEntry& file) = 0;

        // Removes a portable file from the index.
        virtual SQLite::rowid_t RemovePortableFile(SQLite::Connection& connection, const Portable::PortableFileEntry& file) = 0;

        // Updates the file with matching FilePath in the index.
        // The return value indicates whether the index was modified by the function.
        virtual std::pair<bool, SQLite::rowid_t> UpdatePortableFile(SQLite::Connection& connection, const Portable::PortableFileEntry& file) = 0;

        // Returns a bool value indicating whether the PortableFile already exists in the index.
        virtual bool Exists(SQLite::Connection& connection, const Portable::PortableFileEntry& file) = 0;

        // Returns a bool value indicating whether the index is empty.
        virtual bool IsEmpty(SQLite::Connection& connection) = 0;

        // Returns a vector including all the portable files recorded in the index.
        virtual std::vector<Portable::PortableFileEntry> GetAllPortableFiles(SQLite::Connection& connection) = 0;
    };
}