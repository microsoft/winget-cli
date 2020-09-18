// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "AppInstallerRepositorySource.h"

namespace AppInstaller::Repository
{
    struct AggregatedSource : public ISource
    {
        explicit AggregatedSource(std::string identifier);

        AggregatedSource(const AggregatedSource&) = delete;
        AggregatedSource& operator=(const AggregatedSource&) = delete;

        AggregatedSource(AggregatedSource&&) = default;
        AggregatedSource& operator=(AggregatedSource&&) = default;

        ~AggregatedSource() = default;

        // Get the source's details.
        const SourceDetails& GetDetails() const override;

        // Gets the source's identifier; a unique identifier independent of the name
        // that will not change between a remove/add or between additional adds.
        // Must be suitable for filesystem names.
        const std::string& GetIdentifier() const override;

        // Execute a search on the source.
        SearchResult Search(const SearchRequest & request) const override;

        // Adds a source to be aggregated.
        void AddSource(std::shared_ptr<ISource> source);

    private:
        std::vector<std::shared_ptr<ISource>> m_sources;
        SourceDetails m_details;
        std::string m_identifier;

        // Sorts a vector of results.
        static void SortResultMatches(std::vector<ResultMatch>& matches);
    };
}


