// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Pinning_1_0/PinningIndexInterface.h"
#include "Microsoft/Schema/Pinning_1_0/PinTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::Pinning_V1_0
{
    namespace
    {
        std::optional<SQLite::rowid_t> GetExistingPinId(SQLite::Connection& connection, const Pinning::PinKey& pinKey)
        {
            auto result = PinTable::GetIdByPinKey(connection, pinKey);

            if (!result)
            {
                AICLI_LOG(Repo, Verbose, << "Did not find pin " << pinKey.ToString());
            }

            return result;
        }

    }

    // Version 1.0
    SQLite::Version PinningIndexInterface::GetVersion() const
    {
        return { 1, 0 };
    }

    void PinningIndexInterface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createpintable_v1_0");
        Pinning_V1_0::PinTable::Create(connection);
        savepoint.Commit();
    }

    SQLite::rowid_t PinningIndexInterface::AddPin(SQLite::Connection& connection, const Pinning::Pin& pin)
    {
        auto existingPin = GetExistingPinId(connection, pin.GetKey());

        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), existingPin.has_value());

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addpin_v1_0");
        SQLite::rowid_t pinId = PinTable::AddPin(connection, pin);

        savepoint.Commit();
        return pinId;
    }

    std::pair<bool, SQLite::rowid_t> PinningIndexInterface::UpdatePin(SQLite::Connection& connection, const Pinning::Pin& pin)
    {
        auto existingPinId = GetExistingPinId(connection, pin.GetKey());

        // If the pin doesn't exist, fail the update
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), !existingPinId);


        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatepin_v1_0");
        bool status = PinTable::UpdatePinById(connection, existingPinId.value(), pin);

        savepoint.Commit();
        return { status, existingPinId.value() };
    }

    SQLite::rowid_t PinningIndexInterface::RemovePin(SQLite::Connection& connection, const Pinning::PinKey& pinKey)
    {
        auto existingPinId = GetExistingPinId(connection, pinKey);

        // If the pin doesn't exist, fail the remove
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), !existingPinId);


        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removepin_v1_0");
        PinTable::RemovePinById(connection, existingPinId.value());

        savepoint.Commit();
        return existingPinId.value();
    }

    std::optional<Pinning::Pin> PinningIndexInterface::GetPin(SQLite::Connection& connection, const Pinning::PinKey& pinKey)
    {
        auto existingPinId = GetExistingPinId(connection, pinKey);

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

    bool PinningIndexInterface::ResetAllPins(SQLite::Connection& connection, std::string_view sourceId)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "resetpins_v1_0");
        bool result = PinTable::ResetAllPins(connection, sourceId);
        savepoint.Commit();

        return result;
    }
}