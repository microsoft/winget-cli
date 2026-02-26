// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SQLiteIndex.h"
#include <winget/SQLiteStorageBase.h>
#include "ArpVersionValidation.h"
#include <winget/ManifestYamlParser.h>

namespace AppInstaller::Repository::Microsoft
{
    SQLiteIndex SQLiteIndex::CreateNew(const std::string& filePath, SQLite::Version version, CreateOptions options)
    {
        AICLI_LOG(Repo, Info, << "Creating new SQLite Index with version [" << version << "] at '" << filePath << "'");
        SQLiteIndex result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "sqliteindex_createnew");

        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTables(result.m_dbconn, options);

        result.SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    SQLiteIndex SQLiteIndex::Open(const std::string& filePath, OpenDisposition disposition, Utility::ManagedFile&& indexFile)
    {
        return { filePath, disposition, std::move(indexFile) };
    }

    SQLiteIndex SQLiteIndex::CopyFrom(const std::string& filePath, SQLiteIndex& source)
    {
        return { filePath, source };
    }

    SQLiteIndex::SQLiteIndex(const std::string& target, const SQLite::Version& version) : SQLiteStorageBase(target, version)
    {
        m_dbconn.EnableICU();
        m_interface = Schema::CreateISQLiteIndex(version);
        m_version = m_interface->GetVersion();
        SetDatabaseFilePath(target);
    }

    SQLiteIndex::SQLiteIndex(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile) :
        SQLiteStorageBase(target, disposition, std::move(indexFile))
    {
        m_dbconn.EnableICU();
        AICLI_LOG(Repo, Info, << "Opened SQLite Index with version [" << m_version << "], last write [" << GetLastWriteTime() << "]");
        m_interface = Schema::CreateISQLiteIndex(m_version);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, disposition == SQLiteStorageBase::OpenDisposition::ReadWrite && m_version != m_interface->GetVersion());
        SetDatabaseFilePath(target);
    }

    SQLiteIndex::SQLiteIndex(const std::string& target, SQLiteIndex& source) :
        SQLiteStorageBase(target, source)
    {
        m_dbconn.EnableICU();
        m_interface = Schema::CreateISQLiteIndex(m_version);
        SetDatabaseFilePath(target);
    }

    void SQLiteIndex::SetDatabaseFilePath(const std::string& target)
    {
        if (target != SQLITE_MEMORY_DB_CONNECTION_TARGET)
        {
            m_contextData.Add<Schema::Property::DatabaseFilePath>(Utility::ConvertToUTF16(target));
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    void SQLiteIndex::ForceVersion(const SQLite::Version& version)
    {
        m_interface = Schema::CreateISQLiteIndex(version);
    }

    SQLite::Version SQLiteIndex::GetLatestVersion()
    {
        return Schema::CreateISQLiteIndex(SQLite::Version::Latest())->GetVersion();
    }

    const Schema::SQLiteIndexContextData& SQLiteIndex::GetContextData() const
    {
        return m_contextData;
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
        return AddManifestInternalHoldingLock(manifest, relativePath);
    }

    SQLiteIndex::IdType SQLiteIndex::AddManifestInternalHoldingLock(const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
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
        return UpdateManifestInternalHoldingLock(manifest, relativePath);
    }

    bool SQLiteIndex::UpdateManifestInternalHoldingLock(const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
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

    bool SQLiteIndex::AddOrUpdateManifest(const std::filesystem::path& manifestPath, const std::filesystem::path& relativePath)
    {
        AICLI_LOG(Repo, Verbose, << "Adding or Updating manifest from file [" << manifestPath << "]");

        Manifest::Manifest manifest = Manifest::YamlParser::CreateFromPath(manifestPath);
        return AddOrUpdateManifestInternal(manifest, relativePath);
    }

    bool SQLiteIndex::AddOrUpdateManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        return AddOrUpdateManifestInternal(manifest, relativePath);
    }

    bool SQLiteIndex::AddOrUpdateManifest(const Manifest::Manifest& manifest)
    {
        return AddOrUpdateManifestInternal(manifest, {});
    }

    bool SQLiteIndex::AddOrUpdateManifestInternal(const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Adding or Updating manifest for [" << manifest.Id << ", " << manifest.Version << "] at relative path [" << relativePath.value_or("") << "]");

        if (m_interface->GetManifestIdByManifest(m_dbconn, manifest))
        {
            UpdateManifestInternalHoldingLock(manifest, relativePath);
            return false;
        }
        else
        {
            AddManifestInternalHoldingLock(manifest, relativePath);
            return true;
        }
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

        m_interface->PrepareForPackaging(Schema::SQLiteIndexContext{ m_dbconn, m_contextData });
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

    std::optional<std::string> SQLiteIndex::GetPropertyByPrimaryId(IdType primaryId, PackageVersionProperty property) const
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        return m_interface->GetPropertyByPrimaryId(m_dbconn, primaryId, property);
    }

    std::vector<std::string> SQLiteIndex::GetMultiPropertyByPrimaryId(IdType primaryId, PackageVersionMultiProperty property) const
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        return m_interface->GetMultiPropertyByPrimaryId(m_dbconn, primaryId, property);
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

    std::vector<SQLiteIndex::VersionKey> SQLiteIndex::GetVersionKeysById(IdType id) const
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

    bool SQLiteIndex::MigrateTo(SQLite::Version version)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "sqliteindex_migrate_to");

        AICLI_LOG(Repo, Info, << "Attempting to migrate index from [" << m_interface->GetVersion() << "] to [" << version << "]...");
        std::unique_ptr<Schema::ISQLiteIndex> newInterface = Schema::CreateISQLiteIndex(version);

        bool result = newInterface->MigrateFrom(m_dbconn, m_interface.get());

        AICLI_LOG(Repo, Info, << "...migration was " << (result ? "" : "NOT ") << "successful");
        if (result)
        {
            version.SetSchemaVersion(m_dbconn);
            SetLastWriteTime();
            savepoint.Commit();

            m_version = version;
            m_interface = std::move(newInterface);
        }

        return result;
    }

    void SQLiteIndex::SetProperty(Property property, const std::string& value)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };

        switch (property)
        {
        case Property::PackageUpdateTrackingBaseTime:
            m_interface->SetProperty(m_dbconn, Schema::Property::PackageUpdateTrackingBaseTime, value);
            break;
        case Property::IntermediateFileOutputPath:
        {
            std::filesystem::path pathValue{ Utility::ConvertToUTF16(value) };
            THROW_HR_IF(E_INVALIDARG, pathValue.empty() || pathValue.is_relative());
            m_contextData.Add<Schema::Property::IntermediateFileOutputPath>(std::move(pathValue));
        }
            break;
        }
    }
}
