// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/SQLiteIndex.h"
#include "ISource.h"
#include <winget/FileCache.h>
#include <memory>


namespace AppInstaller::Repository::Microsoft
{
    // A source that holds a SQLiteIndex and lock.
    struct SQLiteIndexSource : public std::enable_shared_from_this<SQLiteIndexSource>, public ISource
    {
        static constexpr ISourceType SourceType = ISourceType::SQLiteIndexSource;

        SQLiteIndexSource(
            const SourceDetails& details,
            SQLiteIndex&& index,
            bool isInstalledSource = false, 
            bool requireManifestHash = false);

        SQLiteIndexSource(const SQLiteIndexSource&) = delete;
        SQLiteIndexSource& operator=(const SQLiteIndexSource&) = delete;

        SQLiteIndexSource(SQLiteIndexSource&&) = default;
        SQLiteIndexSource& operator=(SQLiteIndexSource&&) = default;

        ~SQLiteIndexSource() = default;

        // Get the source's details.
        const SourceDetails& GetDetails() const override;

        // Gets the source's identifier; a unique identifier independent of the name
        // that will not change between a remove/add or between additional adds.
        // Must be suitable for filesystem names.
        const std::string& GetIdentifier() const override;

        // Execute a search on the source.
        SearchResult Search(const SearchRequest& request) const override;

        // Casts to the requested type.
        void* CastTo(ISourceType type) override;

        // Gets the index.
        SQLiteIndex& GetIndex() { return m_index; }
        const SQLiteIndex& GetIndex() const { return m_index; }

        // Determines if the other source refers to the same as this.
        bool IsSame(const SQLiteIndexSource* other) const;

        bool RequireManifestHash() const { return m_requireManifestHash; }

    private:
        std::shared_ptr<SQLiteIndexSource> NonConstSharedFromThis() const;

        SourceDetails m_details;
        bool m_requireManifestHash;
        bool m_isInstalled;
        std::shared_ptr<Caching::FileCache> m_manifestCache;
        std::shared_ptr<Caching::FileCache> m_packageVersionDataCache;

    protected:
        SQLiteIndex m_index;
    };

    // A source that holds a SQLiteIndex and lock.
    struct SQLiteIndexWriteableSource : public SQLiteIndexSource, public IMutablePackageSource
    {
        SQLiteIndexWriteableSource(
            const SourceDetails& details,
            SQLiteIndex&& index,
            bool isInstalledSource = false);

        // Casts to the requested type.
        void* CastTo(ISourceType type) override;

        // Adds a package version to the source.
        void AddPackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);

        // Removes a package version from the source.
        void RemovePackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);
    };

    namespace details
    {
        // For the IPackage(Version) implementations that need to hold a weak reference to a SQLiteIndexSource.
        struct SourceReference
        {
            SourceReference(const std::shared_ptr<SQLiteIndexSource>& source);

        protected:
            std::shared_ptr<SQLiteIndexSource> GetReferenceSource() const;

        private:
            std::weak_ptr<SQLiteIndexSource> m_source;
        };
    }
}
