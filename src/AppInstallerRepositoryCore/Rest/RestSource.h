// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ISource.h"
#include "RestClient.h"

namespace AppInstaller::Repository::Rest
{
    // A source that holds a RestSource.
    struct RestSource : public std::enable_shared_from_this<RestSource>, public ISource
    {
        static constexpr ISourceType SourceType = ISourceType::RestSource;

        RestSource(const SourceDetails& details, SourceInformation information, RestClient&& restClient);

        RestSource(const RestSource&) = delete;
        RestSource& operator=(const RestSource&) = delete;

        RestSource(RestSource&&) = default;
        RestSource& operator=(RestSource&&) = default;

        ~RestSource() = default;

        // Gets the source's identifier; a unique identifier independent of the name
        // that will not change between a remove/add or between additional adds.
        // Must be suitable for filesystem names.
        const std::string& GetIdentifier() const override;

        // Get the source's details.
        const SourceDetails& GetDetails() const override;

        SourceInformation GetInformation() const override;

        bool QueryFeatureFlag(SourceFeatureFlag flag) const override;

        // Execute a search on the source.
        SearchResult Search(const SearchRequest& request) const override;

        // Casts to the requested type.
        void* CastTo(ISourceType type) override;

        // Gets the rest client.
        const RestClient& GetRestClient() const;

        // Determines if the other source refers to the same as this.
        bool IsSame(const RestSource* other) const;

    private:
        std::shared_ptr<RestSource> NonConstSharedFromThis() const;

        SourceDetails m_details;
        SourceInformation m_information;
        RestClient m_restClient;
    };
}
