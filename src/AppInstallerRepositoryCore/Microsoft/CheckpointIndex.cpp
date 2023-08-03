// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointIndex.h"
#include "SQLiteStorageBase.h"
#include "Schema/Checkpoint_1_0/CheckpointIndexInterface.h"

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        constexpr std::string_view s_Checkpoints = "Checkpoints"sv;
    }

    CheckpointIndex CheckpointIndex::CreateNew(const std::string& filePath, Schema::Version version)
    {
        AICLI_LOG(Repo, Info, << "Creating new Checkpoint Index with version [" << version << "] at '" << filePath << "'");
        CheckpointIndex result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "CheckpointIndex_createnew");

        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTables(result.m_dbconn);

        result.SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    std::shared_ptr<CheckpointIndex> CheckpointIndex::OpenOrCreateDefault(GUID guid, OpenDisposition openDisposition)
    {
        const auto& indexPath = GetCheckpointIndexPath(guid);
        AICLI_LOG(Repo, Info, << "Opening checkpoint index");

        try
        {
            if (std::filesystem::exists(indexPath))
            {
                if (std::filesystem::is_regular_file(indexPath))
                {
                    try
                    {
                        AICLI_LOG(Repo, Info, << "Opening existing checkpoint index");
                        return std::make_shared<CheckpointIndex>(CheckpointIndex::Open(indexPath.u8string(), openDisposition));
                    }
                    CATCH_LOG();
                }

                AICLI_LOG(Repo, Info, << "Attempting to delete bad checkpoint index file");
                std::filesystem::remove_all(indexPath);
            }

            return std::make_shared<CheckpointIndex>(CheckpointIndex::CreateNew(indexPath.u8string()));
        }
        CATCH_LOG();

        return {};
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    std::optional<std::filesystem::path> s_CheckpointIndexDirectoryOverride{};
    void TestHook_SetCheckpointIndexDirectory_Override(std::optional<std::filesystem::path>&& checkpointIndexDirectory)
    {
        s_CheckpointIndexDirectoryOverride = std::move(checkpointIndexDirectory);
    }
#endif

    std::filesystem::path CheckpointIndex::GetCheckpointIndexPath(GUID guid)
    {
        wchar_t checkpointGuid[256];
        THROW_HR_IF(E_UNEXPECTED, !StringFromGUID2(guid, checkpointGuid, ARRAYSIZE(checkpointGuid)));

        const auto DefaultIndexDirectoryPath = Runtime::GetPathTo(Runtime::PathName::LocalState) / s_Checkpoints;
#ifndef AICLI_DISABLE_TEST_HOOKS
        const auto checkpointIndexDirectory = s_CheckpointIndexDirectoryOverride.has_value() ? s_CheckpointIndexDirectoryOverride.value() : DefaultIndexDirectoryPath;
#else
        const auto checkpointIndexDirectory = DefaultIndexDirectoryPath;
#endif

        if (!std::filesystem::exists(checkpointIndexDirectory))
        {
            std::filesystem::create_directories(checkpointIndexDirectory);
            AICLI_LOG(Repo, Info, << "Creating checkpoint index directory: " << checkpointIndexDirectory);
        }
        else
        {
            THROW_HR_IF(ERROR_CANNOT_MAKE, !std::filesystem::is_directory(checkpointIndexDirectory));
        }

        auto indexPath = checkpointIndexDirectory / checkpointGuid;
        indexPath.replace_extension(".db");
        return indexPath;
    }

    bool CheckpointIndex::IsEmpty()
    {
        return m_interface->IsEmpty(m_dbconn);
    }

    CheckpointIndex::IdType CheckpointIndex::SetClientVersion(std::string_view clientVersion)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Setting client version [" << clientVersion << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_setclientversion");

        IdType result = m_interface->SetClientVersion(m_dbconn, clientVersion);

        SetLastWriteTime();
        savepoint.Commit();
        return result;
    }

    std::string CheckpointIndex::GetClientVersion()
    {
        return m_interface->GetClientVersion(m_dbconn);
    }

    void CheckpointIndex::AddContext(int contextId)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Adding context [" << contextId << "] to checkpoint index");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_addcontext");

        m_interface->AddContextToArgumentTable(m_dbconn, contextId);
        m_interface->AddContextToContextTable(m_dbconn, contextId);

        SetLastWriteTime();
        savepoint.Commit();
    }

    void CheckpointIndex::RemoveContext(int contextId)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Removing context [" << contextId << "] from checkpoint index");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_removecontext");

        m_interface->RemoveContextFromArgumentTable(m_dbconn, contextId);
        m_interface->RemoveContextFromContextTable(m_dbconn, contextId);

        SetLastWriteTime();
        savepoint.Commit();
    }

    CheckpointIndex::IdType CheckpointIndex::SetCommandName(int contextId, std::string_view commandName)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Setting command name [" << commandName << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_setcommandname");

        IdType result = m_interface->SetCommandName(m_dbconn, contextId, commandName);

        SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    std::string CheckpointIndex::GetCommandName(int contextId)
    {
        return m_interface->GetCommandName(m_dbconn, contextId);
    }

    bool CheckpointIndex::UpdateArgument(int contextId, std::string_view name, std::string_view value)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Updating argument column for [" << name << "] with value [" << value << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_updatestringargument");

        bool result = m_interface->UpdateArgumentByContextId(m_dbconn, contextId, name, value);

        SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    bool CheckpointIndex::UpdateArgument(int contextId, std::string_view name, bool value)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Updating argument column for [" << name << "] with value [" << value << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_updateboolargument");

        bool result = m_interface->UpdateArgumentByContextId(m_dbconn, contextId, name, value);

        SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    bool CheckpointIndex::ContainsArgument(int contextId, std::string_view name)
    {
        return m_interface->ContainsArgument(m_dbconn, contextId, name);
    }

    std::string CheckpointIndex::GetStringArgument(int contextId, std::string_view name)
    {
        return m_interface->GetStringArgumentByContextId(m_dbconn, contextId, name);
    }

    bool CheckpointIndex::GetBoolArgument(int contextId, std::string_view name)
    {
        return m_interface->GetBoolArgumentByContextId(m_dbconn, contextId, name);
    }

    int CheckpointIndex::GetFirstContextId()
    {
        return m_interface->GetFirstContextId(m_dbconn);
    }

    bool CheckpointIndex::SetLastCheckpoint(int contextId, int checkpointFlag)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Updating last checkpoint flag for [" << contextId << "] with value [" << checkpointFlag << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_updatelastcheckpointflag");

        bool result = m_interface->SetLastCheckpointByContextId(m_dbconn, contextId, checkpointFlag);

        SetLastWriteTime();
        savepoint.Commit();
        return result;
    }

    int CheckpointIndex::GetLastCheckpoint(int contextId)
    {
        return m_interface->GetLastCheckpointByContextId(m_dbconn, contextId);
    }

    std::unique_ptr<Schema::ICheckpointIndex> CheckpointIndex::CreateICheckpointIndex() const
    {
        if (m_version == Schema::Version{ 1, 0 } ||
            m_version.MajorVersion == 1 ||
            m_version.IsLatest())
        {
            return std::make_unique<Schema::Checkpoint_V1_0::CheckpointIndexInterface>();
        }

        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }

    CheckpointIndex::CheckpointIndex(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile) :
        SQLiteStorageBase(target, disposition, std::move(indexFile))
    {
        AICLI_LOG(Repo, Info, << "Opened Checkpoint Index with version [" << m_version << "], last write [" << GetLastWriteTime() << "]");
        m_interface = CreateICheckpointIndex();
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, disposition == SQLiteStorageBase::OpenDisposition::ReadWrite && m_version != m_interface->GetVersion());
    }

    CheckpointIndex::CheckpointIndex(const std::string& target, Schema::Version version) : SQLiteStorageBase(target, version)
    {
        m_interface = CreateICheckpointIndex();
        m_version = m_interface->GetVersion();
    }
}