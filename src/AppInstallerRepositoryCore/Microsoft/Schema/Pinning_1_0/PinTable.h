// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/IPinningIndex.h"
#include <string_view>

namespace AppInstaller::Repository::Microsoft::Schema::Pinning_V1_0
{
    // A table 
    struct PinTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        static std::optional<SQLite::rowid_t> SelectByPinKey(SQLite::Connection& connection, const Pinning::PinKey& pinKey);
        static SQLite::rowid_t AddPin(SQLite::Connection& connection, const Pinning::Pin& pin);
        static bool UpdatePin(SQLite::Connection& connection, SQLite::rowid_t pinId, const Pinning::Pin& pin);
        static void RemovePinById(SQLite::Connection& connection, SQLite::rowid_t pinId);
        static std::optional<Pinning::Pin> GetPinById(SQLite::Connection& connection, const SQLite::rowid_t pinId);
        static std::vector<Pinning::Pin> GetAllPins(SQLite::Connection& connection);
        static void ResetAllPins(SQLite::Connection& connection);
    };
}
