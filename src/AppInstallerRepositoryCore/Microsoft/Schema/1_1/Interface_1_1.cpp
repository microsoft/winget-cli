// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_1/Interface.h"

#include "Microsoft/Schema/1_0/IdTable.h"
#include "Microsoft/Schema/1_0/NameTable.h"
#include "Microsoft/Schema/1_0/MonikerTable.h"
#include "Microsoft/Schema/1_0/VersionTable.h"
#include "Microsoft/Schema/1_0/ChannelTable.h"

#include "Microsoft/Schema/1_0/PathPartTable.h"

#include "Microsoft/Schema/1_0/ManifestTable.h"

#include "Microsoft/Schema/1_0/TagsTable.h"
#include "Microsoft/Schema/1_0/CommandsTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_1
{
    Schema::Version Interface::GetVersion() const
    {
        return { 1, 1 };
    }

    void Interface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_1");

        V1_0::IdTable::Create(connection);
        V1_0::NameTable::Create(connection);
        V1_0::MonikerTable::Create(connection);
        V1_0::VersionTable::Create(connection);
        V1_0::ChannelTable::Create(connection);

        V1_0::PathPartTable::Create(connection);

        V1_0::ManifestTable::Create(connection, {
            { V1_0::IdTable::ValueName(), true, false }, 
            { V1_0::NameTable::ValueName(), false, false },
            { V1_0::MonikerTable::ValueName(), false, false },
            { V1_0::VersionTable::ValueName(), true, false },
            { V1_0::ChannelTable::ValueName(), true, false },
            { V1_0::PathPartTable::ValueName(), false, true }
            });

        V1_0::TagsTable::Create(connection);
        V1_0::CommandsTable::Create(connection);

        savepoint.Commit();
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v1_1");

        V1_0::IdTable::PrepareForPackaging(connection);
        V1_0::NameTable::PrepareForPackaging(connection);
        V1_0::MonikerTable::PrepareForPackaging(connection);
        V1_0::VersionTable::PrepareForPackaging(connection);
        V1_0::ChannelTable::PrepareForPackaging(connection);

        V1_0::PathPartTable::PrepareForPackaging(connection);

        V1_0::ManifestTable::PrepareForPackaging(connection, {
            V1_0::VersionTable::ValueName(),
            V1_0::ChannelTable::ValueName(),
            V1_0::PathPartTable::ValueName(),
            });

        V1_0::TagsTable::PrepareForPackaging(connection);
        V1_0::CommandsTable::PrepareForPackaging(connection);

        savepoint.Commit();

        // Force the database to actually shrink the file size.
        // This *must* be done outside of an active transaction.
        SQLite::Builder::StatementBuilder builder;
        builder.Vacuum();
        builder.Execute(connection);
    }
}
