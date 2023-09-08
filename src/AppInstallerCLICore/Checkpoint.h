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
        // call this here static_assert(dataType)
        // static_assert()

        friend CheckpointManager;

        std::vector<T> GetCheckpointDataTypes()
        {
            return m_checkpointRecord->GetDataTypes(m_checkpointId);
        }

        // Returns a boolean value indicating whether the field name exists.
        bool Has(T dataType, std::string fieldName)
        {
            // to integral wrapper around the data type

            // return a boolean to determine if the context has the field name that exists.
            return m_checkpointRecord->HasDataField(m_checkpointId, dataType, fieldName);
        }

        // Gets all available field names.
        std::vector<std::string> GetFieldNames(T dataType)
        {
            return m_checkpointRecord->GetDataFieldNames(m_checkpointId, dataType);
        }

        std::string GetOne(T dataType)
        {
            return m_checkpointRecord->GetDataSingleValue(m_checkpointId, dataType);
            // For usage with a data types that only have a single file.
        }

        std::vector<std::string> GetMany(T dataType)
        {

        }

        std::vector<std::string> Get(T dataType, std::string fieldName)
        {
            // Dispatch function?
            // Passing in the object you want it to fill and utilize overloads
            return m_checkpointRecord->GetDataFieldValue(m_checkpointId, dataType, fieldName);
        }

        void SetOne(T dataType, std::string value)
        {

        }

        void Set(T dataType, std::string_view fieldName, std::vector<std::string> values)
        {
            m_checkpointRecord->SetDataFieldValue(m_checkpointId, dataType, fieldName, values);
        }

    private: 
        Checkpoint(std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointRecord> checkpointRecord, AppInstaller::Repository::Microsoft::CheckpointRecord::IdType checkpointId) :
            m_checkpointRecord(checkpointRecord), m_checkpointId(checkpointId){};

        AppInstaller::Repository::Microsoft::CheckpointRecord::IdType m_checkpointId;
        std::shared_ptr<CheckpointRecord> m_checkpointRecord;
    };
}