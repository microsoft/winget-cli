// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestSource.h"
#include "RestSourceFactory.h"
#include "cpprest/http_client.h"

namespace AppInstaller::Repository::Rest
{
    RestSource::RestSource(const SourceDetails& details, std::string identifier, RestClient&& restClient)
        : m_details(details), m_identifier(std::move(identifier)), m_restClient(std::move(restClient))
    {
    }

    const SourceDetails& RestSource::GetDetails() const
    {
        return m_details;
    }

    const std::string& RestSource::GetIdentifier() const
    {
        return m_identifier;
    }

    SearchResult RestSource::Search(const SearchRequest& request) const
    {
        // Make search for everything work
        RestClient::SearchResult results = m_restClient.Search(request);
        SearchResult returnVal;
        return returnVal;
    }
}
