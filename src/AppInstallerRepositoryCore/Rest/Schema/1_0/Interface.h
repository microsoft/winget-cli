// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/IRestClient.h"
#include <winget/HttpClientHelper.h>

namespace AppInstaller::Repository::Rest::Schema::V1_0
{
    // Interface to this schema version exposed through IRestClient.
    struct Interface : public IRestClient
    {
        Interface(const std::string& restApi, const Http::HttpClientHelper& helper);

        Interface(const Interface&) = delete;
        Interface& operator=(const Interface&) = delete;

        Interface(Interface&&) = default;
        Interface& operator=(Interface&&) = default;

        Utility::Version GetVersion() const override;
        IRestClient::Information GetSourceInformation() const override;
        IRestClient::SearchResult Search(const SearchRequest& request) const override;
        std::optional<Manifest::Manifest> GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const override;
        std::vector<Manifest::Manifest> GetManifests(const std::string& packageId, const std::map<std::string_view, std::string>& params = {}) const override;

    protected:
        bool MeetsOptimizedSearchCriteria(const SearchRequest& request) const;
        IRestClient::SearchResult OptimizedSearch(const SearchRequest& request) const;
        IRestClient::SearchResult SearchInternal(const SearchRequest& request) const;

        // Check query params against source information and update if necessary.
        virtual std::map<std::string_view, std::string> GetValidatedQueryParams(const std::map<std::string_view, std::string>& params) const;

        // Check search request against source information and get json search body.
        virtual web::json::value GetValidatedSearchBody(const SearchRequest& searchRequest) const;

        virtual SearchResult GetSearchResult(const web::json::value& searchResponseObject) const;
        virtual std::vector<Manifest::Manifest> GetParsedManifests(const web::json::value& manifestsResponseObject) const;

        // Gets auth headers if source requires authentication for access.
        virtual Http::HttpClientHelper::HttpRequestHeaders GetAuthHeaders() const;

        Http::HttpClientHelper::HttpRequestHeaders m_requiredRestApiHeaders;

    private:
        std::string m_restApiUri;
        utility::string_t m_searchEndpoint;
        Http::HttpClientHelper m_httpClientHelper;
    };
}
