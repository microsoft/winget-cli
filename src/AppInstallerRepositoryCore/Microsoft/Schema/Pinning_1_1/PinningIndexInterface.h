// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/Pinning_1_0/PinningIndexInterface.h"

namespace AppInstaller::Repository::Microsoft::Schema::Pinning_V1_1
{
    struct PinningIndexInterface : public Pinning_V1_0::PinningIndexInterface
    {
        SQLite::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection) override;
        bool MigrateFrom(SQLite::Connection& connection, const IPinningIndex* current) override;
        SQLite::rowid_t AddPin(SQLite::Connection& connection, const Pinning::Pin& pin) override;
        std::pair<bool, SQLite::rowid_t> UpdatePin(SQLite::Connection& connection, const Pinning::Pin& pin) override;
        std::optional<Pinning::Pin> GetPin(SQLite::Connection& connection, const Pinning::PinKey& pinKey) override;
        std::vector<Pinning::Pin> GetAllPins(SQLite::Connection& connection) override;
    };
}
