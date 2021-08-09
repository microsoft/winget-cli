// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/1_0/Interface.h"

namespace AppInstaller::Repository::Rest::Schema::V1_1
{
    // Interface to this schema version exposed through IRestClient.
    struct Interface : public V1_0::Interface
    {
        Interface(const std::string& restApi, const HttpClientHelper& httpClientHelper = {});

        Interface(const Interface&) = delete;
        Interface& operator=(const Interface&) = delete;

        Interface(Interface&&) = default;
        Interface& operator=(Interface&&) = default;

        Utility::Version GetVersion() const override;
        IRestClient::SearchResult Search(const SearchRequest& request) const override;
        std::optional<Manifest::Manifest> GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const override;
        std::vector<Manifest::Manifest> GetManifests(const std::string& packageId, const std::map<std::string_view, std::string>& params = {}) const override;
   
    protected:
        bool MeetsOptimizedSearchCriteria(const SearchRequest& request) const;
        IRestClient::SearchResult OptimizedSearch(const SearchRequest& request) const;
        IRestClient::SearchResult SearchInternal(const SearchRequest& request) const;

    private:
        std::string m_restApiUri;
        utility::string_t m_searchEndpoint;
        std::unordered_map<utility::string_t, utility::string_t> m_requiredRestApiHeaders;
        HttpClientHelper m_httpClientHelper;
    };
}
