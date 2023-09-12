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
        // Constructor that generates a new resume id and creates the checkpoint record.
        CheckpointManager();

        // Constructor that loads the resume id and opens an existing checkpoint record.
        CheckpointManager(GUID resumeId);

        ~CheckpointManager() = default;

        // Gets the file path of the checkpoint record.
        static std::filesystem::path GetCheckpointRecordPath(GUID guid);

        // Gets the automatic checkpoint.
        std::optional<Checkpoint<AutomaticCheckpointData>> GetAutomaticCheckpoint();

        // Creates a new automatic checkpoint.
        Checkpoint<AutomaticCheckpointData> CreateAutomaticCheckpoint();

        // Gets all data checkpoints.
        std::vector<Checkpoint<CLI::Execution::Data>> GetCheckpoints();

        // Creates a new data checkpoint.
        Checkpoint<CLI::Execution::Data> CreateCheckpoint(std::string_view checkpointName);

        // Cleans up the checkpoint record.
        void CleanUpRecord();

    private:
        GUID m_resumeId = {};
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointRecord> m_checkpointRecord;
    };
}