// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <set>
#include <cpprest/json.h>
#include "Rest/Schema/IRestClient.h"
#include "Rest/Schema/HttpClientHelper.h"
#include "cpprest/json.h"

namespace AppInstaller::Repository::Rest
{
    struct RestClient
    {
        // The return type of Search
        using SearchResult = Rest::Schema::IRestClient::SearchResult;

        RestClient(const RestClient&) = delete;
        RestClient& operator=(const RestClient&) = delete;

        RestClient(RestClient&&) = default;
        RestClient& operator=(RestClient&&) = default;

        // Performs a search based on the given criteria.
        Schema::IRestClient::SearchResult Search(const SearchRequest& request) const;

        std::optional<Manifest::Manifest> GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const;

        std::string GetSourceIdentifier() const;

        static std::optional<AppInstaller::Utility::Version> GetLatestCommonVersion(const std::vector<std::string>& serverSupportedVersions, const std::set<AppInstaller::Utility::Version>& wingetSupportedVersions);

        static Schema::IRestClient::Information GetInformation(const utility::string_t& restApi, const Schema::HttpClientHelper& httpClientHelper);

        static std::unique_ptr<Schema::IRestClient> GetSupportedInterface(const std::string& restApi, const Schema::IRestClient::Information& information, const AppInstaller::Utility::Version& version);

        static RestClient Create(const std::string& restApi, const Schema::HttpClientHelper& helper = {});

    private:
        RestClient(std::unique_ptr<Schema::IRestClient> supportedInterface, std::string sourceIdentifier, Schema::IRestClient::Information&& information);

        std::unique_ptr<Schema::IRestClient> m_interface;
        std::string m_sourceIdentifier;
        Schema::IRestClient::Information m_information;
    };
}
