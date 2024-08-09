// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteStorageBase.h>
#include <winget/ManagedFile.h>

namespace AppInstaller::Repository::Microsoft
{
    namespace Schema
    {
        struct ICheckpointDatabase;
    }

    struct CheckpointDatabase : SQLite::SQLiteStorageBase
    {
        // An id that refers to a specific Checkpoint.
        using IdType = SQLite::rowid_t;

        CheckpointDatabase(const CheckpointDatabase&) = delete;
        CheckpointDatabase& operator=(const CheckpointDatabase&) = delete;

        CheckpointDatabase(CheckpointDatabase&&);
        CheckpointDatabase& operator=(CheckpointDatabase&&);

        // Create a new checkpoint database.
        static std::shared_ptr<CheckpointDatabase> CreateNew(const std::string& filePath, SQLite::Version version = SQLite::Version::Latest());

        // Opens an existing checkpoint database.
        static std::shared_ptr<CheckpointDatabase> Open(const std::string& filePath, OpenDisposition disposition = OpenDisposition::ReadWrite, Utility::ManagedFile&& indexFile = {});

        // Returns a value indicating whether the database is empty.
        bool IsEmpty();

        // Adds a new checkpoint name to the checkpoint table.
        IdType AddCheckpoint(std::string_view checkpointName);

        // Returns all checkpoint ids in descending (newest at the front) order.
        std::vector<IdType> GetCheckpointIds();

        // Returns a boolean value indicating a field exists for a checkpoint data type.
        bool HasDataField(IdType checkpointId, int type, const std::string& name);

        // Returns the available data types for a checkpoint id.
        std::vector<int> GetDataTypes(IdType checkpointId);

        // Returns the available field names for a checkpoint data.
        std::vector<std::string> GetDataFieldNames(IdType checkpointId, int dataType);

        // Sets the value(s) for a data type and field.
        void SetDataValue(IdType checkpointId, int dataType, const std::string& field, const std::vector<std::string>& values);

        // Updates the value(s) for a data type and field.
        void UpdateDataValue(IdType checkpointId, int dataType, const std::string& field, const std::vector<std::string>& values);

        // Gets a single value for a data type field.
        std::string GetDataFieldSingleValue(IdType checkpointId, int dataType, const std::string& field);

        // Gets multiple values for a data type field.
        std::vector<std::string> GetDataFieldMultiValue(IdType checkpointId, int dataType, const std::string& field);

        // Removes the value(s) for a data type.
        void RemoveDataType(IdType checkpointId, int dataType);

    private:
        // Constructor used to open an existing index.
        CheckpointDatabase(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile);

        // Constructor used to create a new index.
        CheckpointDatabase(const std::string& target, SQLite::Version version);

        std::unique_ptr<Schema::ICheckpointDatabase> m_interface;
    };
}