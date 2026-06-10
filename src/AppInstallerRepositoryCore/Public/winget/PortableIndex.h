// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteStorageBase.h>
#include <winget/PortableFileEntry.h>
#include <winget/ManagedFile.h>

using namespace AppInstaller::Portable;

namespace AppInstaller::Repository::Microsoft
{
    namespace Schema
    {
        struct IPortableIndex;
    }

    struct PortableIndex : SQLite::SQLiteStorageBase
    {
        // An id that refers to a specific portable file.
        using IdType = SQLite::rowid_t;

        PortableIndex(const PortableIndex&) = delete;
        PortableIndex& operator=(const PortableIndex&) = delete;

        PortableIndex(PortableIndex&&);
        PortableIndex& operator=(PortableIndex&&);

        ~PortableIndex();

        // Creates a new PortableIndex database of the given version.
        static PortableIndex CreateNew(const std::string& filePath, SQLite::Version version = SQLite::Version::Latest());

        // Opens an existing PortableIndex database.
        static PortableIndex Open(const std::string& filePath, OpenDisposition disposition, Utility::ManagedFile&& indexFile = {});

        IdType AddPortableFile(const Portable::PortableFileEntry& file);

        void RemovePortableFile(const Portable::PortableFileEntry& file);

        bool UpdatePortableFile(const Portable::PortableFileEntry& file);

        void AddOrUpdatePortableFile(const Portable::PortableFileEntry& file);

        std::vector<Portable::PortableFileEntry> GetAllPortableFiles();

        bool Exists(const Portable::PortableFileEntry& file);

        bool IsEmpty();

    private:
        // Constructor used to open an existing index.
        PortableIndex(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile);

        // Constructor used to create a new index.
        PortableIndex(const std::string& target, SQLite::Version version);

        // Creates the IPortableIndex interface object for this version.
        std::unique_ptr<Schema::IPortableIndex> CreateIPortableIndex() const;

        std::unique_ptr<Schema::IPortableIndex> m_interface;
    };
}