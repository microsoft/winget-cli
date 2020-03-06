// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/SQLiteIndex.h"
#include "Public/AppInstallerRepositorySource.h"
#include <AppInstallerSynchronization.h>

#include <memory>


namespace AppInstaller::Repository::Microsoft
{
    // A source that holds a SQLiteIndex and lock.
    struct SQLiteIndexSource : public std::enable_shared_from_this<SQLiteIndexSource>, public ISource
    {
        SQLiteIndexSource(const SourceDetails& details, SQLiteIndex&& index, Synchronization::CrossProcessReaderWriteLock&& lock = {});

        SQLiteIndexSource(const SQLiteIndexSource&) = delete;
        SQLiteIndexSource& operator=(const SQLiteIndexSource&) = delete;

        SQLiteIndexSource(SQLiteIndexSource&&) = default;
        SQLiteIndexSource& operator=(SQLiteIndexSource&&) = default;

        ~SQLiteIndexSource() = default;

        // Get the source's details.
        const SourceDetails& GetDetails() const override;

        // Execute a search on the source.
        SearchResult Search(const SearchRequest& request) override;

        // Gets the index.
        SQLiteIndex& GetIndex() { return m_index; }

    private:
        SourceDetails m_details;
        Synchronization::CrossProcessReaderWriteLock m_lock;
        SQLiteIndex m_index;
    };
}
