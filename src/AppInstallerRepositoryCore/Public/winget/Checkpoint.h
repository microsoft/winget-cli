// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/CheckpointDatabase.h>
#include <guiddef.h>

using namespace AppInstaller::Repository::Microsoft;

namespace AppInstaller::Checkpoints
{
    enum AutomaticCheckpointData
    {
        ClientVersion,
        Command,
        Arguments,
        ResumeCount
    };

    struct CheckpointManager;

    // A representation of a row in the Checkpoint table.
    template <typename T>
    struct Checkpoint
    {
        static_assert(std::is_enum<T>::value);
        friend CheckpointManager;

        std::vector<T> GetCheckpointDataTypes()
        {
            return m_checkpointDatabase->GetDataTypes(m_checkpointId);
        }

        // Returns a boolean value indicating whether the field name exists.
        bool Has(T dataType, const std::string& fieldName)
        {
            return m_checkpointDatabase->HasDataField(m_checkpointId, dataType, fieldName);
        }

        // Gets all available field names.
        std::vector<std::string> GetFieldNames(T dataType)
        {
            return m_checkpointDatabase->GetDataFieldNames(m_checkpointId, dataType);
        }

        // Sets a single field value for a data type.
        void Set(T dataType, const std::string& fieldName, const std::string& value)
        {
            m_checkpointDatabase->SetDataValue(m_checkpointId, dataType, fieldName, { value });
        }

        // Sets multiple field values for a data type.
        void SetMany(T dataType, const std::string& fieldName, const std::vector<std::string>& values)
        {
            m_checkpointDatabase->SetDataValue(m_checkpointId, dataType, fieldName, values);
        }

        // Update a single existing field value for a data type.
        void Update(T dataType, const std::string& fieldName, const std::string& value)
        {
            m_checkpointDatabase->UpdateDataValue(m_checkpointId, dataType, fieldName, { value });
        }

        // Gets a single field value for a data type.
        std::string Get(T dataType, const std::string& fieldName)
        {
            return m_checkpointDatabase->GetDataFieldSingleValue(m_checkpointId, dataType, fieldName);
        }

        // Gets multiple field values for a data type.
        std::vector<std::string> GetMany(T dataType, const std::string& fieldName)
        {
            return m_checkpointDatabase->GetDataFieldMultiValue(m_checkpointId, dataType, fieldName);
        }

    private: 
        Checkpoint(std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointDatabase> checkpointDatabase, AppInstaller::Repository::Microsoft::CheckpointDatabase::IdType checkpointId) :
            m_checkpointDatabase(checkpointDatabase), m_checkpointId(checkpointId){};

        AppInstaller::Repository::Microsoft::CheckpointDatabase::IdType m_checkpointId;
        std::shared_ptr<CheckpointDatabase> m_checkpointDatabase;
    };
}