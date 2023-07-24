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
        CheckpointManager(GUID checkpointId);

        void InitializeCheckpoint(std::string_view clientVersion, std::string_view commandName);

        void SaveCheckpoint(Execution::Context& context, Execution::CheckpointFlags flag);

        std::unique_ptr<Execution::Context> CreateContextFromCheckpointIndex();

        std::string GetClientVersion();

        std::string GetCommandName();

    private:
        GUID m_checkpointId;
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointIndex> m_checkpointIndex = nullptr;
        void PopulateContextArgsFromCheckpointIndex(Execution::Context& context);
    };
}