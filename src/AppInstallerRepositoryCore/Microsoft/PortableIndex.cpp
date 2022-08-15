// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableIndex.h"
#include "Schema/MetadataTable.h"

namespace AppInstaller::Repository::Microsoft
{
    PortableIndex PortableIndex::CreateNew(const std::string& filePath, Schema::Version version)
    {
        AICLI_LOG(Repo, Info, << "Creating new Portable Index [" << version << "] at '" << filePath << "'");
        PortableIndex result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "portableindex_createnew");

        Schema::MetadataTable::Create(result.m_dbconn);

        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTable(result.m_dbconn);

        //result.SetLastWriteTime();

        savepoint.Commit();

        return result;
    }


    PortableIndex::PortableIndex(const std::string& target, SQLite::Connection::OpenDisposition disposition, SQLite::Connection::OpenFlags flags, Utility::ManagedFile&& indexFile) :
        m_dbconn(SQLite::Connection::Create(target, disposition, flags)), m_indexFile(std::move(indexFile))
    {
        m_dbconn.EnableICU();
        m_version = Schema::Version::GetSchemaVersion(m_dbconn);
        //AICLI_LOG(Repo, Info, << "Opened SQLite Index with version [" << m_version << "], last write [" << GetLastWriteTime() << "]");
        m_interface = m_version.CreateIPortableIndex();
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, disposition == SQLite::Connection::OpenDisposition::ReadWrite && m_version != m_interface->GetVersion());
    }

    PortableIndex::PortableIndex(const std::string& target, Schema::Version version) :
        m_dbconn(SQLite::Connection::Create(target, SQLite::Connection::OpenDisposition::Create))
    {
        m_dbconn.EnableICU();
        m_interface = version.CreateIPortableIndex();
        m_version = m_interface->GetVersion();
    }
}