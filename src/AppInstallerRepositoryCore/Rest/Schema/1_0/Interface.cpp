// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/Schema/IRestClient.h"
#include "Rest/HttpClientHelper.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "Rest/Schema/Json/JsonHelper.h"
#include "Rest/Schema/Json/CommonRestConstants.h"
#include "Rest/Schema/Json/ManifestDeserializer.h"
#include "Rest/Schema/RestHelper.h"
#include "Rest/Schema/Json/SearchResponseDeserializer.h"
#include "winget/ManifestValidation.h"

using namespace std::string_view_literals;
using namespace AppInstaller::Repository::Rest::Schema::Json;

namespace AppInstaller::Repository::Rest::Schema::V1_0
{
    // Endpoint constants
    constexpr std::string_view ManifestSearchPostEndpoint = "/manifestSearch"sv;
    constexpr std::string_view ManifestByVersionAndChannelGetEndpoint = "/packageManifests/"sv;

    // Search body constants
    constexpr std::string_view FetchAllManifests = "fetchAllManifests"sv;

    namespace
    {
        web::json::value GetSearchBody(const SearchRequest& searchRequest)
        {
            // TODO: Use search request to construct search body.
            UNREFERENCED_PARAMETER(searchRequest);

            web::json::value json_body;
            json_body[JsonHelper::GetUtilityString(FetchAllManifests)] = web::json::value::string(L"true");

            return json_body;
        }

        utility::string_t GetSearchEndpoint(const std::string& restApiUri)
        {
            std::string fullSearchAPI = restApiUri;
            return utility::conversions::to_string_t(fullSearchAPI.append(ManifestSearchPostEndpoint));
        }

        utility::string_t GetManifestByVersionEndpoint(
            const std::string& restApiUri, const std::string& packageId, const std::string& version, const std::string& channel)
        {
            std::string versionEndpoint = restApiUri;
            versionEndpoint.append(ManifestByVersionAndChannelGetEndpoint).append(packageId);

            // Add Version Query param
            // TODO: Encode the URL.
            if (!version.empty())
            {
                versionEndpoint.append("?version=").append(version);
            }

            // Add Channel Query param
            if (version.empty() && !channel.empty())
            {
                versionEndpoint.append("?channel=").append(channel);
            }
            else if (!channel.empty())
            {
                versionEndpoint.append("&channel=").append(channel);
            }

            return utility::conversions::to_string_t(versionEndpoint);
        }
    }

    Interface::Interface(const std::string& restApi)
    {
        m_restApiUri = RestHelper::GetRestAPIBaseUri(restApi);
        m_searchEndpoint = GetSearchEndpoint(m_restApiUri);
        m_requiredRestApiHeaders.emplace_back(
            std::pair(JsonHelper::GetUtilityString(ContractVersion), JsonHelper::GetUtilityString(GetVersion())));
    }

    std::string Interface::GetVersion() const
    {
        // TODO: Change type to Version if necessary.
        return "1.0.0";
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
        web::json::value jsonObject = clientHelper.HandlePost(GetSearchBody(request), m_requiredRestApiHeaders);

        SearchResponseDeserializer searchResponseDeserializer;
        return searchResponseDeserializer.Deserialize(jsonObject);
    }

    std::optional<Manifest::Manifest> Interface::GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const
    {
        std::vector<Manifest::Manifest> manifests = GetManifests(packageId, version, channel);

        // TODO: Handle multiple manifest selection.
        if (manifests.size() > 0)
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
        // TODO: Send in VersionLatest = true query param.
        SearchResult searchResult;
        std::vector<Manifest::Manifest> manifests = GetManifests(request.Filters[0].Value, {}, {});

        if (manifests.size() > 0)
        {
            // TODO: After adding the VersionLatest query param, we should be expecting one or no version. Using the first one until then.
            Manifest::Manifest manifest = manifests.at(0);
            PackageInfo packageInfo = PackageInfo{
                manifest.Id,
                manifest.DefaultLocalization.Get<AppInstaller::Manifest::Localization::PackageName>(),
                manifest.DefaultLocalization.Get<AppInstaller::Manifest::Localization::Publisher>() };

            std::vector<VersionInfo> versions;
            versions.emplace_back(
                VersionInfo{ AppInstaller::Utility::VersionAndChannel {manifest.Version, manifest.Channel}, std::move(manifest) });

            Package package = Package{ std::move(packageInfo), std::move(versions) };
            searchResult.Matches.emplace_back(std::move(package));
        }

        return searchResult;
    }

    std::vector<Manifest::Manifest> Interface::GetManifests(const std::string& packageId, const std::string& version, const std::string& channel) const
    {
        // TODO: Make a list of query params supported instead of using function parameters.
        std::vector<Manifest::Manifest> results;
        HttpClientHelper clientHelper{ GetManifestByVersionEndpoint(m_restApiUri, packageId, version, channel) };
        web::json::value jsonObject = clientHelper.HandleGet(m_requiredRestApiHeaders);

        // Parse json and return Manifests
        ManifestDeserializer manifestDeserializer;
        std::vector<Manifest::Manifest> manifests = manifestDeserializer.Deserialize(jsonObject);

        // Manifest validation
        for (auto& manifestItem : manifests)
        {
            Manifest::Manifest manifest = manifestItem;
            std::vector<AppInstaller::Manifest::ValidationError> validationErrors =
                AppInstaller::Manifest::ValidateManifest(manifest);

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

            results.emplace_back(manifest);
        }

        return results;
    }
}
