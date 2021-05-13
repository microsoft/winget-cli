// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/AppInstallerRepositorySource.h"
#include "RestClient.h"

namespace AppInstaller::Repository::Rest
{
    // A source that holds a RestSource.
    struct RestSource : public std::enable_shared_from_this<RestSource>, public ISource
    {
        RestSource(const SourceDetails& details, std::string identifier, RestClient&& m_restClient);

        RestSource(const RestSource&) = delete;
        RestSource& operator=(const RestSource&) = delete;

        RestSource(RestSource&&) = default;
        RestSource& operator=(RestSource&&) = default;

        ~RestSource() = default;

        // Get the source's details.
        const SourceDetails& GetDetails() const override;

        // Gets the source's identifier; a unique identifier independent of the name
        // that will not change between a remove/add or between additional adds.
        // Must be suitable for filesystem names.
        const std::string& GetIdentifier() const override;

        // Gets the rest client.
        const RestClient& GetRestClient() const;

        // Execute a search on the source.
        SearchResult Search(const SearchRequest& request) const override;

        // Determines if the other source refers to the same as this.
        bool IsSame(const RestSource* other) const;

    private:
        SourceDetails m_details;
        RestClient m_restClient;
    };
}
