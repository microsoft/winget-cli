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

        // Responsible for getting the source information contracts with minimal validation. Does not try to create a rest interface out of it.
        static Schema::IRestClient::Information GetInformation(const std::string& restApi, std::optional<std::string> customHeader, std::string_view caller, const Schema::HttpClientHelper& helper = {});

        static std::unique_ptr<Schema::IRestClient> GetSupportedInterface(const std::string& restApi, const Schema::HttpClientHelper::HttpRequestHeaders& additionalHeaders, const Schema::IRestClient::Information& information, const Authentication::AuthenticationArguments& authArgs, const AppInstaller::Utility::Version& version);

        // Creates the rest client. Full validation performed (just as opening the source)
        static RestClient Create(const std::string& restApi, std::optional<std::string> customHeader, std::string_view caller, const Authentication::AuthenticationArguments& authArgs = {}, const Schema::HttpClientHelper& helper = {});
    private:
        RestClient(std::unique_ptr<Schema::IRestClient> supportedInterface, std::string sourceIdentifier);

        std::unique_ptr<Schema::IRestClient> m_interface;
        std::string m_sourceIdentifier;
    };
}
