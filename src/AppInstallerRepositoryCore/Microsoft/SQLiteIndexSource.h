// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/SQLiteIndex.h"
#include "ISource.h"
#include <AppInstallerSynchronization.h>

#include <memory>


namespace AppInstaller::Repository::Microsoft
{
    // A source that holds a SQLiteIndex and lock.
    struct SQLiteIndexSource : public std::enable_shared_from_this<SQLiteIndexSource>, public ISource
    {
        SQLiteIndexSource(
            const SourceDetails& details,
            std::string identifier,
            std::function<SQLiteIndex(const SourceDetails&, IProgressCallback&, Synchronization::CrossProcessReaderWriteLock&)>&& getIndexFunc,
            bool isInstalledSource = false);

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

        void UpdateLastUpdateTime(std::chrono::system_clock::time_point time) override;

        void Open(IProgressCallback& progress) override;

        // Execute a search on the source.
        SearchResult Search(const SearchRequest& request) const override;

        // Gets the index.
        const SQLiteIndex& GetIndex() const;

        // Determines if the other source refers to the same as this.
        bool IsSame(const SQLiteIndexSource* other) const;

    private:
        std::string m_identifier;
        SourceDetails m_details;
        Synchronization::CrossProcessReaderWriteLock m_lock;
        bool m_isInstalled;
        std::function<SQLiteIndex(const SourceDetails&, IProgressCallback&, Synchronization::CrossProcessReaderWriteLock&)> m_getIndexFunc;

    protected:
        std::optional<SQLiteIndex> m_index;
        std::once_flag m_openFlag;
        std::atomic_bool m_isOpened = false;
    };

    // A source that holds a SQLiteIndex and lock.
    struct SQLiteIndexWriteableSource : public SQLiteIndexSource, public IMutablePackageSource
    {
        SQLiteIndexWriteableSource(const SourceDetails& details,
            std::string identifier,
            std::function<SQLiteIndex(const SourceDetails&, IProgressCallback&, Synchronization::CrossProcessReaderWriteLock&)>&& getIndexFunc,
            bool isInstalledSource = false);

        // Adds a package version to the source.
        void AddPackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);

        // Removes a package version from the source.
        void RemovePackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);
    };
}
