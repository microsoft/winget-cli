// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <set>
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

        static std::optional<AppInstaller::Utility::Version> GetLatestCommonVersion(const AppInstaller::Repository::Rest::Schema::IRestClient::Information& information, const std::set<AppInstaller::Utility::Version>& wingetSupportedVersions);

        static utility::string_t GetInformationEndpoint(const utility::string_t& restApiUri);
        
        static AppInstaller::Utility::Version GetSupportedVersion(const utility::string_t& restApi, const std::set<AppInstaller::Utility::Version>& wingetSupportedVersions);

        static std::unique_ptr<Schema::IRestClient> GetSupportedInterface(const std::string& restApi, const AppInstaller::Utility::Version& version);

        static RestClient Create(const std::string& restApi);

    private:
        std::unique_ptr<Schema::IRestClient> m_interface;
    };
}
