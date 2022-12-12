// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/IPinningIndex.h"

namespace AppInstaller::Repository::Microsoft::Schema::Pinning_V1_0
{
    struct PinningIndexInterface : public IPinningIndex
    {
        // Version 1.0
        Schema::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection) override;

    private:
        SQLite::rowid_t AddPin(SQLite::Connection& connection, const Pinning::Pin& pin) override;

        // Removes a pin from the index.
        SQLite::rowid_t RemovePin(SQLite::Connection& connection, const Pinning::PinKey& pinKey) override;

        // Returns the current pin for a given package if it exists.
        std::optional<Pinning::Pin> GetPin(SQLite::Connection& connection, const Pinning::PinKey& pinKey) override;

        // Returns a vector containing all the existing pins.
        std::vector<Pinning::Pin> GetAllPins(SQLite::Connection& connection) override;
    };
}
