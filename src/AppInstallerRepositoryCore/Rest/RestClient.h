// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <set>
#include <cpprest/json.h>
#include "Rest/Schema/IRestClient.h"
#include "Rest/Schema/HttpClientHelper.h"
#include "cpprest/json.h"
#include "ISource.h"

namespace AppInstaller::Repository::Rest
{
    struct RestClient
    {
        RestClient(const RestClient&) = delete;
        RestClient& operator=(const RestClient&) = delete;

        RestClient(RestClient&&) = default;
        RestClient& operator=(RestClient&&) = default;

        // Performs a search based on the given criteria.
        Schema::IRestClient::SearchResult Search(const SearchRequest& request) const;

        std::optional<Manifest::Manifest> GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const;

        std::string GetSourceIdentifier() const;

        Schema::IRestClient::Information GetSourceInformation() const;

        static std::optional<AppInstaller::Utility::Version> GetLatestCommonVersion(const std::vector<std::string>& serverSupportedVersions, const std::set<AppInstaller::Utility::Version>& wingetSupportedVersions);

        static Schema::IRestClient::Information GetInformation(const utility::string_t& restApi, const std::unordered_map<utility::string_t, utility::string_t>& additionalHeaders, const Schema::HttpClientHelper& httpClientHelper);

        static std::unique_ptr<Schema::IRestClient> GetSupportedInterface(const std::string& restApi, const std::unordered_map<utility::string_t, utility::string_t>& additionalHeaders, const Schema::IRestClient::Information& information, const AppInstaller::Utility::Version& version);

        static RestClient Create(const std::string& restApi, std::optional<std::string> customHeader, std::string_view caller, const Schema::HttpClientHelper& helper = {});
    private:
        RestClient(std::unique_ptr<Schema::IRestClient> supportedInterface, std::string sourceIdentifier);

        std::unique_ptr<Schema::IRestClient> m_interface;
        std::string m_sourceIdentifier;
    };
}
