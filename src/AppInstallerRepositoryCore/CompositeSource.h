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

        // Gets a value indicating whether this source is a composite of other sources,
        // and thus the packages may come from disparate sources as well.
        bool IsComposite() const override { return true; }

        // Execute a search on the source.
        SearchResult Search(const SearchRequest& request) const override;

        // ~ISource

        // Adds an available source to be aggregated.
        void AddAvailableSource(std::shared_ptr<ISource> source);

        // Sets the installed source to be composited.
        void SetInstalledSource(std::shared_ptr<ISource> source, CompositeSearchBehavior searchBehavior = CompositeSearchBehavior::Installed);

    private:
        // Performs a search when an installed source is present.
        // Will only return packages that are installed.
        SearchResult SearchInstalled(const SearchRequest& request) const;

        // Performs a search when no installed source is present.
        SearchResult SearchAvailable(const SearchRequest& request) const;

        std::shared_ptr<ISource> m_installedSource;
        std::vector<std::shared_ptr<ISource>> m_availableSources;
        SourceDetails m_details;
        CompositeSearchBehavior m_searchBehavior;
    };
}


