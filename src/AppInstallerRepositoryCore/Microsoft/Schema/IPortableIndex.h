// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/Version.h"
#include <filesystem>

namespace AppInstaller::Repository::Microsoft::Schema
{
    struct IPortableIndex
    {
        // File type enum of the portable file
        enum class PortableFileType
        {
            Unknown,
            File,
            Directory,
            Symlink
        };

        // Metadata representation of a portable file placed down during installation
        struct PortableFile
        {
            // Version 1.0
            PortableFileType FileType = PortableFileType::Unknown;
            std::string SHA256;
            std::string SymlinkTarget;

            void SetFilePath(const std::filesystem::path& path) { m_filePath = std::filesystem::absolute(path); };

            std::filesystem::path GetFilePath() const { return m_filePath; };

        private:
            std::filesystem::path m_filePath;
        };

        virtual ~IPortableIndex() = default;

        // Gets the schema version that this index interface is built for.
        virtual Schema::Version GetVersion() const = 0;

        // Creates all of the version dependent tables within the database.
        virtual void CreateTable(SQLite::Connection& connection) = 0;

        // Version 1.0
        // Adds a portable file to the index.
        virtual SQLite::rowid_t AddPortableFile(SQLite::Connection& connection, const PortableFile& file) = 0;

        // Removes a portable file from the index.
        virtual SQLite::rowid_t RemovePortableFile(SQLite::Connection& connection, const PortableFile& file) = 0;

        // Updates the file with matching FilePath in the index.
        // The return value indicates whether the index was modified by the function.
        virtual std::pair<bool, SQLite::rowid_t> UpdatePortableFile(SQLite::Connection& connection, const PortableFile& file) = 0;

        // Returns a bool value indicating whether the PortableFile already exists in the index.
        virtual bool Exists(SQLite::Connection& connection, const PortableFile& file) = 0;

        // Returns a bool value indicating whether the index is empty.
        virtual bool IsEmpty(SQLite::Connection& connection) = 0;

        // Returns a vector including all the portable files recorded in the index.
        virtual std::vector<PortableFile> GetAllPortableFiles(SQLite::Connection& connection) = 0;
    };
}