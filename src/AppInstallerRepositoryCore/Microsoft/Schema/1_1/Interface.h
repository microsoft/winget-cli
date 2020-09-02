// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/1_0/Interface.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_1
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public V1_0::Interface
    {
        // Version 1.1
        Schema::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection) override;
        void PrepareForPackaging(SQLite::Connection& connection) override;
    };
}
