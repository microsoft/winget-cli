// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/SQLiteIndexSourceV2.h"
#include <winget/ManifestYamlParser.h>

using namespace AppInstaller::Utility;


namespace AppInstaller::Repository::Microsoft::details::V2
{
    // Get the relative path and hash for the package version data manifest.
    std::pair<std::filesystem::path, std::string> CreatePackageVersionDataRelativePath(const std::shared_ptr<SQLiteIndexSource>& source, SQLiteIndex::IdType packageRowId)
    {
        const SQLiteIndex& index = source->GetIndex();

        std::string identifier = index.GetPropertyByPrimaryId(packageRowId, PackageVersionProperty::Id).value();
        std::string hash = index.GetPropertyByPrimaryId(packageRowId, PackageVersionProperty::ManifestSHA256Hash).value();
        std::filesystem::path relativePath = Manifest::PackageVersionDataManifest::GetRelativeDirectoryPath(identifier, hash) / Manifest::PackageVersionDataManifest::VersionManifestCompressedFileName();

        return std::make_pair(std::move(relativePath), std::move(hash));
    }

    // Gets package version data for the given package in the index.
    Manifest::PackageVersionDataManifest GetPackageVersionData(const std::shared_ptr<SQLiteIndexSource>& source, SQLiteIndex::IdType packageRowId, const Caching::FileCache& fileCache)
    {
        auto pathAndHash = CreatePackageVersionDataRelativePath(source, packageRowId);
        auto fileStream = fileCache.GetFile(pathAndHash.first, SHA256::ConvertToBytes(pathAndHash.second));
        auto fileBytes = ReadEntireStreamAsByteArray(*fileStream);

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
        LocIndString GetProperty(PackageVersionProperty property) const override
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

        std::vector<LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const override
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
                m_manifestCache->GetFile(ConvertToUTF16(m_packageVersionData->ManifestRelativePath), SHA256::ConvertToBytes(m_packageVersionData->ManifestHash));
            m_manifest = Manifest::YamlParser::Create(ReadEntireStream(*manifestStream));
            m_manifest->ApplyLocale();
        }

        LocIndString GetPropertyFromIndex(PackageVersionProperty property) const
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

        LocIndString GetPropertyFromVersionData(PackageVersionProperty property) const
        {
            auto sharedLock = m_versionAndManifestLock.lock_shared();
            return GetPropertyFromVersionDataWithLock(property);
        }

        LocIndString GetPropertyFromVersionDataWithLock(PackageVersionProperty property) const
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

        LocIndString GetPropertyFromManifest(PackageVersionProperty property) const
        {
            auto sharedLock = m_versionAndManifestLock.lock_shared();
            return GetPropertyFromManifestWithLock(property);
        }

        LocIndString GetPropertyFromManifestWithLock(PackageVersionProperty property) const
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

        std::vector<LocIndString> GetMultiPropertyFromIndex(PackageVersionMultiProperty property) const
        {
            std::vector<LocIndString> result;

            for (auto&& value : GetReferenceSource()->GetIndex().GetMultiPropertyByPrimaryId(m_packageRowId, property))
            {
                // Values coming from the index will always be localized/independent.
                result.emplace_back(std::move(value));
            }

            return result;
        }

        std::vector<LocIndString> GetMultiPropertyFromManifest(PackageVersionMultiProperty property) const
        {
            auto sharedLock = m_versionAndManifestLock.lock_shared();
            return GetMultiPropertyFromManifestWithLock(property);
        }

        std::vector<LocIndString> GetMultiPropertyFromManifestWithLock(PackageVersionMultiProperty property) const
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

            std::vector<LocIndString> result;

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

    SQLitePackage::SQLitePackage(
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

    LocIndString SQLitePackage::GetProperty(PackageProperty property) const
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

        return LocIndString{ result ? std::move(result).value() : std::string{} };
    }

    std::vector<PackageVersionKey> SQLitePackage::GetVersionKeys() const
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

    std::shared_ptr<IPackageVersion> SQLitePackage::GetLatestVersion() const
    {
        std::shared_ptr<SQLiteIndexSource> source = GetReferenceSource();
        auto sharedLock = m_versionKeysLock.lock_shared();
        return std::make_shared<PackageVersion>(source, m_packageRowId, m_latestVersionData, m_manifestCache, m_packageVersionDataCache);
    }

    std::shared_ptr<IPackageVersion> SQLitePackage::GetVersion(const PackageVersionKey& versionKey) const
    {
        std::shared_ptr<SQLiteIndexSource> source = GetReferenceSource();

        // Ensure that this key targets this (or any) source
        if (!versionKey.SourceId.empty() && versionKey.SourceId != source->GetIdentifier())
        {
            return {};
        }

        std::optional<Manifest::PackageVersionDataManifest::VersionData> versionData;

        // Check for a latest version request.
        if (versionKey.IsDefaultLatest())
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

    Source SQLitePackage::GetSource() const
    {
        return Source{ GetReferenceSource() };
    }

    bool SQLitePackage::IsSame(const IPackage* other) const
    {
        const SQLitePackage* otherSQLite = PackageCast<const SQLitePackage*>(other);

        if (otherSQLite)
        {
            return GetReferenceSource()->IsSame(otherSQLite->GetReferenceSource().get()) && m_packageRowId == otherSQLite->m_packageRowId;
        }

        return false;
    }

    const void* SQLitePackage::CastTo(IPackageType type) const
    {
        if (type == PackageType)
        {
            return this;
        }

        return nullptr;
    }

    std::shared_ptr<IPackage> SQLitePackage::GetInstalled()
    {
        return m_isInstalled ? shared_from_this() : std::shared_ptr<IPackage>{};
    }

    std::vector<std::shared_ptr<IPackage>> SQLitePackage::GetAvailable()
    {
        return m_isInstalled ? std::vector<std::shared_ptr<IPackage>>{} : std::vector<std::shared_ptr<IPackage>>{ shared_from_this() };
    }

    bool SQLitePackage::MapKey::operator<(const MapKey& other) const
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

    // Ensures that we have the package version data present.
    void SQLitePackage::EnsurePackageVersionData(const std::shared_ptr<SQLiteIndexSource>& source) const
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
}
