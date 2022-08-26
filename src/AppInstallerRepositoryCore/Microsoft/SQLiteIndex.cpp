// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SQLiteIndex.h"
#include "SQLiteStorageBase.h"
#include "ArpVersionValidation.h"
#include <winget/ManifestYamlParser.h>

#include "Schema/1_0/Interface.h"
#include "Schema/1_1/Interface.h"
#include "Schema/1_2/Interface.h"
#include "Schema/1_3/Interface.h"
#include "Schema/1_4/Interface.h"
#include "Schema/1_5/Interface.h"
#include "Schema/1_6/Interface.h"

namespace AppInstaller::Repository::Microsoft
{
    SQLiteIndex SQLiteIndex::CreateNew(const std::string& filePath, Schema::Version version, CreateOptions options)
    {
        AICLI_LOG(Repo, Info, << "Creating new SQLite Index [" << version << "] at '" << filePath << "'");
        SQLiteIndex result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "sqliteindex_createnew");

        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTables(result.m_dbconn, options);

        result.SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    std::unique_ptr<Schema::ISQLiteIndex> SQLiteIndex::CreateISQLiteIndex() const
    {
        using namespace Schema;

        if (m_version == Version{ 1, 0 })
        {
            return std::make_unique<V1_0::Interface>();
        }
        else if (m_version == Version{ 1, 1 })
        {
            return std::make_unique<V1_1::Interface>();
        }
        else if (m_version == Version{ 1, 2 })
        {
            return std::make_unique<V1_2::Interface>();
        }
        else if (m_version == Version{ 1, 3 })
        {
            return std::make_unique<V1_3::Interface>();
        }
        else if (m_version == Version{ 1, 4 })
        {
            return std::make_unique<V1_4::Interface>();
        }
        else if (m_version == Version{ 1, 5 })
        {
            return std::make_unique<V1_5::Interface>();
        }
        else if (m_version == Version{ 1, 6 } ||
            m_version.MajorVersion == 1 ||
            m_version.IsLatest())
        {
            return std::make_unique<V1_6::Interface>();
        }

        // We do not have the capacity to operate on this schema version
        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }

    SQLiteIndex::SQLiteIndex(const std::string& target, Schema::Version version) : SQLiteStorageBase(target, version)
    {
        m_dbconn.EnableICU();
        m_interface = CreateISQLiteIndex();
        m_version = m_interface->GetVersion();
    }

    SQLiteIndex::SQLiteIndex(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile) :
        SQLiteStorageBase(target, disposition, std::move(indexFile))
    {
        m_dbconn.EnableICU();
        AICLI_LOG(Repo, Info, << "Opened SQLite Index with version [" << m_version << "], last write [" << GetLastWriteTime() << "]");
        m_interface = CreateISQLiteIndex();
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, disposition == SQLiteStorageBase::OpenDisposition::ReadWrite && m_version != m_interface->GetVersion());
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    void SQLiteIndex::ForceVersion(const Schema::Version& version)
    {
        m_version = version;
        m_interface = CreateISQLiteIndex();
    }
#endif

    SQLiteIndex::IdType SQLiteIndex::AddManifest(const std::filesystem::path& manifestPath, const std::filesystem::path& relativePath)
    {
        AICLI_LOG(Repo, Verbose, << "Adding manifest from file [" << manifestPath << "]");

        Manifest::Manifest manifest = Manifest::YamlParser::CreateFromPath(manifestPath);
        return AddManifestInternal(manifest, relativePath);
    }

