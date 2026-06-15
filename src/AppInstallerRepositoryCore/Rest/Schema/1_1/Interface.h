// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/1_0/Interface.h"

namespace AppInstaller::Repository::Rest::Schema::V1_1
{
    // Interface to this schema version exposed through IRestClient.
    struct Interface : public V1_0::Interface
    {
        Interface(const std::string& restApi, const Http::HttpClientHelper& helper, IRestClient::Information information, const Http::HttpClientHelper::HttpRequestHeaders& additionalHeaders = {});

        Interface(const Interface&) = delete;
        Interface& operator=(const Interface&) = delete;

        Interface(Interface&&) = default;
        Interface& operator=(Interface&&) = default;

        Utility::Version GetVersion() const override;
        IRestClient::Information GetSourceInformation() const override;

    protected:
        // Check query params against source information and update if necessary.
        std::map<std::string_view, std::string> GetValidatedQueryParams(const std::map<std::string_view, std::string>& params) const override;

        // Check search request against source information and get json search body.
        web::json::value GetValidatedSearchBody(const SearchRequest& searchRequest) const override;

        SearchResult GetSearchResult(const web::json::value& searchResponseObject) const override;
        std::vector<Manifest::Manifest> GetParsedManifests(const web::json::value& manifestsResponseObject) const override;

        PackageMatchField ConvertStringToPackageMatchField(std::string_view field) const;

        IRestClient::Information m_information;
    };
}
