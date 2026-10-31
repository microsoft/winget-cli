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

    SQLite::rowid_t PinningIndexInterface::AddPin(SQLite::Connection& connection, const Pinning::Pin& pin)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addpin_v1_1");
        SQLite::rowid_t pinId = Pinning_V1_0::PinningIndexInterface::AddPin(connection, pin);
        THROW_HR_IF(E_UNEXPECTED, !Pinning_V1_1::PinTable::UpdateMetadataById(connection, pinId, pin));

        savepoint.Commit();
        return pinId;
    }

    std::pair<bool, SQLite::rowid_t> PinningIndexInterface::UpdatePin(SQLite::Connection& connection, const Pinning::Pin& pin)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatepin_v1_1");
        auto [baseUpdated, pinId] = Pinning_V1_0::PinningIndexInterface::UpdatePin(connection, pin);
        bool metadataUpdated = PinTable::UpdateMetadataById(connection, pinId, pin);
        savepoint.Commit();

        return { baseUpdated || metadataUpdated, pinId };
    }

    std::optional<Pinning::Pin> PinningIndexInterface::GetPin(SQLite::Connection& connection, const Pinning::PinKey& pinKey)
    {
        auto existingPinId = Pinning_V1_0::PinTable::GetIdByPinKey(connection, pinKey);

        if (!existingPinId)
        {
            return {};
        }

        return PinTable::GetPinById(connection, existingPinId.value());
    }

    std::vector<Pinning::Pin> PinningIndexInterface::GetAllPins(SQLite::Connection& connection)
    {
        return PinTable::GetAllPins(connection);
    }
}
