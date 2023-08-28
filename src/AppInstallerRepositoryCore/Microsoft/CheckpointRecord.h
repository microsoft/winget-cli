// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/ICheckpointRecord.h"
#include "Microsoft/SQLiteStorageBase.h"
#include <winget/ManagedFile.h>

namespace AppInstaller::Repository::Microsoft
{
    enum AutomaticCheckpointData
    {
        ClientVersion,
        CommandName
    };

    struct CheckpointRecord : SQLiteStorageBase
    {
        // An id that refers to a specific Checkpoint.
        using IdType = SQLite::rowid_t;

        CheckpointRecord(const CheckpointRecord&) = delete;
        CheckpointRecord& operator=(const CheckpointRecord&) = delete;

        CheckpointRecord(CheckpointRecord&&) = default;
        CheckpointRecord& operator=(CheckpointRecord&&) = default;

        // Opens an existing CheckpointRecord database.
        static CheckpointRecord Open(const std::string& filePath, OpenDisposition disposition = OpenDisposition::ReadWrite, Utility::ManagedFile&& indexFile = {})
        {
            return { filePath, disposition, std::move(indexFile) };
        }

        // Create a new CheckpointRecord database.
        static CheckpointRecord CreateNew(const std::string& filePath, Schema::Version version = Schema::Version::Latest());

        // Gets the file path of the CheckpointRecord database.
        static std::filesystem::path GetCheckpointRecordPath(GUID guid);

        // Returns a value indicating whether the record is empty.
        bool IsEmpty();

        Checkpoint<AutomaticCheckpointData> GetAutomaticCheckpoint();

        std::map<std::string, Checkpoint<Execution::ContextData>> GetCheckpoints();


        // Gets all available context data for a checkpoint.
        std::vector<int> GetAvailableData(std::string_view checkpointName);

        // Gets the specified metadata value.
        std::string GetMetadata(CheckpointMetadata checkpointMetadata);

        // Sets the specified metadata value.
        IdType SetMetadata(CheckpointMetadata checkpointMetadata, std::string_view value);

        // Adds a new checkpoint.
        IdType AddCheckpoint(std::string_view checkpointName);

        // Gets the latest checkpoint.
        std::string GetLastCheckpoint();

        // Returns a value indicating whether the checkpoint exists.
        bool CheckpointExists(std::string_view checkpointName);

        // Adds a context data value.
        IdType AddContextData(std::string_view checkpointName, int contextData, std::string_view name, std::string_view value, int index);

        // Gets the context data values.
        std::vector<std::string> GetContextData(std::string_view checkpointName, int contextData);

        // Gets the context data values by property name.
        std::vector<std::string> GetContextDataByName(std::string_view checkpointName, int contextData, std::string_view name);

        // Removes the context data.
        void RemoveContextData(std::string_view checkpointName, int contextData);

    private:
        // Constructor used to open an existing index.
        CheckpointRecord(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile);

        // Constructor used to create a new index.
        CheckpointRecord(const std::string& target, Schema::Version version);

        // Creates the ICheckpointRecord interface object for this version.
        std::unique_ptr<Schema::ICheckpointRecord> CreateICheckpointRecord() const;

        std::unique_ptr<Schema::ICheckpointRecord> m_interface;
    };
}