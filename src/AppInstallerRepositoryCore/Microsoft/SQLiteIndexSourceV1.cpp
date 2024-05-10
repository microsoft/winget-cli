// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/SQLiteIndexSourceV1.h"
#include <winget/ManifestYamlParser.h>

using namespace AppInstaller::Utility;


namespace AppInstaller::Repository::Microsoft::details::V1
{
    // The IPackageVersion implementation for V1 index.
    struct PackageVersion : public SourceReference, public IPackageVersion
    {
        PackageVersion(const std::shared_ptr<SQLiteIndexSource>& source, SQLiteIndex::IdType manifestId, const std::shared_ptr<Caching::FileCache>& manifestCache) :
            SourceReference(source), m_manifestId(manifestId), m_manifestCache(manifestCache) {}

        // Inherited via IPackageVersion
        LocIndString GetProperty(PackageVersionProperty property) const override
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

        std::vector<LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const override
        {
            std::vector<LocIndString> result;

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

            std::unique_ptr<std::istream> manifestStream = m_manifestCache->GetFile(ConvertToUTF16(relativePathOpt.value()), manifestSHA256);
            return Manifest::YamlParser::Create(ReadEntireStream(*manifestStream));
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

    SQLitePackage::SQLitePackage(const std::shared_ptr<SQLiteIndexSource>& source, SQLiteIndex::IdType idId, const std::shared_ptr<Caching::FileCache>& manifestCache, bool isInstalled) :
        SourceReference(source), m_idId(idId), m_manifestCache(manifestCache), m_isInstalled(isInstalled) {}

    LocIndString SQLitePackage::GetProperty(PackageProperty property) const
    {
        LocIndString result;

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

    std::shared_ptr<IPackageVersion> SQLitePackage::GetLatestVersion() const
    {
        std::shared_ptr<SQLiteIndexSource> source = GetReferenceSource();
        std::optional<SQLiteIndex::IdType> manifestId = source->GetIndex().GetManifestIdByKey(m_idId, {}, {});

        if (manifestId)
        {
            return std::make_shared<PackageVersion>(source, manifestId.value(), m_manifestCache);
        }

        return {};
    }

    std::shared_ptr<IPackageVersion> SQLitePackage::GetVersion(const PackageVersionKey& versionKey) const
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

    Source SQLitePackage::GetSource() const
    {
        return Source{ GetReferenceSource() };
    }

    bool SQLitePackage::IsSame(const IPackage* other) const
    {
        const SQLitePackage* otherSQLite = PackageCast<const SQLitePackage*>(other);

        if (otherSQLite)
        {
            return GetReferenceSource()->IsSame(otherSQLite->GetReferenceSource().get()) && m_idId == otherSQLite->m_idId;
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
}
