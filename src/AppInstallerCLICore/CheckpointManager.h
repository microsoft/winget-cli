// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

#include <guiddef.h>

namespace AppInstaller::Repository::Microsoft
{
    struct CheckpointIndex;
}

namespace AppInstaller::CLI::Checkpoint
{
    struct CheckpointManager
    {
        static CheckpointManager& Instance()
        {
            static CheckpointManager s_Instance;
            return s_Instance;
        }

        void Initialize(); // checks if guid is already defined, this means that it has already been loaded.

        void InitializeFromGuid(GUID checkpointId);

        void RemoveContext(Execution::Context& context);

        void SaveCheckpoint(Execution::Context& context, Execution::CheckpointFlags flag);

        void LoadCheckpoint(Execution::Context& context, Execution::CheckpointFlags flag);

        std::unique_ptr<Execution::Context> CreateContextFromCheckpointIndex();

        std::string GetClientVersion();

        std::string GetCommandName();

    private:
        CheckpointManager() = default;
        GUID m_checkpointId = {};
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointIndex> m_checkpointIndex = nullptr;

        void PopulateContextArgsFromIndex(Execution::Context& context);
        void RecordContextArgsToIndex(Execution::Context& context);
    };
}
// 