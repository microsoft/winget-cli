// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/Schema/IRestClient.h"
#include <winget/HttpClientHelper.h>
#include <winget/JsonUtil.h>
#include <winget/ManifestJSONParser.h>
#include <winget/ManifestValidation.h>
#include <winget/Rest.h>
#include "Rest/Schema/CommonRestConstants.h"
#include "Rest/Schema/SearchResponseParser.h"
#include "Rest/Schema/SearchRequestComposer.h"

using namespace std::string_view_literals;

namespace AppInstaller::Repository::Rest::Schema::V1_0
{
    namespace
    {
        // Query params
        constexpr std::string_view VersionQueryParam = "Version"sv;
        constexpr std::string_view ChannelQueryParam = "Channel"sv;

        utility::string_t GetSearchEndpoint(const std::string& restApiUri)
        {
            return AppInstaller::Rest::AppendPathToUri(AppInstaller::JSON::GetUtilityString(restApiUri), AppInstaller::JSON::GetUtilityString(ManifestSearchPostEndpoint));
        }

        utility::string_t GetManifestByVersionEndpoint(
            const std::string& restApiUri, const std::string& packageId, const std::map<std::string_view, std::string>& queryParameters)
        {
            utility::string_t getManifestEndpoint = AppInstaller::Rest::AppendPathToUri(
                AppInstaller::JSON::GetUtilityString(restApiUri), AppInstaller::JSON::GetUtilityString(ManifestByVersionAndChannelGetEndpoint));

            utility::string_t getManifestWithPackageIdPath = AppInstaller::Rest::AppendPathToUri(getManifestEndpoint, AppInstaller::JSON::GetUtilityString(packageId));

            // Create the endpoint with query parameters
            return AppInstaller::Rest::AppendQueryParamsToUri(getManifestWithPackageIdPath, queryParameters);
        }

        std::optional<utility::string_t> GetContinuationToken(const web::json::value& jsonObject)
        {
            std::optional<std::string> continuationToken = AppInstaller::JSON::GetRawStringValueFromJsonNode(jsonObject, AppInstaller::JSON::GetUtilityString(ContinuationToken));

            if (continuationToken)
            {
                return utility::conversions::to_string_t(continuationToken.value());
            }

            return {};
        }

        AppInstaller::Http::HttpClientHelper::HttpResponseHandlerResult CustomRestCallResponseHandler(const web::http::http_response& response)
        {
            AppInstaller::Http::HttpClientHelper::HttpResponseHandlerResult result;
            result.UseDefaultHandling = true;

            if (response.status_code() == web::http::status_codes::NotFound &&
                response.headers().content_type()._Starts_with(web::http::details::mime_types::application_json))
            {
                auto responseJson = response.extract_json().get();
                if (responseJson.is_object() && responseJson.has_field(L"code") && responseJson.has_field(L"message"))
                {
                    // We'll treat 404 with json response containing code and message fields as empty result.
                    // Leave the HttpResponseHandlerResult result empty and disable default HttpClientHelper handling.
                    result.UseDefaultHandling = false;
                }
            }

            return result;
        }
    }

    Interface::Interface(const std::string& restApi, const Http::HttpClientHelper& httpClientHelper) : m_restApiUri(restApi), m_httpClientHelper(httpClientHelper)
    {
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL, !AppInstaller::Rest::IsValidUri(AppInstaller::JSON::GetUtilityString(restApi)));

