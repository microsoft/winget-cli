// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteStatementBuilder.h>
#include "Microsoft/Schema/IPinningIndex.h"
#include "Microsoft/Schema/Pinning_1_0/PinTable.h"
#include <string_view>

namespace AppInstaller::Repository::Microsoft::Schema::Pinning_V1_1
{
    struct PinTable : Pinning_V1_0::PinTable
    {
        // Migrates an existing v1.0 pin table by adding the date_added and note columns.
        static void MigrateFrom1_0(SQLite::Connection& connection);

        // Adds a new pin. Returns the row ID of the added pin.
        static SQLite::rowid_t AddPin(SQLite::Connection& connection, const Pinning::Pin& pin);

        // Updates an existing pin.
        // Returns a value indicating whether there were any changes.
        static bool UpdatePinById(SQLite::Connection& connection, SQLite::rowid_t pinId, const Pinning::Pin& pin);

        // Gets a pin by its row ID if it exists.
        // Used for testing
        static std::optional<Pinning::Pin> GetPinById(SQLite::Connection& connection, const SQLite::rowid_t pinId);

        // Gets all the currently existing pins.
        static std::vector<Pinning::Pin> GetAllPins(SQLite::Connection& connection);
    };
}
