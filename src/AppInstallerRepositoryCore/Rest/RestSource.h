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
        RestSource(const SourceDetails& details);

        RestSource(const RestSource&) = delete;
        RestSource& operator=(const RestSource&) = delete;

        RestSource(RestSource&&) = default;
        RestSource& operator=(RestSource&&) = default;

        ~RestSource() = default;

        // Gets the source's identifier; a unique identifier independent of the name
        // that will not change between a remove/add or between additional adds.
        // Must be suitable for filesystem names.
        const std::string& GetIdentifier() override;

        // Get the source's details.
        SourceDetails& GetDetails() override;

        SourceInformation GetInformation() const override;

        void Open(IProgressCallback& progress) override;

        // Execute a search on the source.
        SearchResult Search(const SearchRequest& request) const override;

        bool SetCustomHeader(std::string_view header) override;

        // Gets the rest client.
        const RestClient& GetRestClient() const;

        // Determines if the other source refers to the same as this.
        bool IsSame(const RestSource* other) const;

    private:

        void OpenSourceInternal();

        std::string m_identifier;
        SourceDetails m_details;
        SourceInformation m_information;
        std::string m_header;
        std::optional<RestClient> m_restClient;
        std::once_flag m_openFlag;
        std::atomic_bool m_isOpened = false;
    };
}
