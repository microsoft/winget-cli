// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "AppInstallerRepositorySource.h"

namespace AppInstaller::Repository
{
    struct CompositeSource : public ISource
    {
        explicit CompositeSource(std::string identifier);

        CompositeSource(const CompositeSource&) = delete;
        CompositeSource& operator=(const CompositeSource&) = delete;

        CompositeSource(CompositeSource&&) = default;
        CompositeSource& operator=(CompositeSource&&) = default;

        ~CompositeSource() = default;

        // ISource

        // Get the source's details.
        const SourceDetails& GetDetails() const override;

        // Gets the source's identifier; a unique identifier independent of the name
        // that will not change between a remove/add or between additional adds.
        // Must be suitable for filesystem names.
        const std::string& GetIdentifier() const override;

        // Execute a search on the source.
        SearchResult Search(const SearchRequest& request) const override;

        // ~ISource

        // Adds an available source to be aggregated.
        void AddAvailableSource(std::shared_ptr<ISource> source);

        // Sets the installed source to be composited.
        void SetInstalledSource(std::shared_ptr<ISource> source);

    private:
        // Sorts a vector of results.
        static void SortResultMatches(std::vector<ResultMatch>& matches);

        std::shared_ptr<ISource> m_installedSource;
        std::vector<std::shared_ptr<ISource>> m_availableSources;
        SourceDetails m_details;
        std::string m_identifier;
    };
}


