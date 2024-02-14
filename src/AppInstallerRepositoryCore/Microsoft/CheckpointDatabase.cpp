// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/CheckpointDatabase.h"
#include "Microsoft/Schema/ICheckpointDatabase.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointDatabaseInterface.h"

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        // Creates the ICheckpointDatabase interface object for the given version.
        std::unique_ptr<Schema::ICheckpointDatabase> CreateICheckpointDatabase(const SQLite::Version& version)
        {
            if (version == SQLite::Version{ 1, 0 } ||
                version.MajorVersion == 1 ||
                version.IsLatest())
            {
                return std::make_unique<Schema::Checkpoint_V1_0::CheckpointDatabaseInterface>();
            }

            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    CheckpointDatabase::CheckpointDatabase(CheckpointDatabase&&) = default;
    CheckpointDatabase& CheckpointDatabase::operator=(CheckpointDatabase&&) = default;

    std::shared_ptr<CheckpointDatabase> CheckpointDatabase::CreateNew(const std::string& filePath, SQLite::Version version)
    {
        AICLI_LOG(Repo, Info, << "Creating new Checkpoint database with version [" << version << "] at '" << filePath << "'");
        CheckpointDatabase result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "CheckpointDatabase_CreateNew");

        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTables(result.m_dbconn);

        result.SetLastWriteTime();

        savepoint.Commit();

        return std::make_shared<CheckpointDatabase>(std::move(result));
    }

    std::shared_ptr<CheckpointDatabase> CheckpointDatabase::Open(const std::string& filePath, SQLite::SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile)
    {
        return std::make_shared<CheckpointDatabase>(CheckpointDatabase{ filePath, disposition, std::move(indexFile) });
    }

    bool CheckpointDatabase::IsEmpty()
    {
        return m_interface->IsEmpty(m_dbconn);
    }

    CheckpointDatabase::IdType CheckpointDatabase::AddCheckpoint(std::string_view checkpointName)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Adding checkpoint [" << checkpointName << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "CheckpointDatabase_addCheckpoint");

        IdType result = m_interface->AddCheckpoint(m_dbconn, checkpointName);

        SetLastWriteTime();
        savepoint.Commit();
        return result;
    }

    std::vector<CheckpointDatabase::IdType> CheckpointDatabase::GetCheckpointIds()
    {
        return m_interface->GetCheckpointIds(m_dbconn);
    }

    bool CheckpointDatabase::HasDataField(IdType checkpointId, int type, const std::string& name)
    {
        return m_interface->GetCheckpointDataFieldValues(m_dbconn, checkpointId, type, name).has_value();
    }

    std::vector<int> CheckpointDatabase::GetDataTypes(IdType checkpointId)
    {
        return m_interface->GetCheckpointDataTypes(m_dbconn, checkpointId);
    }

    std::vector<std::string> CheckpointDatabase::GetDataFieldNames(IdType checkpointId, int dataType)
    {
        return m_interface->GetCheckpointDataFields(m_dbconn, checkpointId, dataType);
    }

    void CheckpointDatabase::SetDataValue(IdType checkpointId, int dataType, const std::string& field, const std::vector<std::string>& values)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Setting checkpoint data [" << dataType << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "CheckpointDatabase_setDataValue");

        m_interface->SetCheckpointDataValues(m_dbconn, checkpointId, dataType, field, values);

        SetLastWriteTime();
        savepoint.Commit();
    }

    void CheckpointDatabase::UpdateDataValue(IdType checkpointId, int dataType, const std::string& field, const std::vector<std::string>& values)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Updating checkpoint data [" << dataType << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "CheckpointDatabase_updateDataValue");

        m_interface->RemoveCheckpointDataType(m_dbconn, checkpointId, dataType);
        m_interface->SetCheckpointDataValues(m_dbconn, checkpointId, dataType, field, values);

        SetLastWriteTime();
        savepoint.Commit();
    }

    void CheckpointDatabase::RemoveDataType(IdType checkpointId, int dataType)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Removing checkpoint data [" << dataType << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "CheckpointDatabase_removeDataValue");

        m_interface->RemoveCheckpointDataType(m_dbconn, checkpointId, dataType);

        SetLastWriteTime();
        savepoint.Commit();
    }

    std::string CheckpointDatabase::GetDataFieldSingleValue(IdType checkpointId, int dataType, const std::string& field)
    {
        const auto& values = m_interface->GetCheckpointDataFieldValues(m_dbconn, checkpointId, dataType, field);

        if (!values.has_value())
        {
            THROW_HR(E_UNEXPECTED);
        }

        return values.value()[0];
    }

    std::vector<std::string> CheckpointDatabase::GetDataFieldMultiValue(IdType checkpointId, int dataType, const std::string& field)
    {
        const auto& values = m_interface->GetCheckpointDataFieldValues(m_dbconn, checkpointId, dataType, field);

        if (!values.has_value())
        {
            THROW_HR(E_UNEXPECTED);
        }

        return values.value();
    }


    CheckpointDatabase::CheckpointDatabase(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile) :
        SQLiteStorageBase(target, disposition, std::move(indexFile))
    {
        AICLI_LOG(Repo, Info, << "Opened Checkpoint Index with version [" << m_version << "], last write [" << GetLastWriteTime() << "]");
        m_interface = CreateICheckpointDatabase(m_version);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, disposition == SQLiteStorageBase::OpenDisposition::ReadWrite && m_version != m_interface->GetVersion());
    }

    CheckpointDatabase::CheckpointDatabase(const std::string& target, SQLite::Version version) : SQLiteStorageBase(target, version)
    {
        m_interface = CreateICheckpointDatabase(m_version);
        m_version = m_interface->GetVersion();
    }
}