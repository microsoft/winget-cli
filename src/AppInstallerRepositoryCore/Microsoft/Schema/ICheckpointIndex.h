// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/Version.h"

namespace AppInstaller::Repository::Microsoft::Schema
{
    struct ICheckpointIndex
    {
        virtual ~ICheckpointIndex() = default;

        // Gets the schema version that this index interface is built for.
        virtual Schema::Version GetVersion() const = 0;

        // Creates all of the version dependent tables within the database.
        virtual void CreateTable(SQLite::Connection& connection) = 0;

        // Version 1.0
        // Returns a bool value indicating whether the index is empty.
        virtual bool IsEmpty(SQLite::Connection& connection) = 0;
    };
}