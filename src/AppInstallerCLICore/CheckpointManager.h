// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <guiddef.h>
#include "winget/ManifestCommon.h"

namespace AppInstaller::Repository::Microsoft
{
    struct CheckpointRecord;
}

namespace AppInstaller::CLI::Checkpoint
{
    struct CheckpointManager
    {
        CheckpointManager();

        bool IsLoaded() { return m_checkpointRecord ? true: false; };

        void CreateRecord();

        // Loads an existing record from an id.
        void LoadExistingRecord(GUID id);

        // Gets the client version from the checkpoint index.
        std::string GetClientVersion();

        // Gets the command name from the checkpoint index.
        std::string GetCommandName();

        // Gets the command arguments from the checkpoint index.
        std::string GetArguments();

        void SetClientVersion(std::string_view value);

        void SetCommandName(std::string_view value);

        // Adds a context data to the checkpoint record.
        void AddContextData(std::string_view checkpointName, int contextData, std::string_view name, std::string_view value, int index);

        // Releases and deletes the checkpoint index.
        void CleanUpIndex();

    private:
        GUID m_checkpointId;
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointRecord> m_checkpointRecord;
    };
}