// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_0/Interface.h"

#include "Microsoft/Schema/1_0/IdTable.h"
#include "Microsoft/Schema/1_0/NameTable.h"
#include "Microsoft/Schema/1_0/MonikerTable.h"
#include "Microsoft/Schema/1_0/VersionTable.h"
#include "Microsoft/Schema/1_0/ChannelTable.h"
#include "Microsoft/Schema/1_0/TagsTable.h"
#include "Microsoft/Schema/1_0/CommandsTable.h"
#include "Microsoft/Schema/1_0/ProtocolsTable.h"
#include "Microsoft/Schema/1_0/ExtensionsTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    Schema::Version Interface::GetVersion() const
    {
        return { 1, 0 };
    }

    void Interface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "V1_0_createtables");

        IdTable::Create(connection);
        NameTable::Create(connection);
        MonikerTable::Create(connection);
        VersionTable::Create(connection);
        ChannelTable::Create(connection);
        TagsTable::Create(connection);
        CommandsTable::Create(connection);
        ProtocolsTable::Create(connection);
        ExtensionsTable::Create(connection);

        savepoint.Commit();
    }
}
