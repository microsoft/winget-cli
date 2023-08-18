// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/ICheckpointRecord.h"
#include "Microsoft/SQLiteStorageBase.h"
#include <winget/ManagedFile.h>

namespace AppInstaller::Repository::Microsoft
{
    enum CheckpointMetadata
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
        static CheckpointRecord Open(const std::string& filePath, OpenDisposition disposition, Utility::ManagedFile&& indexFile = {})
        {
            return { filePath, disposition, std::move(indexFile) };
        }

        // Create a new CheckpointRecord database.
        static CheckpointRecord CreateNew(const std::string& filePath, Schema::Version version = Schema::Version::Latest());

        static std::shared_ptr<CheckpointRecord> OpenDefault(GUID guid);

        static std::shared_ptr<CheckpointRecord> CreateDefault(GUID guid);

        // Gets the file path of the CheckpointRecord database.
        static std::filesystem::path GetCheckpointRecordPath(GUID guid);

        bool IsEmpty();

        std::string GetMetadata(CheckpointMetadata checkpointMetadata);

        IdType SetMetadata(CheckpointMetadata checkpointMetadata, std::string_view value);

        IdType AddCheckpoint(std::string_view checkpointName);

        std::optional<IdType> GetCheckpointId(std::string_view checkpointName);

        IdType AddContextData(SQLite::rowid_t checkpointId, int contextData, std::string_view name, std::string_view value, int index);

        std::vector<std::string> GetContextData(SQLite::rowid_t checkpointId, int contextData, std::string_view name);

        void RemoveContextData(SQLite::rowid_t checkpointId, int contextData);

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