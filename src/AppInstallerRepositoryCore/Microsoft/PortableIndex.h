// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/IPortableIndex.h"
#include "Microsoft/Schema/Portable_1_0/PortableTable.h"
#include "Microsoft/SQLiteStorageBase.h"
#include <winget/ManagedFile.h>

namespace AppInstaller::Repository::Microsoft
{
    struct PortableIndex : SQLiteStorageBase
    {
        // An id that refers to a specific portable file.
        using IdType = SQLite::rowid_t;

        PortableIndex(const PortableIndex&) = delete;
        PortableIndex& operator=(const PortableIndex&) = delete;

        PortableIndex(PortableIndex&&) = default;
        PortableIndex& operator=(PortableIndex&&) = default;

        // Creates a new PortableIndex database of the given version.
        static PortableIndex CreateNew(const std::string& filePath, Schema::Version version = Schema::Version::Latest());

        // Opens an existing PortableIndex database.
        static PortableIndex Open(const std::string& filePath, OpenDisposition disposition, Utility::ManagedFile&& indexFile = {})
        {
            return { filePath, disposition, std::move(indexFile) };
        }

        IdType AddPortableFile(const Schema::IPortableIndex::PortableFile& file);

        void RemovePortableFile(const Schema::IPortableIndex::PortableFile& file);

        bool UpdatePortableFile(const Schema::IPortableIndex::PortableFile& file);

    protected:
        // Constructor used to open an existing index.
        PortableIndex(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile);

        // Constructor used to create a new index.
        PortableIndex(const std::string& target, Schema::Version version);

        // Creates the IPortableIndex interface object for this version.
        std::unique_ptr<Schema::IPortableIndex> CreateIPortableIndex() const;

        std::unique_ptr<Schema::IPortableIndex> m_interface;
    };
}