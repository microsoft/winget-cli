// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/Schema/IRestClient.h"
#include "Rest/HttpClientHelper.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "Rest/Schema/JsonHelper.h"
#include "winget/ManifestValidation.h"
#include "Rest/Schema/RestHelper.h"
#include "Rest/Schema/CommonRestConstants.h"
#include "Rest/Schema/1_0/Json/CommonJsonConstants.h"
#include "Rest/Schema/1_0/Json/ManifestDeserializer.h"
#include "Rest/Schema/1_0/Json/SearchResponseDeserializer.h"
#include "Rest/Schema/1_0/Json/SearchRequestSerializer.h"

using namespace std::string_view_literals;
using namespace AppInstaller::Repository::Rest::Schema::V1_0::Json;

namespace AppInstaller::Repository::Rest::Schema::V1_0
{
    // Endpoint constants
    constexpr std::string_view ManifestSearchPostEndpoint = "/manifestSearch"sv;
    constexpr std::string_view ManifestByVersionAndChannelGetEndpoint = "/packageManifests/"sv;

    // Query params
    constexpr std::string_view VersionQueryParam = "Version"sv;
    constexpr std::string_view ChannelQueryParam = "Channel"sv;

    namespace
    {
        web::json::value GetSearchBody(const SearchRequest& searchRequest)
        {
            SearchRequestSerializer serializer;
            return serializer.Serialize(searchRequest);
        }

        utility::string_t GetSearchEndpoint(const std::string& restApiUri)
        {
            return RestHelper::AppendPathToUri(JsonHelper::GetUtilityString(restApiUri), JsonHelper::GetUtilityString(ManifestSearchPostEndpoint));
        }

        utility::string_t GetManifestByVersionEndpoint(
            const std::string& restApiUri, const std::string& packageId, const std::map<std::string_view, std::string>& queryParameters)
        {
            utility::string_t versionEndpoint = RestHelper::AppendPathToUri(
                JsonHelper::GetUtilityString(restApiUri), JsonHelper::GetUtilityString(ManifestByVersionAndChannelGetEndpoint));

            utility::string_t packageIdPath = RestHelper::AppendPathToUri(versionEndpoint, JsonHelper::GetUtilityString(packageId));

            // Create the endpoint with query parameters
            return RestHelper::AppendQueryParamsToUri(packageIdPath, queryParameters);
        }
    }

    Interface::Interface(const std::string& restApi)
    {
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL, !RestHelper::IsValidUri(JsonHelper::GetUtilityString(restApi)));

        m_restApiUri = restApi;
        m_searchEndpoint = GetSearchEndpoint(m_restApiUri);
        m_requiredRestApiHeaders.emplace_back(
            std::pair(JsonHelper::GetUtilityString(ContractVersion), JsonHelper::GetUtilityString(GetVersion().ToString())));
    }

    Utility::Version Interface::GetVersion() const
    {
        return Version_1_0_0;
    }

    IRestClient::SearchResult Interface::Search(const SearchRequest& request) const
    {
        // Optimization
        if (MeetsOptimizedSearchCriteria(request))
        {
            return OptimizedSearch(request);
        }

        // TODO: Handle continuation token
        HttpClientHelper clientHelper{ m_searchEndpoint };
        std::optional<web::json::value> jsonObject = clientHelper.HandlePost(GetSearchBody(request), m_requiredRestApiHeaders);

        if (!jsonObject)
        {
            AICLI_LOG(Repo, Verbose, << "No search results returned by rest source");
            return {};
        }

        SearchResponseDeserializer searchResponseDeserializer;
        return searchResponseDeserializer.Deserialize(jsonObject.value());
    }

    std::optional<Manifest::Manifest> Interface::GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const
    {
        std::map<std::string_view, std::string> queryParams;
        if (!version.empty())
        {
            queryParams.emplace(VersionQueryParam, version);
        }

        if (!channel.empty())
        {
            queryParams.emplace(ChannelQueryParam, channel);
        }

        std::vector<Manifest::Manifest> manifests = GetManifests(packageId, queryParams);

        // TODO: Handle multiple manifest selection.
        if (!manifests.empty())
        {
            return manifests.at(0);
        }

        return {};
    }

    bool Interface::MeetsOptimizedSearchCriteria(const SearchRequest& request) const
    {
        if (!request.Query && request.Inclusions.size() == 0 &&
            request.Filters.size() == 1 && request.Filters[0].Field == PackageMatchField::Id &&
            request.Filters[0].Type == MatchType::Exact)
        {
            AICLI_LOG(Repo, Verbose, << "Search request meets optimized search criteria.");
            return true;
        }

        return false;
    }

    IRestClient::SearchResult Interface::OptimizedSearch(const SearchRequest& request) const
    {
        SearchResult searchResult;
        std::vector<Manifest::Manifest> manifests = GetManifests(request.Filters[0].Value);

        if (!manifests.empty())
        {
            auto& manifest = manifests.at(0);
            PackageInfo packageInfo = PackageInfo{
                manifest.Id,
                manifest.DefaultLocalization.Get<AppInstaller::Manifest::Localization::PackageName>(),
                manifest.DefaultLocalization.Get<AppInstaller::Manifest::Localization::Publisher>() };

            // Add all the versions to the package info object
            std::vector<VersionInfo> versions;
            for (auto& manifestVersion : manifests)
            {
                versions.emplace_back(
                    VersionInfo{ AppInstaller::Utility::VersionAndChannel {manifestVersion.Version, manifestVersion.Channel}, manifestVersion });
            }

            Package package = Package{ std::move(packageInfo), std::move(versions) };
            searchResult.Matches.emplace_back(std::move(package));
        }

        return searchResult;
    }

    std::vector<Manifest::Manifest> Interface::GetManifests(const std::string& packageId, const std::map<std::string_view, std::string>& params) const
    {
        std::vector<Manifest::Manifest> results;
        HttpClientHelper clientHelper{ GetManifestByVersionEndpoint(m_restApiUri, packageId, params) };
        std::optional<web::json::value> jsonObject = clientHelper.HandleGet(m_requiredRestApiHeaders);

        if (!jsonObject)
        {
            AICLI_LOG(Repo, Verbose, << "No results were returned by the rest source for package id: " << packageId);
            return results;
        }

        // Parse json and return Manifests
        ManifestDeserializer manifestDeserializer;
        std::vector<Manifest::Manifest> manifests = manifestDeserializer.Deserialize(jsonObject.value());

        // Manifest validation
        for (auto& manifestItem : manifests)
        {
            std::vector<AppInstaller::Manifest::ValidationError> validationErrors =
                AppInstaller::Manifest::ValidateManifest(manifestItem);

            int errors = 0;
            for (auto& error : validationErrors)
            {
                if (error.ErrorLevel == Manifest::ValidationError::Level::Error)
                {
                    AICLI_LOG(Repo, Error, << "Received manifest contains validation error: " << error.Message);
                    errors++;
                }
            }

            THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA, errors > 0);

            results.emplace_back(manifestItem);
        }

        return results;
    }
}
