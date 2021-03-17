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
            json_body[JsonHelper::GetJsonKeyNameString(FetchAllManifests)] = web::json::value::string(L"true");

            return json_body;
        }

        std::string GetRestAPIBaseUri(std::string restApiUri)
        {
            if (!restApiUri.empty() && restApiUri.back() == '/')
            {
                restApiUri.pop_back();
            }

            return restApiUri;
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

    Interface::Interface(const std::string& restApi, const std::string& restApiVersion)
    {
        m_restApiUriVersion = restApiVersion;
        m_restApiUri = GetRestAPIBaseUri(restApi);
        m_searchEndpoint = GetSearchEndpoint(m_restApiUri);
    }

    IRestClient::SearchResult Interface::Search(const SearchRequest& request) const
    {
        // Optimzation
        if (MeetsOptimizedSearchCriteria(request))
        {
            return OptimizedSearch(request);
        }
        
        // TODO: Handle continuation token and Use APIVersion.
        HttpClientHelper clientHelper{ m_searchEndpoint };
        web::json::value jsonObject = clientHelper.HandlePost(GetSearchBody(request));

        SearchResponseDeserializer searchResponseDeserializer;
        std::optional<IRestClient::SearchResult> searchResult = searchResponseDeserializer.Deserialize(jsonObject);

        if (!searchResult.has_value())
        {
            return {};
        }

        return searchResult.value();
    }

    std::optional<Manifest::Manifest> Interface::GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const
    {
        // TODO: Use APIVersion.
        std::optional<Manifest::Manifest> manifest;
        HttpClientHelper clientHelper{ GetManifestByVersionEndpoint(m_restApiUri, packageId, version, channel) };
        web::json::value jsonObject = clientHelper.HandleGet();

        // Parse json and return Manifest
        std::optional<std::reference_wrapper<const web::json::value>> data =
            JsonHelper::GetJsonValueFromNode(jsonObject, JsonHelper::GetJsonKeyNameString(Data));
        
        if (data.has_value())
        {
            ManifestDeserializer manifestDeserializer;
            manifest = manifestDeserializer.Deserialize(data.value().get());

            // Manifest validation
            if (manifest.has_value())
            {
                std::vector<AppInstaller::Manifest::ValidationError> validationErrors =
                    AppInstaller::Manifest::ValidateManifest(manifest.value());

                if (validationErrors.size() > 0)
                {
                    AICLI_LOG(Repo, Verbose, << "Recieved invalid manifest. Skipping");
                    return {};
                }
            }
        }

        return manifest;
    }

    bool Interface::MeetsOptimizedSearchCriteria(const SearchRequest& request) const
    {
        if (!request.Query.has_value() && request.Inclusions.size() == 0 &&
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
        std::optional<Manifest::Manifest> manifestResult = GetManifestByVersion(request.Filters[0].Value, {}, {});

        if (manifestResult.has_value())
        {
            Manifest::Manifest manifest = std::move(manifestResult.value());
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
}
