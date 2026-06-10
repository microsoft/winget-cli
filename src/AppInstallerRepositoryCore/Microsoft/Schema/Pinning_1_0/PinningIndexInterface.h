// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/IPinningIndex.h"

namespace AppInstaller::Repository::Microsoft::Schema::Pinning_V1_0
{
    struct PinningIndexInterface : public IPinningIndex
    {
        // Version 1.0
        SQLite::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection) override;

    private:
        SQLite::rowid_t AddPin(SQLite::Connection& connection, const Pinning::Pin& pin) override;
        std::pair<bool, SQLite::rowid_t> UpdatePin(SQLite::Connection& connection, const Pinning::Pin& pin) override;
        SQLite::rowid_t RemovePin(SQLite::Connection& connection, const Pinning::PinKey& pinKey) override;
        std::optional<Pinning::Pin> GetPin(SQLite::Connection& connection, const Pinning::PinKey& pinKey) override;
        std::vector<Pinning::Pin> GetAllPins(SQLite::Connection& connection) override;
        bool ResetAllPins(SQLite::Connection& connection, std::string_view sourceId) override;
    };
}
