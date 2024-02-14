// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContextData.h"
#include "ExecutionContext.h"
#include <winget/Checkpoint.h>
#include <guiddef.h>

namespace AppInstaller::Checkpoints
{
    // Reads the command arguments from the automatic checkpoint and populates the context.
    void LoadCommandArgsFromAutomaticCheckpoint(CLI::Execution::Context& context, Checkpoint<AutomaticCheckpointData>& automaticCheckpoint);

    // Owns the lifetime of a checkpoint data base and creates the checkpoints.
    struct CheckpointManager
    {
        // Constructor that generates a new resume id and creates the checkpoint database.
        CheckpointManager();

        // Constructor that loads the resume id and opens an existing checkpoint database.
        CheckpointManager(const std::string& resumeId);

        ~CheckpointManager() = default;

        // Gets the file path of the checkpoint database.
        static std::filesystem::path GetCheckpointDatabasePath(const std::string_view& resumeId, bool createCheckpointDirectory = false);

        // Gets the resume id.
        std::string GetResumeId() { return m_resumeId; };

        // Gets the automatic checkpoint.
        std::optional<Checkpoint<AutomaticCheckpointData>> GetAutomaticCheckpoint();

        // Creates an automatic checkpoint using the provided context.
        void CreateAutomaticCheckpoint(CLI::Execution::Context& context);

        // Gets all context data checkpoints.
        std::vector<Checkpoint<CLI::Execution::Data>> GetCheckpoints();

        // Creates a new context data checkpoint.
        Checkpoint<CLI::Execution::Data> CreateCheckpoint(std::string_view checkpointName);

        // Cleans up the checkpoint database.
        void CleanUpDatabase();

    private:
        std::string m_resumeId;
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointDatabase> m_checkpointDatabase;
    };
}