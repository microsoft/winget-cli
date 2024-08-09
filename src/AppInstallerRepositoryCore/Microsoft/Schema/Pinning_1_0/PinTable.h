// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteStatementBuilder.h>
#include "Microsoft/Schema/IPinningIndex.h"
#include <string_view>

namespace AppInstaller::Repository::Microsoft::Schema::Pinning_V1_0
{
    struct PinTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        // Gets the row ID for the pin, if it exists.
        static std::optional<SQLite::rowid_t> GetIdByPinKey(SQLite::Connection& connection, const Pinning::PinKey& pinKey);

        // Adds a new pin. Returns the row ID of the added pin.
        static SQLite::rowid_t AddPin(SQLite::Connection& connection, const Pinning::Pin& pin);

        // Updates an existing pin.
        // Returns a value indicating whether there were any changes.
        static bool UpdatePinById(SQLite::Connection& connection, SQLite::rowid_t pinId, const Pinning::Pin& pin);

        // Removes a pin given its row ID.
        static void RemovePinById(SQLite::Connection& connection, SQLite::rowid_t pinId);

        // Gets a pin by its row ID if it exists.
        // Used for testing
        static std::optional<Pinning::Pin> GetPinById(SQLite::Connection& connection, const SQLite::rowid_t pinId);

        // Gets all the currently existing pins.
        static std::vector<Pinning::Pin> GetAllPins(SQLite::Connection& connection);

        // Resets all pins from a given source, or from all sources if none is specified.
        // Returns a value indicating whether there were any changes.
        static bool ResetAllPins(SQLite::Connection& connection, std::string_view sourceId = {});
    };
}