    SQLiteIndex::IdType SQLiteIndex::AddManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        return AddManifestInternal(manifest, relativePath);
    }

    SQLiteIndex::IdType SQLiteIndex::AddManifest(const Manifest::Manifest& manifest)
    {
        return AddManifestInternal(manifest, {});
    }

    SQLiteIndex::IdType SQLiteIndex::AddManifestInternal(const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Adding manifest for [" << manifest.Id << ", " << manifest.Version << "] at relative path [" << relativePath.value_or("") << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "sqliteindex_addmanifest");

        IdType result = m_interface->AddManifest(m_dbconn, manifest, relativePath);

        SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    bool SQLiteIndex::UpdateManifest(const std::filesystem::path& manifestPath, const std::filesystem::path& relativePath)
    {
        AICLI_LOG(Repo, Verbose, << "Updating manifest from file [" << manifestPath << "]");

        Manifest::Manifest manifest = Manifest::YamlParser::CreateFromPath(manifestPath);
        return UpdateManifestInternal(manifest, relativePath);
    }

    bool SQLiteIndex::UpdateManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        return UpdateManifestInternal(manifest, relativePath);
    }

    bool SQLiteIndex::UpdateManifest(const Manifest::Manifest& manifest)
    {
        return UpdateManifestInternal(manifest, {});
    }

    bool SQLiteIndex::UpdateManifestInternal(const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Updating manifest for [" << manifest.Id << ", " << manifest.Version << "] at relative path [" << relativePath.value_or("") << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "sqliteindex_updatemanifest");

        bool result = m_interface->UpdateManifest(m_dbconn, manifest, relativePath).first;

        if (result)
        {
            SetLastWriteTime();

            savepoint.Commit();
        }

        return result;
    }

    void SQLiteIndex::RemoveManifest(const std::filesystem::path& manifestPath, const std::filesystem::path& relativePath)
    {
        AICLI_LOG(Repo, Verbose, << "Removing manifest from file [" << manifestPath << "]");

        Manifest::Manifest manifest = Manifest::YamlParser::CreateFromPath(manifestPath);
        RemoveManifest(manifest, relativePath);
    }

    void SQLiteIndex::RemoveManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        AICLI_LOG(Repo, Verbose, << "Removing manifest for [" << manifest.Id << ", " << manifest.Version << "] at relative path [" << relativePath << "]");
        RemoveManifest(manifest);
    }

    void SQLiteIndex::RemoveManifest(const Manifest::Manifest& manifest)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "sqliteindex_removemanifest");

        m_interface->RemoveManifest(m_dbconn, manifest);

        SetLastWriteTime();

        savepoint.Commit();
    }

    void SQLiteIndex::RemoveManifestById(IdType manifestId)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "SQLiteIndex_RemoveManifestById");

        m_interface->RemoveManifestById(m_dbconn, manifestId);

        SetLastWriteTime();

        savepoint.Commit();
    }

    void SQLiteIndex::PrepareForPackaging()
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Info, << "Preparing index for packaging");

        m_interface->PrepareForPackaging(m_dbconn);
    }

    bool SQLiteIndex::CheckConsistency(bool log) const
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Info, << "Checking index consistency...");

        bool result = m_interface->CheckConsistency(m_dbconn, log);

        AICLI_LOG(Repo, Info, << "...index *WAS" << (result ? "*" : " NOT*") << " consistent.");

        return result;
    }

    Schema::ISQLiteIndex::SearchResult SQLiteIndex::Search(const SearchRequest& request) const
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Performing search: " << request.ToString());

        return m_interface->Search(m_dbconn, request);
    }

    std::optional<std::string> SQLiteIndex::GetPropertyByManifestId(IdType manifestId, PackageVersionProperty property) const
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        return m_interface->GetPropertyByManifestId(m_dbconn, manifestId, property);
    }

    std::vector<std::string> SQLiteIndex::GetMultiPropertyByManifestId(IdType manifestId, PackageVersionMultiProperty property) const
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        return m_interface->GetMultiPropertyByManifestId(m_dbconn, manifestId, property);
    }

    std::optional<SQLiteIndex::IdType> SQLiteIndex::GetManifestIdByKey(IdType id, std::string_view version, std::string_view channel) const
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        return m_interface->GetManifestIdByKey(m_dbconn, id, version, channel);
    }

    std::optional<SQLiteIndex::IdType> SQLiteIndex::GetManifestIdByManifest(const Manifest::Manifest& manifest) const
    {
        return m_interface->GetManifestIdByManifest(m_dbconn, manifest);
    }

    std::vector<Utility::VersionAndChannel> SQLiteIndex::GetVersionKeysById(IdType id) const
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        return m_interface->GetVersionKeysById(m_dbconn, id);
    }

    SQLiteIndex::MetadataResult SQLiteIndex::GetMetadataByManifestId(SQLite::rowid_t manifestId) const
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        return m_interface->GetMetadataByManifestId(m_dbconn, manifestId);
    }

    void SQLiteIndex::SetMetadataByManifestId(IdType manifestId, PackageVersionMetadata metadata, std::string_view value)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        m_interface->SetMetadataByManifestId(m_dbconn, manifestId, metadata, value);
    }

    Utility::NormalizedName SQLiteIndex::NormalizeName(std::string_view name, std::string_view publisher) const
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        return m_interface->NormalizeName(name, publisher);
    }

    std::set<std::pair<SQLite::rowid_t, Utility::NormalizedString>> SQLiteIndex::GetDependenciesByManifestRowId(SQLite::rowid_t manifestRowId) const
    {
        return m_interface->GetDependenciesByManifestRowId(m_dbconn, manifestRowId);
    }

    std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> SQLiteIndex::GetDependentsById(AppInstaller::Manifest::string_t packageId) const
    {
        return m_interface->GetDependentsById(m_dbconn, packageId);
    }
}
