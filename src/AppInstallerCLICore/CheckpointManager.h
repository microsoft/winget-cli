// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContextData.h"
#include "Checkpoint.h"
#include <guiddef.h>

namespace AppInstaller::Repository::Microsoft
{
    struct CheckpointRecord;
}

namespace AppInstaller::Checkpoints
{
    // Owns the lifetime of a checkpoint data base and creates the checkpoints.
    struct CheckpointManager
    {
        CheckpointManager();
        CheckpointManager(GUID resumeId);

        ~CheckpointManager() = default;

        // Gets the file path of the CheckpointRecord database.
        static std::filesystem::path GetCheckpointRecordPath(GUID guid);

        Checkpoint<CLI::Execution::Data> CreateCheckpoint(std::string_view checkpointName);

        Checkpoint<AutomaticCheckpointData> CreateAutomaticCheckpoint();

        Checkpoint<AutomaticCheckpointData> GetAutomaticCheckpoint();

        std::vector<Checkpoint<CLI::Execution::Data>> GetCheckpoints();

    private:
        GUID m_resumeId = {};
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointRecord> m_checkpointRecord;
    };
}