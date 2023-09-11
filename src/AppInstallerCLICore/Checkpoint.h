// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContextData.h"
#include "Microsoft/CheckpointRecord.h"
#include <guiddef.h>

using namespace AppInstaller::Repository::Microsoft;

namespace AppInstaller::Checkpoints
{
    enum AutomaticCheckpointData
    {
        ClientVersion,
        CommandName,
        Arguments
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
            return m_checkpointRecord->GetDataTypes(m_checkpointId);
        }

        // Returns a boolean value indicating whether the field name exists.
        bool Has(T dataType, std::string fieldName)
        {
            return m_checkpointRecord->HasDataField(m_checkpointId, dataType, fieldName);
        }

        // Gets all available field names.
        std::vector<std::string> GetFieldNames(T dataType)
        {
            return m_checkpointRecord->GetDataFieldNames(m_checkpointId, dataType);
        }
        
        // Sets a single value for the a data type.
        void Set(T dataType, std::string value)
        {
            m_checkpointRecord->SetDataValue(m_checkpointId, dataType, {}, { value });
        }

        // Sets a single field value for a data type.
        void Set(T dataType, std::string fieldName, std::string value)
        {
            m_checkpointRecord->SetDataValue(m_checkpointId, dataType, fieldName, { value });
        }

        // Sets multiple field values for a data type.
        void SetMany(T dataType, std::string fieldName, std::vector<std::string> values)
        {
            m_checkpointRecord->SetDataValue(m_checkpointId, dataType, fieldName, values);
        }

        // Gets a single value for a data type.
        std::string Get(T dataType)
        {
            return m_checkpointRecord->GetDataSingleValue(m_checkpointId, dataType);
        }

        // Gets a single field value for a data type.
        std::string Get(T dataType, std::string fieldName)
        {
            return m_checkpointRecord->GetDataFieldSingleValue(m_checkpointId, dataType, fieldName);
        }

        // Gets multiple field values for a data type.
        std::vector<std::string> GetMany(T dataType, std::string fieldName)
        {
            return m_checkpointRecord->GetDataFieldMultiValue(m_checkpointId, dataType, fieldName);
        }

    private: 
        Checkpoint(std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointRecord> checkpointRecord, AppInstaller::Repository::Microsoft::CheckpointRecord::IdType checkpointId) :
            m_checkpointRecord(checkpointRecord), m_checkpointId(checkpointId){};

        AppInstaller::Repository::Microsoft::CheckpointRecord::IdType m_checkpointId;
        std::shared_ptr<CheckpointRecord> m_checkpointRecord;
    };
}