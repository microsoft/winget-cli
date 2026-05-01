// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Pinning_1_1/PinningIndexInterface.h"
#include "Microsoft/Schema/Pinning_1_1/PinTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::Pinning_V1_1
{
    // Version 1.1
    SQLite::Version PinningIndexInterface::GetVersion() const
    {
        return { 1, 1 };
    }

    void PinningIndexInterface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createpintable_v1_1");
        Pinning_V1_0::PinningIndexInterface base;
        base.CreateTables(connection);
        MigrateFrom(connection, &base);
        savepoint.Commit();
    }

    bool PinningIndexInterface::MigrateFrom(SQLite::Connection& connection, const IPinningIndex* current)
    {
        if (!current || current->GetVersion() != SQLite::Version{ 1, 0 })
        {
            return false;
        }

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "migratepintable_v1_0_to_v1_1");
        Pinning_V1_1::PinTable::MigrateFrom1_0(connection);
        savepoint.Commit();

        return true;
    }

	// Override the pin methods to use the correct PinTable methods for version 1.1

    SQLite::rowid_t PinningIndexInterface::IAddPin(SQLite::Connection& connection, const Pinning::Pin& pin)
    {
        return PinTable::AddPin(connection, pin);
    }

    bool PinningIndexInterface::IUpdatePinById(SQLite::Connection& connection, SQLite::rowid_t pinId, const Pinning::Pin& pin)
    {
        return PinTable::UpdatePinById(connection, pinId, pin);
    }

    std::optional<Pinning::Pin> PinningIndexInterface::IGetPinById(SQLite::Connection& connection, SQLite::rowid_t pinId)
    {
        return PinTable::GetPinById(connection, pinId);
    }

    std::vector<Pinning::Pin> PinningIndexInterface::IGetAllPins(SQLite::Connection& connection)
    {
        return PinTable::GetAllPins(connection);
    }
}