        m_searchEndpoint = GetSearchEndpoint(m_restApiUri);
        m_requiredRestApiHeaders.emplace(AppInstaller::JSON::GetUtilityString(ContractVersion), AppInstaller::JSON::GetUtilityString(Version_1_0_0.ToString()));
    }

    Utility::Version Interface::GetVersion() const
    {
        return Version_1_0_0;
    }

    IRestClient::Information Interface::GetSourceInformation() const
    {
        return {};
    }

    IRestClient::SearchResult Interface::Search(const SearchRequest& request) const
    {
        // Optimization
        if (MeetsOptimizedSearchCriteria(request))
        {
            return OptimizedSearch(request);
        }

        return SearchInternal(request);
    }

    IRestClient::SearchResult Interface::SearchInternal(const SearchRequest& request) const
    {
        SearchResult results;
        utility::string_t continuationToken;
        Http::HttpClientHelper::HttpRequestHeaders searchHeaders = m_requiredRestApiHeaders;
        do
        {
            if (!continuationToken.empty())
            {
                AICLI_LOG(Repo, Verbose, << "Received continuation token. Retrieving more results.");
                searchHeaders.insert_or_assign(AppInstaller::JSON::GetUtilityString(ContinuationToken), continuationToken);
            }

            std::optional<web::json::value> jsonObject = m_httpClientHelper.HandlePost(m_searchEndpoint, GetValidatedSearchBody(request), searchHeaders, GetAuthHeaders(), CustomRestCallResponseHandler);

            utility::string_t ct;
            if (jsonObject)
            {
                SearchResult currentResult = GetSearchResult(jsonObject.value());

                size_t insertElements = !request.MaximumResults ? currentResult.Matches.size() :
                    std::min(currentResult.Matches.size(), request.MaximumResults - results.Matches.size());

                if (insertElements < currentResult.Matches.size())
                {
                    results.Truncated = true;
                }

                std::move(currentResult.Matches.begin(), std::next(currentResult.Matches.begin(), insertElements), std::inserter(results.Matches, results.Matches.end()));
                ct = GetContinuationToken(jsonObject.value()).value_or(L"");
            }

            continuationToken = ct;

        } while (!continuationToken.empty() && (!request.MaximumResults || results.Matches.size() < request.MaximumResults));

        if (!continuationToken.empty())
        {
            results.Truncated = true;
        }

        if (results.Matches.empty())
        {
            AICLI_LOG(Repo, Verbose, << "No search results returned by rest source");
        }

        return results;
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

        if (!manifests.empty())
        {
            for (Manifest::Manifest manifest : manifests)
            {
                if (Utility::CaseInsensitiveEquals(manifest.Version, version) &&
                    Utility::CaseInsensitiveEquals(manifest.Channel, channel))
                {
                    return manifest;
                }
            }
        }

        return {};
    }

    bool Interface::MeetsOptimizedSearchCriteria(const SearchRequest& request) const
    {
        // Optimization: If the user wants to install a certain package with an exact match on package id and a particular rest source, we will
        // call the package manifest endpoint to get the manifest directly instead of running a search for it.
        if (!request.Query && request.Inclusions.size() == 0 &&
            request.Filters.size() == 1 && request.Filters[0].Field == PackageMatchField::Id &&
            (request.Filters[0].Type == MatchType::Exact || request.Filters[0].Type == MatchType::CaseInsensitive))
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
                auto packageFamilyNames = manifestVersion.GetPackageFamilyNames();
                auto productCodes = manifestVersion.GetProductCodes();
                auto arpVersionRange = manifestVersion.GetArpVersionRange();
                auto upgradeCodes = manifestVersion.GetUpgradeCodes();

                versions.emplace_back(
                    VersionInfo{
                        AppInstaller::Utility::VersionAndChannel {manifestVersion.Version, manifestVersion.Channel},
                        manifestVersion,
                        std::vector<std::string>{ packageFamilyNames.begin(), packageFamilyNames.end()},
                        std::vector<std::string>{ productCodes.begin(), productCodes.end()},
                        arpVersionRange.IsEmpty() ? std::vector<Utility::Version>{} : std::vector<Utility::Version>{ arpVersionRange.GetMinVersion(), arpVersionRange.GetMaxVersion() },
                        std::vector<std::string>{ upgradeCodes.begin(), upgradeCodes.end()} });
            }

            Package package = Package{ std::move(packageInfo), std::move(versions) };
            searchResult.Matches.emplace_back(std::move(package));
        }

        return searchResult;
    }

    std::vector<Manifest::Manifest> Interface::GetManifests(const std::string& packageId, const std::map<std::string_view, std::string>& params) const
    {
        auto validatedParams = GetValidatedQueryParams(params);

        std::vector<Manifest::Manifest> results;
        utility::string_t continuationToken;
        Http::HttpClientHelper::HttpRequestHeaders searchHeaders = m_requiredRestApiHeaders;
        std::optional<web::json::value> jsonObject = m_httpClientHelper.HandleGet(GetManifestByVersionEndpoint(m_restApiUri, packageId, validatedParams), searchHeaders, GetAuthHeaders(), CustomRestCallResponseHandler);

        if (!jsonObject)
        {
            AICLI_LOG(Repo, Verbose, << "No results were returned by the rest source for package id: " << packageId);
            return results;
        }

        // Parse json and return Manifests
        std::vector<Manifest::Manifest> manifests = GetParsedManifests(jsonObject.value());

        // Manifest validation
        for (auto& manifestItem : manifests)
        {
            std::vector<AppInstaller::Manifest::ValidationError> validationErrors =
                AppInstaller::Manifest::ValidateManifest(manifestItem, false);

            int errors = 0;
            for (auto& error : validationErrors)
            {
                if (error.ErrorLevel == Manifest::ValidationError::Level::Error)
                {
                    AICLI_LOG(Repo, Error, << "Received manifest contains validation error: " << error.GetErrorMessage());
                    errors++;
                }
            }

            THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA, errors > 0);

            results.emplace_back(manifestItem);
        }

        return results;
    }

    std::map<std::string_view, std::string> Interface::GetValidatedQueryParams(const std::map<std::string_view, std::string>& params) const
    {
        return params;
    }

    web::json::value Interface::GetValidatedSearchBody(const SearchRequest& searchRequest) const
    {
        SearchRequestComposer searchRequestComposer{ GetVersion() };
        return searchRequestComposer.Serialize(searchRequest);
    }

    IRestClient::SearchResult Interface::GetSearchResult(const web::json::value& searchResponseObject) const
    {
        SearchResponseParser searchResponseParser{ GetVersion() };
        return searchResponseParser.Deserialize(searchResponseObject);
    }

    std::vector<Manifest::Manifest> Interface::GetParsedManifests(const web::json::value& manifestsResponseObject) const
    {
        JSON::ManifestJSONParser manifestParser{ GetVersion() };
        return manifestParser.Deserialize(manifestsResponseObject);
    }

    Http::HttpClientHelper::HttpRequestHeaders Interface::GetAuthHeaders() const
    {
        return {};
    }
}
