// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Portable_1_0/PortableIndexInterface.h"
#include "PortableTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::Portable_V1_0
{
    Schema::Version PortableIndexInterface::GetVersion() const
    {
        return { 1, 0 };
    }

    void PortableIndexInterface::CreateTable(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createportableindextable_v1_0");
        Portable_V1_0::PortableTable::Create(connection);
        savepoint.Commit();
    }
}