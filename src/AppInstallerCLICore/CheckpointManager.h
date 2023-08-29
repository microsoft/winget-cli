// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <guiddef.h>
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/ICheckpointRecord.h"
#include "Microsoft/SQLiteStorageBase.h"
#include <winget/ManagedFile.h>
#include "ExecutionContextData.h"

using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::SQLite;
using namespace AppInstaller::Repository::Microsoft;

namespace AppInstaller::Checkpoints
{
    // A representation of a single context data (all rows for a single checkpoint and single context data).
    struct CheckpointData
    {
        CheckpointData(int64_t contextDataId) : m_contextDataId(contextDataId) {};

        // Returns a boolean value indicating whether the field name exists.
        bool Has(std::string fieldName)
        {
            return m_values.find(fieldName) != m_values.end();
        }

        // Gets all available field names.
        std::vector<std::string> GetFieldNames()
        {
            std::vector<std::string> fieldNames;
            for (auto it = m_values.begin(); it != m_values.end(); ++it) {
                fieldNames.push_back(it->first);
            }
            return fieldNames;
        }

        std::vector<std::string> Get(std::string fieldName)
        {
            auto it = m_values.find(fieldName);
            return it->second;
        }

        void Set(std::string fieldName, std::vector<std::string> values)
        {
            m_values.insert({ fieldName, values });
        }

        // This will be used to rebuild our data objects later...
        //template<typename T>
        //T Get(std::string fieldName);

        //template<typename T>
        //void Set(std::string fieldName, T data);

    private:
        int64_t m_contextDataId;
        std::map<std::string, std::vector<std::string>> m_values;
    };

    // The other enum would be execution context data.

    // Enum to define the types of checkpoint data
    enum AutomaticCheckpointData
    {
        ClientVersion,
        CommandName,
        Arguments
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

        Checkpoint<AutomaticCheckpointData> GetStartingCheckpoint();

        std::map<std::string, Checkpoint<CLI::Execution::Data>> GetCheckpoints();

    private:
        // Constructor used to open an existing index.
        CheckpointRecord(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile);

        // Constructor used to create a new index.
        CheckpointRecord(const std::string& target, Schema::Version version);

        // Creates the ICheckpointRecord interface object for this version.
        std::unique_ptr<Schema::ICheckpointRecord> CreateICheckpointRecord() const;

        std::unique_ptr<Schema::ICheckpointRecord> m_interface;
    };

    enum class CheckpointNames
    {
        Automatic,
        Installer,
    };

    // A representation of a row in the Checkpoint table. 
    template <typename T>
    struct Checkpoint
    {
        friend CheckpointRecord;

        Checkpoint(std::vector<CheckpointData> checkpointData) : m_checkpointData(checkpointData) {};

        CheckpointData GetData(T type)
        {
            return m_checkpointDataMap[type];
        }

        std::vector<T> GetCheckpointDataTypes()
        {
            std::vector<T> dataTypes;
            for (const auto& data : m_checkpointDataMap)
            {
                dataTypes.emplace_back(data.first);
            }

            return dataTypes;
        }

    private:
        Checkpoint(SQLite::rowid_t id) : m_rowid(id) {};
        SQLite::rowid_t m_rowId;
        std::map<T, CheckpointData> m_checkpointDataMap;
    };
}