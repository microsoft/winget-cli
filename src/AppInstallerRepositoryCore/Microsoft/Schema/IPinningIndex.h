// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteVersion.h>
#include "winget/Pin.h"

namespace AppInstaller::Repository::Microsoft::Schema
{
    struct IPinningIndex
    {
        virtual ~IPinningIndex() = default;

        // Gets the schema version that this index interface is built for.
        virtual SQLite::Version GetVersion() const = 0;

        // Creates all of the version dependent tables within the database.
        virtual void CreateTables(SQLite::Connection& connection) = 0;

        // Version 1.0
        // Adds a pin to the index.
        virtual SQLite::rowid_t AddPin(SQLite::Connection& connection, const Pinning::Pin& pin) = 0;

        // Updates an existing pin in the index.
        // The return value indicates whether the index was modified by the function.
        virtual std::pair<bool, SQLite::rowid_t> UpdatePin(SQLite::Connection& connection, const Pinning::Pin& pin) = 0;

        // Removes a pin from the index.
        virtual SQLite::rowid_t RemovePin(SQLite::Connection& connection, const Pinning::PinKey& pinKey) = 0;

        // Returns the current pin for a given package if it exists.
        virtual std::optional<Pinning::Pin> GetPin(SQLite::Connection& connection, const Pinning::PinKey& pinKey) = 0;

        // Returns a vector containing all the existing pins.
        virtual std::vector<Pinning::Pin> GetAllPins(SQLite::Connection& connection) = 0;

        // Removes all the pins from a given source, or from all sources if none is specified.
        // Returns a value indicating whether any pin was deleted.
        virtual bool ResetAllPins(SQLite::Connection& connection, std::string_view sourceId) = 0;
    };
}