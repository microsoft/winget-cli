// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/IRestClient.h"
#include "cpprest/json.h"

namespace AppInstaller::Repository::Rest
{
    struct RestClient
    {
        RestClient(std::unique_ptr<Schema::IRestClient> supportedInterface);

        // The return type of Search
        using SearchResult = Rest::Schema::IRestClient::SearchResult;

        RestClient(const RestClient&) = delete;
        RestClient& operator=(const RestClient&) = delete;

        RestClient(RestClient&&) = default;
        RestClient& operator=(RestClient&&) = default;

        // Performs a search based on the given criteria.
        Schema::IRestClient::SearchResult Search(const SearchRequest& request) const;

        std::optional<Manifest::Manifest> GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const;

        static utility::string_t GetInformationEndpoint(const std::string& restApiUri);

        static std::string GetSupportedVersion(const std::string& restApi);

        static std::unique_ptr<Schema::IRestClient> GetSupportedInterface(const std::string& restApi, const std::string& version);

        static RestClient Create(const std::string& restApi);

    private:
        std::unique_ptr<Schema::IRestClient> m_interface;
    };
}
