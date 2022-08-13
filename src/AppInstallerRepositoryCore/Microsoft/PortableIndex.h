// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/IPortableIndex.h"
#include <winget/ManagedFile.h>

#include <mutex>
#include <chrono>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace AppInstaller::Repository::Microsoft
{
    struct PortableIndex
    {
        PortableIndex(const PortableIndex&) = delete;
        PortableIndex& operator=(const PortableIndex&) = delete;

        PortableIndex(PortableIndex&&) = default;
        PortableIndex& operator=(PortableIndex&&) = default;

        // Creates a new index database of the given version.
        static PortableIndex CreateNew(const std::string& filePath, Schema::Version version = Schema::Version::Latest());

        // Gets the schema version of the index.
        Schema::Version GetVersion() const { return m_version; }

    private:
        // Constructor used to open an existing index.
        PortableIndex(const std::string& target, SQLite::Connection::OpenDisposition disposition, SQLite::Connection::OpenFlags flags, Utility::ManagedFile&& indexFile);

        // Constructor used to create a new index.
        PortableIndex(const std::string& target, Schema::Version version);

        Utility::ManagedFile m_indexFile;
        SQLite::Connection m_dbconn;
        Schema::Version m_version;
        std::unique_ptr<Schema::IPortableIndex> m_interface;
        std::unique_ptr<std::mutex> m_interfaceLock = std::make_unique<std::mutex>();
    };
}