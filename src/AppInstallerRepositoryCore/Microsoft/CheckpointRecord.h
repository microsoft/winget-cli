// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/ICheckpointRecord.h"
#include "Microsoft/SQLiteStorageBase.h"
#include <winget/ManagedFile.h>

namespace AppInstaller::Repository::Microsoft
{
    struct CheckpointRecord : SQLiteStorageBase
    {
        // An id that refers to a specific Checkpoint.
        using IdType = SQLite::rowid_t;

        CheckpointRecord(const CheckpointRecord&) = delete;
        CheckpointRecord& operator=(const CheckpointRecord&) = delete;

        CheckpointRecord(CheckpointRecord&&) = default;
        CheckpointRecord& operator=(CheckpointRecord&&) = default;

        // Create a new CheckpointRecord database.
        static CheckpointRecord CreateNew(const std::string& filePath, Schema::Version version = Schema::Version::Latest());

        // Opens an existing CheckpointRecord database.
        static CheckpointRecord Open(const std::string& filePath, OpenDisposition disposition = OpenDisposition::ReadWrite, Utility::ManagedFile&& indexFile = {})
        {
            return { filePath, disposition, std::move(indexFile) };
        }

        // Returns a value indicating whether the record is empty.
        bool IsEmpty();

        // Returns the corresponding id of the checkpoint.
        std::optional<IdType> GetCheckpointIdByName(std::string_view checkpointName);

        // Adds a new checkpoint name to the checkpoint table.
        IdType AddCheckpoint(std::string_view checkpointName);

        // Gets the name of all checkpoints.
        std::vector<std::string> GetCheckpoints();

        // Returns a boolean value indicating a field exists for a checkpoint data type.
        bool HasDataField(IdType checkpointId, int type, std::string name);

        // Returns the available data types for a checkpoint id.
        std::vector<int> GetDataTypes(IdType checkpointId);

        // Returns the available field names for a checkpoint data.
        std::vector<std::string> GetDataFieldNames(IdType checkpointId, int dataType);

        // Sets the value(s) for a data type and field.
        void SetDataValue(IdType checkpointId, int dataType, std::string field, std::vector<std::string> values);

        // Gets a single value for the data type.
        std::string GetDataSingleValue(IdType checkpointId, int dataType);

        // Gets a single value for a data type field.
        std::string GetDataFieldSingleValue(IdType checkpointId, int dataType, std::string_view field);

        // Gets multiple values for a data type field.
        std::vector<std::string> GetDataFieldMultiValue(IdType checkpointId, int dataType, std::string field);

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