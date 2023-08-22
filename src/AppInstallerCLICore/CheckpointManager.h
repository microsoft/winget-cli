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
        // Returns a bool value indicating whether a record has been loaded.
        bool IsLoaded() { return m_checkpointRecord ? true: false; };

        // Creates a new checkpoint record.
        void CreateRecord();

        // Loads an existing record from an id.
        void LoadRecord(GUID id);

        // Gets a boolean value indicating whether the checkpoint name exists in the record.
        bool Exists(std::string_view checkpointName);

        // Sets the client version.
        void SetClientVersion(std::string_view value);

        // Gets the client version from the checkpoint record.
        std::string GetClientVersion();

        // Sets the command name.
        void SetCommandName(std::string_view value);

        // Gets the command name from the checkpoint record.
        std::string GetCommandName();

        // Gets the latest checkpoint.
        std::string GetLastCheckpoint();

        // Gets the available context data items for a given checkpoint.
        std::vector<int> GetAvailableContextData(std::string_view checkpointName);

        // Adds a context data to the checkpoint record.
        void AddContextData(std::string_view checkpointName, int contextData, std::string_view name, std::string_view value, int record);

        // Gets the values associated with a context data
        std::vector<std::string> GetContextData(std::string_view checkpointName, int contextData);

        // Gets the values by property name associated with a context data.
        std::vector<std::string> GetContextDataByName(std::string_view checkpointName, int contextData, std::string_view name);

        // Releases and deletes the checkpoint record.
        void DeleteRecord();

    private:
        GUID m_checkpointId = {};
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointRecord> m_checkpointRecord;
    };
}