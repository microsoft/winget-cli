// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointRecord.h"
#include "Schema/Checkpoint_1_0/CheckpointRecordInterface.h"

namespace AppInstaller::Repository::Microsoft
{
    CheckpointRecord CheckpointRecord::CreateNew(const std::string& filePath, Schema::Version version)
    {
        AICLI_LOG(Repo, Info, << "Creating new Checkpoint Index with version [" << version << "] at '" << filePath << "'");
        CheckpointRecord result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "CheckpointRecord_createnew");

        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTables(result.m_dbconn);

        result.SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    bool CheckpointRecord::IsEmpty()
    {
        return m_interface->IsEmpty(m_dbconn);
    }

    std::optional<CheckpointRecord::IdType> CheckpointRecord::GetCheckpointIdByName(std::string_view checkpointName)
    {
        return m_interface->GetCheckpointIdByName(m_dbconn, checkpointName);
    }

    CheckpointRecord::IdType CheckpointRecord::AddCheckpoint(std::string_view checkpointName)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Adding checkpoint [" << checkpointName << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointrecord_addcheckpoint");

        IdType result = m_interface->AddCheckpoint(m_dbconn, checkpointName);

        SetLastWriteTime();
        savepoint.Commit();
        return result;
    }

    std::vector<std::string> CheckpointRecord::GetCheckpoints()
    {
        return m_interface->GetAvailableCheckpoints(m_dbconn);
    }

    bool CheckpointRecord::HasDataField(IdType checkpointId, int type, std::string name)
    {
        return m_interface->HasCheckpointDataField(m_dbconn, checkpointId, type, name);
    }

    std::vector<int> CheckpointRecord::GetDataTypes(IdType checkpointId)
    {
        return m_interface->GetCheckpointDataTypes(m_dbconn, checkpointId);
    }

    std::vector<std::string> CheckpointRecord::GetDataFieldNames(IdType checkpointId, int dataType)
    {
        return m_interface->GetCheckpointDataFields(m_dbconn, checkpointId, dataType);
    }

    // Set data single value can be reused for all of these methods.

    void CheckpointRecord::SetDataSingleValue(IdType checkpointId, int dataType, std::string value)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Setting checkpoint data [" << dataType << "] with value [" << value << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointrecord_setdatasinglevalue");

        m_interface->SetCheckpointDataValue(m_dbconn, checkpointId, dataType, {}, { value });

        SetLastWriteTime();
        savepoint.Commit();
    }

    std::string CheckpointRecord::GetDataSingleValue(IdType checkpointId, int dataType)
    {
        return m_interface->GetDataSingleValue(m_dbconn, checkpointId, dataType);
    }

    void CheckpointRecord::SetDataFieldSingleValue(IdType checkpointId, int dataType, std::string field, std::string value)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Setting checkpoint data [" << dataType << "] with value [" << value << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointrecord_setdatafieldsinglevalue");

        m_interface->SetCheckpointDataValue(m_dbconn, checkpointId, dataType, field, { value });

        SetLastWriteTime();
        savepoint.Commit();
    }

    std::string CheckpointRecord::GetDataFieldSingleValue(IdType checkpointId, int dataType, std::string_view field)
    {
        return m_interface->GetCheckpointDataValues(m_dbconn, checkpointId, dataType, field);
    }

    std::vector<std::string> CheckpointRecord::GetDataFieldMultiValue(IdType checkpointId, int dataType, std::string field)
    {
        return std::vector<std::string>();
    }

    void CheckpointRecord::SetDataFieldMultiValue(IdType checkpointId, int dataType, std::string field, std::vector<std::string> values)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Setting checkpoint data [" << dataType << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointrecord_setdatafieldmultivalue");

        m_interface->SetCheckpointDataValue(m_dbconn, checkpointId, dataType, field, values);

        SetLastWriteTime();
        savepoint.Commit();
    }

    std::unique_ptr<Schema::ICheckpointRecord> CheckpointRecord::CreateICheckpointRecord() const
    {
        if (m_version == Schema::Version{ 1, 0 } ||
            m_version.MajorVersion == 1 ||
            m_version.IsLatest())
        {
            return std::make_unique<Schema::Checkpoint_V1_0::CheckpointRecordInterface>();
        }

        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }

    CheckpointRecord::CheckpointRecord(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile) :
        SQLiteStorageBase(target, disposition, std::move(indexFile))
    {
        AICLI_LOG(Repo, Info, << "Opened Checkpoint Index with version [" << m_version << "], last write [" << GetLastWriteTime() << "]");
        m_interface = CreateICheckpointRecord();
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, disposition == SQLiteStorageBase::OpenDisposition::ReadWrite && m_version != m_interface->GetVersion());
    }

    CheckpointRecord::CheckpointRecord(const std::string& target, Schema::Version version) : SQLiteStorageBase(target, version)
    {
        m_interface = CreateICheckpointRecord();
        m_version = m_interface->GetVersion();
    }
}