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

    protected:
        SQLite::rowid_t IAddPin(SQLite::Connection& connection, const Pinning::Pin& pin) override;
        bool IUpdatePinById(SQLite::Connection& connection, SQLite::rowid_t pinId, const Pinning::Pin& pin) override;
        std::optional<Pinning::Pin> IGetPinById(SQLite::Connection& connection, SQLite::rowid_t pinId) override;
        std::vector<Pinning::Pin> IGetAllPins(SQLite::Connection& connection) override;
    };
}
