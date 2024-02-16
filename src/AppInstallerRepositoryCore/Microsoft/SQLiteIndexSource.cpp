// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/SQLiteIndexSource.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"
#include <winget/ManifestYamlParser.h>


using namespace AppInstaller::Utility;


namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        // The base for the package objects.
        struct SourceReference
        {
            SourceReference(const std::shared_ptr<SQLiteIndexSource>& source) :
                m_source(source) {}

        protected:
            std::shared_ptr<SQLiteIndexSource> GetReferenceSource() const
            {
                std::shared_ptr<SQLiteIndexSource> source = m_source.lock();
                THROW_HR_IF(E_NOT_VALID_STATE, !source);
                return source;
            }

        private:
            std::weak_ptr<SQLiteIndexSource> m_source;
        };

        // The IPackageVersion impl for SQLiteIndexSource.
        struct PackageVersion : public SourceReference, public IPackageVersion
        {
            PackageVersion(const std::shared_ptr<SQLiteIndexSource>& source, SQLiteIndex::IdType manifestId) :
                SourceReference(source), m_manifestId(manifestId) {}

            // Inherited via IPackageVersion
            Utility::LocIndString GetProperty(PackageVersionProperty property) const override
            {
                switch (property)
                {
                case PackageVersionProperty::SourceIdentifier:
                    return LocIndString{ GetReferenceSource()->GetIdentifier() };
                case PackageVersionProperty::SourceName:
                    return LocIndString{ GetReferenceSource()->GetDetails().Name };
                default:
                    // Values coming from the index will always be localized/independent.
                    std::optional<std::string> optValue = GetReferenceSource()->GetIndex().GetPropertyByManifestId(m_manifestId, property);
                    return LocIndString{ optValue ? optValue.value() : std::string{} };
                }
            }

            std::vector<Utility::LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const override
            {
                std::vector<Utility::LocIndString> result;

                for (auto&& value : GetReferenceSource()->GetIndex().GetMultiPropertyByManifestId(m_manifestId, property))
                {
                    // Values coming from the index will always be localized/independent.
                    result.emplace_back(std::move(value));
                }

                return result;
            }

            Manifest::Manifest GetManifest() override
            {
                std::shared_ptr<SQLiteIndexSource> source = GetReferenceSource();

                std::optional<std::string> relativePathOpt = source->GetIndex().GetPropertyByManifestId(m_manifestId, PackageVersionProperty::RelativePath);
                THROW_HR_IF(E_NOT_SET, !relativePathOpt);

                std::optional<std::string> manifestHashString = source->GetIndex().GetPropertyByManifestId(m_manifestId, PackageVersionProperty::ManifestSHA256Hash);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE, source->RequireManifestHash() && !manifestHashString);

                SHA256::HashBuffer manifestSHA256;
                if (manifestHashString)
                {
                    manifestSHA256 = SHA256::ConvertToBytes(manifestHashString.value());
                }

                // Try the primary location 
                HRESULT primaryHR = S_OK;
                try
                {
                    return GetManifestFromArgAndRelativePath(source->GetDetails().Arg, relativePathOpt.value(), manifestSHA256);
                }
                catch (...)
                {
                    if (source->GetDetails().AlternateArg.empty())
                    {
                        throw;
                    }
                    primaryHR = LOG_CAUGHT_EXCEPTION_MSG("GetManifest failed on primary location");
                }

                // Try alternate location
                try
                {
                    return GetManifestFromArgAndRelativePath(source->GetDetails().AlternateArg, relativePathOpt.value(), manifestSHA256);
                }
                CATCH_LOG_MSG("GetManifest failed on alternate location");

                THROW_HR(primaryHR);
            }

            Source GetSource() const override
            {
                return Source{ GetReferenceSource() };
            }

            IPackageVersion::Metadata GetMetadata() const override
            {
                auto metadata = GetReferenceSource()->GetIndex().GetMetadataByManifestId(m_manifestId);

                IPackageVersion::Metadata result;
                for (auto&& data : metadata)
                {
                    result.emplace(std::move(data));
                }

                return result;
            }

        private:
            static Manifest::Manifest GetManifestFromArgAndRelativePath(const std::string& arg, const std::string& relativePath, const SHA256::HashBuffer& expectedHash)
            {
                std::string fullPath = arg;
                if (fullPath.back() != '/')
                {
                    fullPath += '/';
                }
                fullPath += relativePath;

                if (Utility::IsUrlRemote(fullPath))
                {
                    std::ostringstream manifestStream;

                    AICLI_LOG(Repo, Info, << "Downloading manifest");
                    ProgressCallback emptyCallback;

                    constexpr int MaxRetryCount = 2;
                    constexpr std::chrono::seconds maximumWaitTimeAllowed = 10s;
                    for (int retryCount = 0; retryCount < MaxRetryCount; ++retryCount)
                    {
                        try
                        {
                            auto downloadHash = Utility::DownloadToStream(fullPath, manifestStream, Utility::DownloadType::Manifest, emptyCallback, !expectedHash.empty());

                            if (!expectedHash.empty() &&
                                (!downloadHash || downloadHash->size() != expectedHash.size() || !std::equal(expectedHash.begin(), expectedHash.end(), downloadHash->begin())))
                            {
                                THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE);
                            }

                            break;
                        }
                        catch (const ServiceUnavailableException& sue)
                        {
                            if (retryCount < MaxRetryCount - 1)
                            {
                                auto waitSecondsForRetry = sue.RetryAfter();
                                if (waitSecondsForRetry > maximumWaitTimeAllowed)
                                {
                                    throw;
                                }

                                // TODO: Get real progress callback to allow cancelation.
                                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(waitSecondsForRetry);
                                Sleep(static_cast<DWORD>(ms.count()));
                            }
                            else
                            {
                                throw;
                            }
                        }
                        catch (...)
                        {
                            if (retryCount < MaxRetryCount - 1)
                            {
                                AICLI_LOG(Repo, Info, << "Downloading manifest failed, waiting a bit and retrying: " << fullPath);
                                Sleep(500);
                            }
                            else
                            {
                                throw;
                            }
                        }
                    }

                    std::string manifestContents = manifestStream.str();
                    AICLI_LOG(Repo, Verbose, << "Manifest contents: " << manifestContents);

                    return Manifest::YamlParser::Create(manifestContents);
                }
                else
                {
                    AICLI_LOG(Repo, Info, << "Opening manifest from local file: " << fullPath);
                    Manifest::Manifest result = Manifest::YamlParser::CreateFromPath(fullPath);

                    if (!expectedHash.empty() &&
                        (result.StreamSha256.size() != expectedHash.size() || !std::equal(expectedHash.begin(), expectedHash.end(), result.StreamSha256.begin())))
                    {
                        THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE);
                    }

                    return result;
                }
            }

            SQLiteIndex::IdType m_manifestId;
        };

        // The IPackage implementation here.
        struct SQLitePackage : public std::enable_shared_from_this<SQLitePackage>, public SourceReference, public IPackage, public ICompositePackage
        {
            static constexpr IPackageType PackageType = IPackageType::SQLitePackage;

            SQLitePackage(const std::shared_ptr<SQLiteIndexSource>& source, SQLiteIndex::IdType idId, bool isInstalled) :
                SourceReference(source), m_idId(idId), m_isInstalled(isInstalled) {}

            // Inherited via IPackage
            Utility::LocIndString GetProperty(PackageProperty property) const
            {
                Utility::LocIndString result;

                std::shared_ptr<IPackageVersion> truth = GetLatestVersion();
                if (truth)
                {
                    switch (property)
                    {
                    case PackageProperty::Id:
                        return truth->GetProperty(PackageVersionProperty::Id);
                    case PackageProperty::Name:
                        return truth->GetProperty(PackageVersionProperty::Name);
                    default:
                        THROW_HR(E_UNEXPECTED);
                    }
                }
                else
                {
                    AICLI_LOG(Repo, Verbose, << "SQLitePackage: No manifest was found for the package with id# '" << m_idId << "'");
                }

                return result;
            }

            std::vector<PackageVersionKey> GetVersionKeys() const override
            {
                std::shared_ptr<SQLiteIndexSource> source = GetReferenceSource();

                {
                    auto sharedLock = m_versionKeysLock.lock_shared();

                    if (!m_versionKeys.empty())
                    {
                        return m_versionKeys;
                    }
                }

                auto exclusiveLock = m_versionKeysLock.lock_exclusive();

                if (!m_versionKeys.empty())
                {
                    return m_versionKeys;
                }

                std::vector<SQLiteIndex::VersionKey> versions = source->GetIndex().GetVersionKeysById(m_idId);

                for (const auto& vk : versions)
                {
                    std::string version = vk.VersionAndChannel.GetVersion().ToString();
                    std::string channel = vk.VersionAndChannel.GetChannel().ToString();
                    m_versionKeys.emplace_back(source->GetIdentifier(), version, channel);
                    m_versionKeysMap.emplace(MapKey{ std::move(version), std::move(channel) }, vk.ManifestId);
                }

                return m_versionKeys;
            }

            std::shared_ptr<IPackageVersion> GetLatestVersion() const override
            {
                std::shared_ptr<SQLiteIndexSource> source = GetReferenceSource();
                std::optional<SQLiteIndex::IdType> manifestId = source->GetIndex().GetManifestIdByKey(m_idId, {}, {});

                if (manifestId)
                {
                    return std::make_shared<PackageVersion>(source, manifestId.value());
                }

                return {};
            }

            std::shared_ptr<IPackageVersion> GetVersion(const PackageVersionKey& versionKey) const override
            {
                std::shared_ptr<SQLiteIndexSource> source = GetReferenceSource();

                // Ensure that this key targets this (or any) source
                if (!versionKey.SourceId.empty() && versionKey.SourceId != source->GetIdentifier())
                {
                    return {};
                }

                std::optional<SQLiteIndex::IdType> manifestId;

                {
                    MapKey requested{ versionKey.Version, versionKey.Channel };
                    auto sharedLock = m_versionKeysLock.lock_shared();

                    auto itr = m_versionKeysMap.find(requested);
                    if (itr != m_versionKeysMap.end())
                    {
                        manifestId = itr->second;
                    }
                }

                if (!manifestId)
                {
                    manifestId = source->GetIndex().GetManifestIdByKey(m_idId, versionKey.Version, versionKey.Channel);
                }

                if (manifestId)
                {
                    return std::make_shared<PackageVersion>(source, manifestId.value());
                }

                return {};
            }

            Source GetSource() const override
            {
                return Source{ GetReferenceSource() };
            }

            bool IsSame(const IPackage* other) const override
            {
                const SQLitePackage* otherSQLite = PackageCast<const SQLitePackage*>(other);

                if (otherSQLite)
                {
                    return GetReferenceSource()->IsSame(otherSQLite->GetReferenceSource().get()) && m_idId == otherSQLite->m_idId;
                }

                return false;
            }

            const void* CastTo(IPackageType type) const override
            {
                if (type == PackageType)
                {
                    return this;
                }

                return nullptr;
            }

            // Inherited via ICompositePackage
            std::shared_ptr<IPackage> GetInstalled() override
            {
                return m_isInstalled ? shared_from_this() : std::shared_ptr<IPackage>{};
            }

            std::vector<std::shared_ptr<IPackage>> GetAvailable() override
            {
                return m_isInstalled ? std::vector<std::shared_ptr<IPackage>>{} : std::vector<std::shared_ptr<IPackage>>{ shared_from_this() };
            }

        private:
            // Contains the information needed to map a version key to it's rows.
            struct MapKey
            {
                Utility::NormalizedString Version;
                Utility::NormalizedString Channel;

                bool operator<(const MapKey& other) const
                {
                    if (Version < other.Version)
                    {
                        return true;
                    }
                    else if (Version == other.Version)
                    {
                        return Channel < other.Channel;
                    }
                    else
                    {
                        return false;
                    }
                }
            };

            SQLiteIndex::IdType m_idId;
            bool m_isInstalled;

            // To avoid removing const from the interface
            mutable wil::srwlock m_versionKeysLock;
            mutable std::vector<PackageVersionKey> m_versionKeys;
            mutable std::map<MapKey, SQLiteIndex::IdType> m_versionKeysMap;
        };
    }

    SQLiteIndexSource::SQLiteIndexSource(
        const SourceDetails& details,
        SQLiteIndex&& index,
        bool isInstalledSource,
        bool requireManifestHash) :
        m_details(details), m_isInstalled(isInstalledSource), m_index(std::move(index)), m_requireManifestHash(requireManifestHash)
    {
    }

    const SourceDetails& SQLiteIndexSource::GetDetails() const
    {
        return m_details;
    }

    const std::string& SQLiteIndexSource::GetIdentifier() const
    {
        return m_details.Identifier;
    }

    SearchResult SQLiteIndexSource::Search(const SearchRequest& request) const
    {
        auto indexResults = m_index.Search(request);

        SearchResult result;
        std::shared_ptr<SQLiteIndexSource> sharedThis = NonConstSharedFromThis();
        for (auto& indexResult : indexResults.Matches)
        {
            result.Matches.emplace_back(
                std::make_shared<SQLitePackage>(sharedThis, indexResult.first, m_isInstalled),
                std::move(indexResult.second));
        }
        result.Truncated = indexResults.Truncated;
        return result;
    }

    void* SQLiteIndexSource::CastTo(ISourceType type)
    {
        if (type == SourceType)
        {
            return this;
        }

        return nullptr;
    }

    bool SQLiteIndexSource::IsSame(const SQLiteIndexSource* other) const
    {
        return (other && GetIdentifier() == other->GetIdentifier());
    }

    std::shared_ptr<SQLiteIndexSource> SQLiteIndexSource::NonConstSharedFromThis() const
    {
        return const_cast<SQLiteIndexSource*>(this)->shared_from_this();
    }

    SQLiteIndexWriteableSource::SQLiteIndexWriteableSource(const SourceDetails& details, SQLiteIndex&& index, bool isInstalledSource) :
        SQLiteIndexSource(details, std::move(index), isInstalledSource)
    {
    }

    void* SQLiteIndexWriteableSource::CastTo(ISourceType type)
    {
        if (type == ISourceType::IMutablePackageSource)
        {
            return static_cast<IMutablePackageSource*>(this);
        }

        return SQLiteIndexSource::CastTo(type);
    }

    void SQLiteIndexWriteableSource::AddPackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        m_index.AddManifest(manifest, relativePath);
    }
    
    void SQLiteIndexWriteableSource::RemovePackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        m_index.RemoveManifest(manifest, relativePath);
    }
}
