// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ICheckpointIndex.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    struct CheckpointIndexInterface : public ICheckpointIndex
    {
        // Version 1.0
        Schema::Version GetVersion() const override;
        void CreateTable(SQLite::Connection& connection) override;

    private:
        bool IsEmpty(SQLite::Connection& connection) override;
    };
}