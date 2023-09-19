// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContextData.h"
#include "ExecutionContext.h"
#include "Public/winget/Checkpoint.h"
#include <guiddef.h>

namespace AppInstaller::Repository::Microsoft
{
    struct CheckpointDatabase;
}

namespace AppInstaller::Checkpoints
{
    // Owns the lifetime of a checkpoint data base and creates the checkpoints.
    struct CheckpointManager
    {
        // Constructor that generates a new resume id and creates the checkpoint database.
        CheckpointManager();

        // Constructor that loads the resume id and opens an existing checkpoint database.
        CheckpointManager(GUID resumeId);

        ~CheckpointManager() = default;

        // Gets the file path of the checkpoint database.
        static std::filesystem::path GetCheckpointDatabasePath(GUID guid, bool createCheckpointDirectory = false);

        // Gets the automatic checkpoint.
        Checkpoint<AutomaticCheckpointData> GetAutomaticCheckpoint();

        // Creates a new automatic checkpoint.
        void CreateAutomaticCheckpoint(CLI::Execution::Context& context);

        // Gets all data checkpoints.
        std::vector<Checkpoint<CLI::Execution::Data>> GetCheckpoints();

        // Creates a new data checkpoint.
        Checkpoint<CLI::Execution::Data> CreateCheckpoint(std::string_view checkpointName);

        // Cleans up the checkpoint database.
        void CleanUpDatabase();

    private:
        GUID m_resumeId = {};
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointDatabase> m_checkpointDatabase;
    };
}