// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/IPortableIndex.h"

namespace AppInstaller::Repository::Microsoft::Schema::Portable_V1_0
{
    struct PortableIndexInterface : public IPortableIndex
    {
        // Version 1.0
        Schema::Version GetVersion() const override;
        void CreateTable(SQLite::Connection& connection) override;
        SQLite::rowid_t AddPortableFile(SQLite::Connection& connection, const PortableFile& file) override;
        SQLite::rowid_t RemovePortableFile(SQLite::Connection& connection, const PortableFile& file) override;
        //std::pair<bool, SQLite::rowid_t> UpdatePortableFile(SQLite::Connection& connection, const PortableFile& file) override;
    };
}