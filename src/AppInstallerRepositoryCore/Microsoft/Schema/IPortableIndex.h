// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/Version.h"
#include <AppInstallerVersions.h>

namespace AppInstaller::Repository::Microsoft::Schema
{
    // Forward declarations
    struct Version;

    struct IPortableIndex
    {
        virtual ~IPortableIndex() = default;

        // Version 1.0

        // Gets the schema version that this index interface is built for.
        virtual Schema::Version GetVersion() const = 0;

        // Creates all of the version dependent tables within the database.
        virtual void CreateTable(SQLite::Connection& connection) = 0;
    };
}
