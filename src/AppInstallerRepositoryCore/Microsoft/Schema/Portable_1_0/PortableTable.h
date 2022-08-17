// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"
#include <string_view>
#include <vector>

namespace AppInstaller::Repository::Microsoft::Schema::Portable_V1_0
{
    struct FileColumnInfo
    {
        std::string_view Name;
        SQLite::rowid_t Value;
    };

    enum class FileTypeEnum
    {
        Unknown,
        File,
        Directory,
        Symlink
    };

    struct PortableFile
    {
        std::string FilePath;
        FileTypeEnum FileType = FileTypeEnum::Unknown;
        std::string SHA256;
        std::string SymlinkTarget;
        bool IsCreated = false;
    };

    struct PortableTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        static bool ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id);

        static void DeleteById(SQLite::Connection& connection, SQLite::rowid_t id);

        static bool IsEmpty(SQLite::Connection& connection);

        static std::optional<SQLite::rowid_t> SelectByFilePath(const SQLite::Connection& connection, const std::filesystem::path& path);

        static std::optional<PortableFile> GetPortableFileById(const SQLite::Connection& connection, SQLite::rowid_t id);
        
        static SQLite::rowid_t AddPortableFile(SQLite::Connection& connection, const PortableFile& file);

        static void RemovePortableFileById(SQLite::Connection& connection, SQLite::rowid_t id);

        static bool UpdatePortableFileById(SQLite::Connection& conneciton, SQLite::rowid_t id, const PortableFile& file);
    };
}
