// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ISource.h"

namespace AppInstaller::Repository
{
    struct CompositeSource : public ISource
    {
        static constexpr ISourceType SourceType = ISourceType::CompositeSource;

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

        // Casts to the requested type.
        void* CastTo(ISourceType type) override;

        // ~ISource

        // Adds an available source to be aggregated.
        void AddAvailableSource(const Source& source);

        // Gets the available sources if the source is composite.
        std::vector<Source> GetAvailableSources() const { return m_availableSources; }

        // Checks if any available sources are present
        bool HasAvailableSource() const { return !m_availableSources.empty(); }

        // Sets the installed source to be composited.
        void SetInstalledSource(Source source, CompositeSearchBehavior searchBehavior = CompositeSearchBehavior::Installed);

    private:
        // Performs a search when an installed source is present.
        SearchResult SearchInstalled(const SearchRequest& request) const;

        // Performs a search when no installed source is present.
        SearchResult SearchAvailable(const SearchRequest& request) const;

        Source m_installedSource;
        std::vector<Source> m_availableSources;
        SourceDetails m_details;
        CompositeSearchBehavior m_searchBehavior = CompositeSearchBehavior::Installed;
    };
}
