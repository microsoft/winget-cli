// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/IPortableIndex.h"
#include "Microsoft/Schema/Portable_1_0/PortableTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::Portable_V1_0
{
    struct PortableIndexInterface : public IPortableIndex
    {
        // Version 1.0
        SQLite::Version GetVersion() const override;
        void CreateTable(SQLite::Connection& connection) override;

    private:
        SQLite::rowid_t AddPortableFile(SQLite::Connection& connection, const Portable::PortableFileEntry& file) override;
        SQLite::rowid_t RemovePortableFile(SQLite::Connection& connection, const Portable::PortableFileEntry& file) override;
        std::pair<bool, SQLite::rowid_t> UpdatePortableFile(SQLite::Connection& connection, const Portable::PortableFileEntry& file) override;
        bool Exists(SQLite::Connection& connection, const Portable::PortableFileEntry& file) override;
        bool IsEmpty(SQLite::Connection& connection) override;
        std::vector<Portable::PortableFileEntry> GetAllPortableFiles(SQLite::Connection& connection) override;
    };
}