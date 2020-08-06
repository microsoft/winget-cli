// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "AppInstallerRepositorySource.h"

namespace AppInstaller::Repository
{
    struct AggregatedSource : public ISource
    {
        AggregatedSource();

        AggregatedSource(const AggregatedSource&) = delete;
        AggregatedSource& operator=(const AggregatedSource&) = delete;

        AggregatedSource(AggregatedSource&&) = default;
        AggregatedSource& operator=(AggregatedSource&&) = default;

        ~AggregatedSource() = default;

        // Get the source's details.
        const SourceDetails& GetDetails() const override;

        // Execute a search on the source.
        SearchResult Search(const SearchRequest & request) override;

        void AddSource(std::shared_ptr<ISource> source);

    private:
        std::vector<std::shared_ptr<ISource>> m_sources;
        SourceDetails m_details;

        void SortResultMatches(std::vector<ResultMatch>& matches);
    };
}


