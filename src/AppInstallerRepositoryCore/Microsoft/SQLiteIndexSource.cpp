// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/SQLiteIndexSource.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"
#include <winget/ManifestYamlParser.h>
#include <winget/PackageVersionDataManifest.h>

using namespace AppInstaller::Utility;


namespace AppInstaller::Repository::Microsoft
{
    namespace anon
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

        namespace V1
        {
            // The IPackageVersion implementation for V1 index.
            struct PackageVersion : public SourceReference, public IPackageVersion
            {
                PackageVersion(const std::shared_ptr<SQLiteIndexSource>& source, SQLiteIndex::IdType manifestId, const std::shared_ptr<Caching::FileCache>& manifestCache) :
                    SourceReference(source), m_manifestId(manifestId), m_manifestCache(manifestCache) {}

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
                        std::optional<std::string> optValue = GetReferenceSource()->GetIndex().GetPropertyByPrimaryId(m_manifestId, property);
                        return LocIndString{ optValue ? optValue.value() : std::string{} };
                    }
                }

                std::vector<Utility::LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const override
                {
                    std::vector<Utility::LocIndString> result;

                    for (auto&& value : GetReferenceSource()->GetIndex().GetMultiPropertyByPrimaryId(m_manifestId, property))
                    {
                        // Values coming from the index will always be localized/independent.
                        result.emplace_back(std::move(value));
                    }

                    return result;
                }

                Manifest::Manifest GetManifest() override
                {
                    std::shared_ptr<SQLiteIndexSource> source = GetReferenceSource();

                    std::optional<std::string> relativePathOpt = source->GetIndex().GetPropertyByPrimaryId(m_manifestId, PackageVersionProperty::RelativePath);
                    THROW_HR_IF(E_NOT_SET, !relativePathOpt);

                    std::optional<std::string> manifestHashString = source->GetIndex().GetPropertyByPrimaryId(m_manifestId, PackageVersionProperty::ManifestSHA256Hash);
                    THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE, source->RequireManifestHash() && !manifestHashString);

                    SHA256::HashBuffer manifestSHA256;
                    if (manifestHashString)
                    {
                        manifestSHA256 = SHA256::ConvertToBytes(manifestHashString.value());
                    }

