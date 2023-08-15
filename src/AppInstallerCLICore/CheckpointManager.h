// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <guiddef.h>
#include "winget/ManifestCommon.h"

namespace AppInstaller::Repository::Microsoft
{
    struct CheckpointIndex;
}

namespace AppInstaller::CLI::Checkpoint
{
    struct CheckpointManager
    {
        // Constructor for an existing resume id.
        CheckpointManager(GUID id);

        // Constructor for a new resume save state.
        CheckpointManager(std::string_view commandName, std::string_view commandArguments, std::string_view clientVersion);
        
        // Gets the client version from the checkpoint index.
        std::string GetClientVersion();

        // Gets the command name from the checkpoint index.
        std::string GetCommandName();

        // Gets the command arguments from the checkpoint index.
        std::string GetArguments();

        // Releases and deletes the checkpoint index.
        void CleanUpIndex();

    private:
        GUID m_checkpointId;
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointIndex> m_checkpointIndex;
    };
}
