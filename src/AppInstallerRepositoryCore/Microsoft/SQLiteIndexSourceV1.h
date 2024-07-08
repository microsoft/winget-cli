// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/SQLiteIndexSource.h"


namespace AppInstaller::Repository::Microsoft::details::V1
{
    // The IPackage implementation for V1 index.
    struct SQLitePackage : public std::enable_shared_from_this<SQLitePackage>, public SourceReference, public IPackage, public ICompositePackage
    {
        static constexpr IPackageType PackageType = IPackageType::SQLitePackage1;

        SQLitePackage(const std::shared_ptr<SQLiteIndexSource>& source, SQLiteIndex::IdType idId, const std::shared_ptr<Caching::FileCache>& manifestCache, bool isInstalled);

        // Inherited via IPackage
        Utility::LocIndString GetProperty(PackageProperty property) const;

        std::vector<PackageVersionKey> GetVersionKeys() const override;

        std::shared_ptr<IPackageVersion> GetLatestVersion() const override;

        std::shared_ptr<IPackageVersion> GetVersion(const PackageVersionKey& versionKey) const override;

        Source GetSource() const override;

        bool IsSame(const IPackage* other) const override;

        const void* CastTo(IPackageType type) const override;

        // Inherited via ICompositePackage
        std::shared_ptr<IPackage> GetInstalled() override;

        std::vector<std::shared_ptr<IPackage>> GetAvailable() override;

    private:
        // Contains the information needed to map a version key to it's rows.
        struct MapKey
        {
            Utility::NormalizedString Version;
            Utility::NormalizedString Channel;

            bool operator<(const MapKey& other) const;
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