                    std::unique_ptr<std::istream> manifestStream = m_manifestCache->GetFile(relativePathOpt.value(), manifestSHA256);
                    return Manifest::YamlParser::Create(Utility::ReadEntireStream(*manifestStream));
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
                SQLiteIndex::IdType m_manifestId;
                std::shared_ptr<Caching::FileCache> m_manifestCache;
            };

            // The IPackage implementation for V1 index.
            struct SQLitePackage : public std::enable_shared_from_this<SQLitePackage>, public SourceReference, public IPackage, public ICompositePackage
            {
                static constexpr IPackageType PackageType = IPackageType::SQLitePackage1;

                SQLitePackage(const std::shared_ptr<SQLiteIndexSource>& source, SQLiteIndex::IdType idId, const std::shared_ptr<Caching::FileCache>& manifestCache, bool isInstalled) :
                    SourceReference(source), m_idId(idId), m_manifestCache(manifestCache), m_isInstalled(isInstalled) {}

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
                        return std::make_shared<PackageVersion>(source, manifestId.value(), m_manifestCache);
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
                        return std::make_shared<PackageVersion>(source, manifestId.value(), m_manifestCache);
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
                std::shared_ptr<Caching::FileCache> m_manifestCache;
                bool m_isInstalled;

                // To avoid removing const from the interface
                mutable wil::srwlock m_versionKeysLock;
                mutable std::vector<PackageVersionKey> m_versionKeys;
                mutable std::map<MapKey, SQLiteIndex::IdType> m_versionKeysMap;
            };
        }

        namespace V2
        {
            // Get the relative path and hash for the package version data manifest.
            std::pair<std::string, std::string> CreatePackageVersionDataRelativePath(const std::shared_ptr<SQLiteIndexSource>& source, SQLiteIndex::IdType packageRowId)
            {
                static constexpr std::string_view s_fixedPathPart = "packages/";

                const SQLiteIndex& index = source->GetIndex();

                std::string hash = index.GetPropertyByPrimaryId(packageRowId, PackageVersionProperty::ManifestSHA256Hash).value();

                // See PrepareForPackaging in the V2 interface for this format.
                std::ostringstream stream;
                stream <<
                    s_fixedPathPart <<
                    index.GetPropertyByPrimaryId(packageRowId, PackageVersionProperty::Id).value() << '/' <<
                    hash << '/' <<
                    Manifest::PackageVersionDataManifest::VersionManifestCompressedFileName();

                return std::make_pair(std::move(stream).str(), std::move(hash));
            }

            // Gets package version data for the given package in the index.
            Manifest::PackageVersionDataManifest GetPackageVersionData(const std::shared_ptr<SQLiteIndexSource>& source, SQLiteIndex::IdType packageRowId, const Caching::FileCache& fileCache)
            {
                auto pathAndHash = CreatePackageVersionDataRelativePath(source, packageRowId);
                auto fileStream = fileCache.GetFile(pathAndHash.first, Utility::SHA256::ConvertToBytes(pathAndHash.second));
                auto fileBytes = Utility::ReadEntireStreamAsByteArray(*fileStream);

                Manifest::PackageVersionDataManifest result;
                result.Deserialize(Manifest::PackageVersionDataManifest::CreateDecompressor().Decompress(fileBytes));

                return result;
            }

            // The IPackageVersion implementation for V2 index.
            struct PackageVersion : public SourceReference, public IPackageVersion
            {
                PackageVersion(
                    const std::shared_ptr<SQLiteIndexSource>& source,
                    SQLiteIndex::IdType packageRowId,
                    std::optional<Manifest::PackageVersionDataManifest::VersionData> packageVersionData,
                    const std::shared_ptr<Caching::FileCache>& manifestCache,
                    const std::shared_ptr<Caching::FileCache>& packageVersionDataCache) :
                        SourceReference(source),
                        m_packageRowId(packageRowId),
                        m_packageVersionData(std::move(packageVersionData)),
                        m_manifestCache(manifestCache),
                        m_packageVersionDataCache(packageVersionDataCache)
                {}

                // Inherited via IPackageVersion
                Utility::LocIndString GetProperty(PackageVersionProperty property) const override
                {
                    switch (property)
                    {
                    case PackageVersionProperty::SourceIdentifier:
                        return LocIndString{ GetReferenceSource()->GetIdentifier() };
                    case PackageVersionProperty::SourceName:
                        return LocIndString{ GetReferenceSource()->GetDetails().Name };
                    case PackageVersionProperty::RelativePath:
                    case PackageVersionProperty::ManifestSHA256Hash:
                    {
                        // These values can only come from the version data.
                        EnsurePackageVersionData();
                        return GetPropertyFromVersionData(property);
                    }
                        break;
                    case PackageVersionProperty::Publisher:
                    {
                        // These values can only come from the manifest.
                        EnsureManifest();
                        return GetPropertyFromManifest(property);
                    }
                        break;
                    case PackageVersionProperty::Id:
                    case PackageVersionProperty::Name:
                    case PackageVersionProperty::Moniker:
                    {
                        // These properties can come from the manifest or the index.
                        // The index values will be for the latest version rather than this specific one though.
                        auto sharedLock = m_versionAndManifestLock.lock_shared();

                        if (m_manifest)
                        {
                            return GetPropertyFromManifestWithLock(property);
                        }
                        else
                        {
                            return GetPropertyFromIndex(property);
                        }
                    }
                        break;
                    case PackageVersionProperty::Version:
                    case PackageVersionProperty::Channel:
                    case PackageVersionProperty::ArpMinVersion:
                    case PackageVersionProperty::ArpMaxVersion:
                    {
                        // These properties can come from the manifest, version data, or the index.
                        // The index values are only for the latest version, but we should always already have the version data
                        // for any version that is not the latest.
                        auto sharedLock = m_versionAndManifestLock.lock_shared();

                        if (m_manifest)
                        {
                            return GetPropertyFromManifestWithLock(property);
                        }
                        else if (m_packageVersionData)
                        {
                            return GetPropertyFromVersionDataWithLock(property);
                        }
                        else
                        {
                            return GetPropertyFromIndex(property);
                        }
                    }
                        break;
                    default:
                        THROW_HR(E_UNEXPECTED);
                    }
                }

                std::vector<Utility::LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const override
                {
                    switch (property)
                    {
                    case PackageVersionMultiProperty::Locale:
                    {
                        // These values can only come from the manifest.
                        EnsureManifest();
                        return GetMultiPropertyFromManifest(property);
                    }
                        break;
                    case PackageVersionMultiProperty::PackageFamilyName:
                    case PackageVersionMultiProperty::ProductCode:
                    case PackageVersionMultiProperty::UpgradeCode:
                    case PackageVersionMultiProperty::Name:
                    case PackageVersionMultiProperty::Publisher:
                    case PackageVersionMultiProperty::Tag:
                    case PackageVersionMultiProperty::Command:
                    {
                        // These properties can come from the manifest or the index.
                        // The index values will be for all versions rather than this specific one though.
                        auto sharedLock = m_versionAndManifestLock.lock_shared();

                        if (m_manifest)
                        {
                            return GetMultiPropertyFromManifestWithLock(property);
                        }
                        else
                        {
                            return GetMultiPropertyFromIndex(property);
                        }
                    }
                    break;
                    default:
                        THROW_HR(E_UNEXPECTED);
                    }
                }

                Manifest::Manifest GetManifest() override
                {
                    EnsureManifest();
                    auto sharedLock = m_versionAndManifestLock.lock_shared();
                    return m_manifest.value();
                }

                Source GetSource() const override
                {
                    return Source{ GetReferenceSource() };
                }

                IPackageVersion::Metadata GetMetadata() const override
                {
                    return {};
                }

            private:
                // Ensures that the package version data is present.
                void EnsurePackageVersionData() const
                {
                    {
                        auto sharedLock = m_versionAndManifestLock.lock_shared();
                        if (m_packageVersionData)
                        {
                            return;
                        }
                    }

                    auto exclusiveLock = m_versionAndManifestLock.lock_exclusive();
                    if (m_packageVersionData)
                    {
                        return;
                    }

                    Manifest::PackageVersionDataManifest packageVersionDataManifest = GetPackageVersionData(GetReferenceSource(), m_packageRowId, *m_packageVersionDataCache);

                    for (const auto& versionData : packageVersionDataManifest.Versions())
                    {
                        // We should only ever be looking for the latest version here.
                        if (!m_packageVersionData || m_packageVersionData->Version < versionData.Version)
                        {
                            m_packageVersionData = versionData;
                        }
                    }
                }

                // Ensures that the manifest is present.
                void EnsureManifest() const
                {
                    {
                        auto sharedLock = m_versionAndManifestLock.lock_shared();
                        if (m_manifest)
                        {
                            return;
                        }
                    }

                    // We will need the package version data to get the manifest.
                    EnsurePackageVersionData();

                    auto exclusiveLock = m_versionAndManifestLock.lock_exclusive();
                    if (m_manifest)
                    {
                        return;
                    }

                    std::unique_ptr<std::istream> manifestStream =
                        m_manifestCache->GetFile(m_packageVersionData->ManifestRelativePath, SHA256::ConvertToBytes(m_packageVersionData->ManifestHash));
                    m_manifest = Manifest::YamlParser::Create(Utility::ReadEntireStream(*manifestStream));
                    m_manifest->ApplyLocale();
                }

                Utility::LocIndString GetPropertyFromIndex(PackageVersionProperty property) const
                {
                    switch (property)
                    {
                    case PackageVersionProperty::Id:
                    case PackageVersionProperty::Name:
                    case PackageVersionProperty::Moniker:
                    case PackageVersionProperty::Version:
                    case PackageVersionProperty::ArpMinVersion:
                    case PackageVersionProperty::ArpMaxVersion:
                    {
                        // Values coming from the index will always be localized/independent.
                        std::optional<std::string> optValue = GetReferenceSource()->GetIndex().GetPropertyByPrimaryId(m_packageRowId, property);
                        return LocIndString{ optValue ? optValue.value() : std::string{} };
                    }
                    default:
                        return {};
                    }
                }

                Utility::LocIndString GetPropertyFromVersionData(PackageVersionProperty property) const
                {
                    auto sharedLock = m_versionAndManifestLock.lock_shared();
                    return GetPropertyFromVersionDataWithLock(property);
                }

                Utility::LocIndString GetPropertyFromVersionDataWithLock(PackageVersionProperty property) const
                {
                    std::string result;

                    switch (property)
                    {
                    case PackageVersionProperty::RelativePath:
                        result = m_packageVersionData->ManifestRelativePath;
                        break;
                    case PackageVersionProperty::ManifestSHA256Hash:
                        result = m_packageVersionData->ManifestHash;
                        break;
                    case PackageVersionProperty::Version:
                        result = m_packageVersionData->Version.ToString();
                        break;
                    case PackageVersionProperty::ArpMinVersion:
                        result = m_packageVersionData->ArpMinVersion.value_or("");
                        break;
                    case PackageVersionProperty::ArpMaxVersion:
                        result = m_packageVersionData->ArpMaxVersion.value_or("");
                        break;
                    }

                    return LocIndString{ std::move(result) };
                }

                Utility::LocIndString GetPropertyFromManifest(PackageVersionProperty property) const
                {
                    auto sharedLock = m_versionAndManifestLock.lock_shared();
                    return GetPropertyFromManifestWithLock(property);
                }

                Utility::LocIndString GetPropertyFromManifestWithLock(PackageVersionProperty property) const
                {
                    std::string result;

                    switch (property)
                    {
                    case PackageVersionProperty::Publisher:
                        result = m_manifest->CurrentLocalization.Get<Manifest::Localization::Publisher>();
                        break;
                    case PackageVersionProperty::Id:
                        result = m_manifest->Id;
                        break;
                    case PackageVersionProperty::Name:
                        result = m_manifest->CurrentLocalization.Get<Manifest::Localization::PackageName>();
                        break;
                    case PackageVersionProperty::Moniker:
                        result = m_manifest->Moniker;
                        break;
                    case PackageVersionProperty::Version:
                        result = m_manifest->Version;
                        break;
                    case PackageVersionProperty::Channel:
                        result = m_manifest->Channel;
                        break;
                    case PackageVersionProperty::ArpMinVersion:
                    {
                        auto versionRange = m_manifest->GetArpVersionRange();
                        if (!versionRange.IsEmpty())
                        {
                            result = versionRange.GetMinVersion().ToString();
                        }
                    }
                        break;
                    case PackageVersionProperty::ArpMaxVersion:
                    {
                        auto versionRange = m_manifest->GetArpVersionRange();
                        if (!versionRange.IsEmpty())
                        {
                            result = versionRange.GetMaxVersion().ToString();
                        }
                    }
                        break;
                    }

                    return LocIndString{ std::move(result) };
                }

                std::vector<Utility::LocIndString> GetMultiPropertyFromIndex(PackageVersionMultiProperty property) const
                {
                    std::vector<Utility::LocIndString> result;

                    for (auto&& value : GetReferenceSource()->GetIndex().GetMultiPropertyByPrimaryId(m_packageRowId, property))
                    {
                        // Values coming from the index will always be localized/independent.
                        result.emplace_back(std::move(value));
                    }

                    return result;
                }

                std::vector<Utility::LocIndString> GetMultiPropertyFromManifest(PackageVersionMultiProperty property) const
                {
                    auto sharedLock = m_versionAndManifestLock.lock_shared();
                    return GetMultiPropertyFromManifestWithLock(property);
                }

                std::vector<Utility::LocIndString> GetMultiPropertyFromManifestWithLock(PackageVersionMultiProperty property) const
                {
                    std::vector<Manifest::string_t> intermediate;

                    switch (property)
                    {
                    case PackageVersionMultiProperty::PackageFamilyName:
                        intermediate = m_manifest->GetPackageFamilyNames();
                        break;
                    case PackageVersionMultiProperty::ProductCode:
                        intermediate = m_manifest->GetProductCodes();
                        break;
                    case PackageVersionMultiProperty::UpgradeCode:
                        intermediate = m_manifest->GetUpgradeCodes();
                        break;
                    case PackageVersionMultiProperty::Name:
                        intermediate = m_manifest->GetPackageNames();
                        break;
                    case PackageVersionMultiProperty::Publisher:
                        intermediate = m_manifest->GetPublishers();
                        break;
                    case PackageVersionMultiProperty::Locale:
                        for (const auto& localization : m_manifest->Localizations)
                        {
                            intermediate.emplace_back(localization.Locale);
                        }
                        break;
                    case PackageVersionMultiProperty::Tag:
                        intermediate = m_manifest->GetAggregatedTags();
                        break;
                    case PackageVersionMultiProperty::Command:
                        intermediate = m_manifest->GetAggregatedCommands();
                        break;
                    }

                    std::vector<Utility::LocIndString> result;

                    for (auto&& value : intermediate)
                    {
                        // Values coming from the manifest will always be localized/independent.
                        result.emplace_back(std::move(value));
                    }

                    return result;
                }

                SQLiteIndex::IdType m_packageRowId;

                mutable wil::srwlock m_versionAndManifestLock;
                mutable std::optional<Manifest::PackageVersionDataManifest::VersionData> m_packageVersionData;
                mutable std::optional<Manifest::Manifest> m_manifest;

                std::shared_ptr<Caching::FileCache> m_manifestCache;
                std::shared_ptr<Caching::FileCache> m_packageVersionDataCache;
            };

            // The IPackage implementation for V2 index.
            struct SQLitePackage : public std::enable_shared_from_this<SQLitePackage>, public SourceReference, public IPackage, public ICompositePackage
            {
                static constexpr IPackageType PackageType = IPackageType::SQLitePackage2;

                SQLitePackage(
                    const std::shared_ptr<SQLiteIndexSource>& source,
                    SQLiteIndex::IdType packageRowId,
                    const std::shared_ptr<Caching::FileCache>& manifestCache,
                    const std::shared_ptr<Caching::FileCache>& packageVersionDataCache,
                    bool isInstalled) :
                    SourceReference(source),
                    m_packageRowId(packageRowId),
                    m_manifestCache(manifestCache),
                    m_packageVersionDataCache(packageVersionDataCache),
                    m_isInstalled(isInstalled)
                {}

                // Inherited via IPackage
                Utility::LocIndString GetProperty(PackageProperty property) const
                {
                    std::optional<std::string> result;
                    std::shared_ptr<SQLiteIndexSource> source = GetReferenceSource();

                    switch (property)
                    {
                    case PackageProperty::Id:
                        result = source->GetIndex().GetPropertyByPrimaryId(m_packageRowId, PackageVersionProperty::Id);
                        break;
                    case PackageProperty::Name:
                        result = source->GetIndex().GetPropertyByPrimaryId(m_packageRowId, PackageVersionProperty::Name);
                        break;
                    default:
                        THROW_HR(E_UNEXPECTED);
                    }

                    return Utility::LocIndString{ result ? std::move(result).value() : std::string{} };
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

                    EnsurePackageVersionData(source);

                    auto sharedLock = m_versionKeysLock.lock_shared();
                    return m_versionKeys;
                }

                std::shared_ptr<IPackageVersion> GetLatestVersion() const override
                {
                    std::shared_ptr<SQLiteIndexSource> source = GetReferenceSource();
                    auto sharedLock = m_versionKeysLock.lock_shared();
                    return std::make_shared<PackageVersion>(source, m_packageRowId, m_latestVersionData, m_manifestCache, m_packageVersionDataCache);
                }

                std::shared_ptr<IPackageVersion> GetVersion(const PackageVersionKey& versionKey) const override
                {
                    std::shared_ptr<SQLiteIndexSource> source = GetReferenceSource();

                    // Ensure that this key targets this (or any) source
                    if (!versionKey.SourceId.empty() && versionKey.SourceId != source->GetIdentifier())
                    {
                        return {};
                    }

                    std::optional<Manifest::PackageVersionDataManifest::VersionData> versionData;

                    // Check for a latest version request.
                    if (versionKey.Version.empty() && versionKey.Channel.empty())
                    {
                        auto sharedLock = m_versionKeysLock.lock_shared();
                        return std::make_shared<PackageVersion>(source, m_packageRowId, m_latestVersionData, m_manifestCache, m_packageVersionDataCache);
                    }

                    EnsurePackageVersionData(source);

                    {
                        MapKey requested{ versionKey.Version, versionKey.Channel };
                        auto sharedLock = m_versionKeysLock.lock_shared();

                        auto itr = m_versionKeysMap.find(requested);
                        if (itr != m_versionKeysMap.end())
                        {
                            versionData = itr->second;
                        }
                    }

                    if (versionData)
                    {
                        return std::make_shared<PackageVersion>(source, m_packageRowId, std::move(versionData), m_manifestCache, m_packageVersionDataCache);
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
                        return GetReferenceSource()->IsSame(otherSQLite->GetReferenceSource().get()) && m_packageRowId == otherSQLite->m_packageRowId;
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

                // Ensures that we have the package version data present.
                void EnsurePackageVersionData(const std::shared_ptr<SQLiteIndexSource>& source) const
                {
                    {
                        auto sharedLock = m_versionKeysLock.lock_shared();

                        if (!m_versionKeys.empty())
                        {
                            return;
                        }
                    }

                    auto exclusiveLock = m_versionKeysLock.lock_exclusive();

                    if (!m_versionKeys.empty())
                    {
                        return;
                    }

                    Manifest::PackageVersionDataManifest packageVersionDataManifest = GetPackageVersionData(source, m_packageRowId, *m_packageVersionDataCache);

                    for (const auto& versionData : packageVersionDataManifest.Versions())
                    {
                        std::string version = versionData.Version.ToString();
                        std::string channel;
                        m_versionKeys.emplace_back(source->GetIdentifier(), version, channel);
                        m_versionKeysMap.emplace(MapKey{ std::move(version), std::move(channel) }, versionData);

                        if (!m_latestVersionData || m_latestVersionData->Version < versionData.Version)
                        {
                            m_latestVersionData = versionData;
                        }
                    }
                }

                SQLiteIndex::IdType m_packageRowId;
                std::shared_ptr<Caching::FileCache> m_manifestCache;
                std::shared_ptr<Caching::FileCache> m_packageVersionDataCache;
                bool m_isInstalled;

                // To avoid removing const from the interface
                mutable wil::srwlock m_versionKeysLock;
                mutable std::vector<PackageVersionKey> m_versionKeys;
                mutable std::map<MapKey, Manifest::PackageVersionDataManifest::VersionData> m_versionKeysMap;
                mutable std::optional<Manifest::PackageVersionDataManifest::VersionData> m_latestVersionData;
            };
        }
    }

    SQLiteIndexSource::SQLiteIndexSource(
        const SourceDetails& details,
        SQLiteIndex&& index,
        bool isInstalledSource,
        bool requireManifestHash) :
        m_details(details), m_isInstalled(isInstalledSource), m_index(std::move(index)), m_requireManifestHash(requireManifestHash)
    {
        std::vector<std::string> cacheSources;
        cacheSources.push_back(m_details.Arg);
        if (!m_details.AlternateArg.empty())
        {
            cacheSources.push_back(m_details.AlternateArg);
        }

        switch (m_index.GetVersion().MajorVersion)
        {
        case 1:
            m_manifestCache = std::make_shared<Caching::FileCache>(Caching::FileCache::Type::IndexV1_Manifest, m_details.Identifier, std::move(cacheSources));
            break;
        case 2:
            m_manifestCache = std::make_shared<Caching::FileCache>(Caching::FileCache::Type::IndexV2_Manifest, m_details.Identifier, cacheSources);
            m_packageVersionDataCache = std::make_shared<Caching::FileCache>(Caching::FileCache::Type::IndexV2_PackageVersionData, m_details.Identifier, std::move(cacheSources));
            break;
        default:
            THROW_WIN32(ERROR_NOT_SUPPORTED);
        }
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
        uint32_t majorVersion = m_index.GetVersion().MajorVersion;

        for (auto& indexResult : indexResults.Matches)
        {
            std::shared_ptr<ICompositePackage> package;

            switch (majorVersion)
            {
            case 1:
                package = std::make_shared<anon::V1::SQLitePackage>(sharedThis, indexResult.first, m_manifestCache, m_isInstalled);
                break;
            case 2:
                package = std::make_shared<anon::V2::SQLitePackage>(sharedThis, indexResult.first, m_manifestCache, m_packageVersionDataCache, m_isInstalled);
                break;
            default:
                THROW_WIN32(ERROR_NOT_SUPPORTED);
            }

            result.Matches.emplace_back(
                std::move(package),
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
