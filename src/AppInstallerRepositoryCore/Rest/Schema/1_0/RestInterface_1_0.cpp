// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MatchCriteriaResolver.h"
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
        constexpr std::string_view MarketQueryParam = "Market"sv;

        bool CanResolveWithManifest(const PackageMatchFilter& filter)
        {
            switch (filter.Type)
            {
            case MatchType::Exact:
            case MatchType::CaseInsensitive:
            case MatchType::StartsWith:
            case MatchType::Substring:
                break;
            default:
                return false;
            }

            switch (filter.Field)
            {
            case PackageMatchField::Name:
            case PackageMatchField::Moniker:
            case PackageMatchField::Tag:
            case PackageMatchField::Command:
            case PackageMatchField::PackageFamilyName:
            case PackageMatchField::ProductCode:
            case PackageMatchField::UpgradeCode:
                return true;
            default:
                return false;
            }
        }

        std::optional<bool> MatchesPackage(const PackageMatchFilter& filter, const IRestClient::Package& package)
        {
            if (filter.Field == PackageMatchField::Id)
            {
                return MatchesRequest(filter, package.PackageInformation.PackageIdentifier);
            }
            if (filter.Field == PackageMatchField::Name &&
                MatchesRequest(filter, package.PackageInformation.PackageName).value_or(false))
            {
                return true;
            }

            std::optional<bool> result = package.Versions.empty() ? std::nullopt : std::optional<bool>{ false };
            for (const auto& version : package.Versions)
            {
                const std::vector<std::string>* values = nullptr;
                switch (filter.Field)
                {
                case PackageMatchField::PackageFamilyName:
                    values = &version.PackageFamilyNames;
                    break;
                case PackageMatchField::ProductCode:
                    values = &version.ProductCodes;
                    break;
                case PackageMatchField::UpgradeCode:
                    values = &version.UpgradeCodes;
                    break;
                }

                if (values && std::any_of(values->begin(), values->end(), [&](const auto& value)
                    {
                        return !value.empty() && MatchesRequest(filter, value).value_or(false);
                    }))
                {
                    return true;
                }

                auto match = version.Manifest ? MatchesRequest(filter, version.Manifest.value()) : std::nullopt;
                if (match && match.value())
                {
                    return true;
                }
                if (!match)
                {
                    result = std::nullopt;
                }
            }

            return result;
        }

        std::vector<IRestClient::VersionInfo> CreateVersionInfos(std::vector<Manifest::Manifest> manifests)
        {
            std::vector<IRestClient::VersionInfo> versions;
            versions.reserve(manifests.size());
            for (auto& manifest : manifests)
            {
                auto packageFamilyNames = manifest.GetPackageFamilyNames();
                auto productCodes = manifest.GetProductCodes();
                auto arpVersionRange = manifest.GetArpVersionRange();
                auto upgradeCodes = manifest.GetUpgradeCodes();
                AppInstaller::Utility::VersionAndChannel versionAndChannel{ manifest.Version, manifest.Channel };

                versions.emplace_back(
                    IRestClient::VersionInfo{
                        std::move(versionAndChannel),
                        std::move(manifest),
                        std::vector<std::string>{ packageFamilyNames.begin(), packageFamilyNames.end() },
                        std::vector<std::string>{ productCodes.begin(), productCodes.end() },
                        arpVersionRange.IsEmpty() ? std::vector<Utility::Version>{} : std::vector<Utility::Version>{ arpVersionRange.GetMinVersion(), arpVersionRange.GetMaxVersion() },
                        std::vector<std::string>{ upgradeCodes.begin(), upgradeCodes.end() } });
            }

            return versions;
        }

        std::optional<Manifest::Manifest> FindManifestByVersionAndChannel(
            const std::vector<Manifest::Manifest>& manifests, std::string_view version, std::string_view channel)
        {
            for (const auto& manifest : manifests)
            {
                if (Utility::CaseInsensitiveEquals(manifest.Version, version) &&
                    Utility::CaseInsensitiveEquals(manifest.Channel, channel))
                {
                    return manifest;
                }
            }

            return std::nullopt;
        }

        void PopulateManifestCache(IRestClient::Package& package, std::vector<Manifest::Manifest> manifests)
        {
            Utility::NormalizedString packageIdentifier = package.PackageInformation.PackageIdentifier;
            for (const auto& manifest : manifests)
            {
                if (!Utility::ICUCaseInsensitiveEquals(manifest.Id, packageIdentifier))
                {
                    AICLI_LOG(Repo, Error, << "Manifest response identifier '" << manifest.Id <<
                        "' does not match '" << package.PackageInformation.PackageIdentifier << "'.");
                    THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
                }
            }

            if (!manifests.empty() && package.Versions.size() == 1 &&
                package.Versions[0].VersionAndChannel.GetVersion().IsUnknown())
            {
                package.Versions = CreateVersionInfos(std::move(manifests));
            }
            else
            {
                for (auto& version : package.Versions)
                {
                    if (!version.Manifest)
                    {
                        version.Manifest = FindManifestByVersionAndChannel(manifests,
                            version.VersionAndChannel.GetVersion().ToString(), version.VersionAndChannel.GetChannel().ToString());
                    }
                }
            }
        }

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

        SearchResult result = SearchInternal(request);

        // Some sources (including msstore) may not return exact package identifier matches for substring ID requests.
        // Preserve substring semantics, but if no match was found for a single-ID request, retry through optimized exact lookup.
        if (result.Matches.empty() && MeetsOptimizedSearchCriteria(request, true))
        {
            SearchRequest optimizedRequest = request;
            optimizedRequest.Filters[0].Type = MatchType::CaseInsensitive;

            AICLI_LOG(Repo, Verbose, << "No search results for ID substring request; retrying with optimized exact ID lookup.");
            return OptimizedSearch(optimizedRequest);
        }

        return result;
    }

    IRestClient::SearchResult Interface::SearchInternal(const SearchRequest& request) const
    {
        SearchRequest validatedRequest = GetValidatedSearchRequest(request);
        if (!validatedRequest.Query && !request.Inclusions.empty() && validatedRequest.Inclusions.empty())
        {
            AICLI_LOG(Repo, Info, << "No supported inclusions remain in the search request.");
            return {};
        }

        constexpr size_t c_manifestRetrievalResultLimit = 3;
        // Probe beyond the enrichment threshold even when the caller requests fewer results.
        if (validatedRequest.Purpose == SearchPurpose::Default && validatedRequest.MaximumResults &&
            (std::any_of(validatedRequest.Filters.begin(), validatedRequest.Filters.end(), CanResolveWithManifest) ||
                (!validatedRequest.Query && std::any_of(validatedRequest.Inclusions.begin(), validatedRequest.Inclusions.end(), CanResolveWithManifest))))
        {
            validatedRequest.MaximumResults = std::max(validatedRequest.MaximumResults, c_manifestRetrievalResultLimit + 1);
        }

        const auto searchBody = SearchRequestComposer{ GetVersion() }.Serialize(validatedRequest);
        size_t candidateCount = 0;
        bool manifestRetrievalDeferred = false;
        SearchResult results;
        utility::string_t continuationToken;
        std::set<utility::string_t> usedContinuationTokens;
        Http::HttpClientHelper::HttpRequestHeaders searchHeaders = m_requiredRestApiHeaders;
        do
        {
            if (!continuationToken.empty())
            {
                if (!usedContinuationTokens.emplace(continuationToken).second)
                {
                    AICLI_LOG(Repo, Error, << "REST source returned a repeated continuation token.");
                    THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
                }

                AICLI_LOG(Repo, Verbose, << "Received continuation token. Retrieving more results.");
                searchHeaders.insert_or_assign(AppInstaller::JSON::GetUtilityString(ContinuationToken), continuationToken);
            }

            std::optional<web::json::value> jsonObject = m_httpClientHelper.HandlePost(m_searchEndpoint, searchBody, searchHeaders, GetAuthHeaders(), CustomRestCallResponseHandler);

            utility::string_t ct;
            if (jsonObject)
            {
                SearchResult currentResult = GetSearchResult(jsonObject.value());
                candidateCount += currentResult.Matches.size();
                manifestRetrievalDeferred |= FilterSearchResult(validatedRequest, currentResult, false);
                std::move(currentResult.Matches.begin(), currentResult.Matches.end(), std::inserter(results.Matches, results.Matches.end()));
                ct = GetContinuationToken(jsonObject.value()).value_or(L"");
            }

            continuationToken = ct;

        } while (!continuationToken.empty() && (!request.MaximumResults || results.Matches.size() < request.MaximumResults ||
            (manifestRetrievalDeferred && candidateCount <= c_manifestRetrievalResultLimit)));

        if (manifestRetrievalDeferred)
        {
            if (continuationToken.empty() && candidateCount <= c_manifestRetrievalResultLimit)
            {
                FilterSearchResult(validatedRequest, results, true);
            }
            else
            {
                AICLI_LOG(Repo, Verbose, << "Skipping manifest retrieval because the search exceeds the complete-result limit.");
            }
        }

        if (request.MaximumResults && results.Matches.size() > request.MaximumResults)
        {
            results.Matches.erase(std::next(results.Matches.begin(), request.MaximumResults), results.Matches.end());
            results.Truncated = true;
        }

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

    bool Interface::FilterSearchResult(const SearchRequest& request, SearchResult& result, bool allowManifestRetrieval) const
    {
        bool manifestRetrievalDeferred = false;
        std::vector<Package> matches;
        matches.reserve(result.Matches.size());
        for (auto& package : result.Matches)
        {
            bool retrievalAttempted = false;
            auto matchesField = [&](const PackageMatchFilter& filter)
            {
                return MatchesPackage(filter, package);
            };

            std::function<std::optional<bool>(const PackageMatchFilter&)> resolveField;
            if (request.Purpose == SearchPurpose::Default)
            {
                resolveField = [&](const PackageMatchFilter& filter) -> std::optional<bool>
                {
                    if (!CanResolveWithManifest(filter))
                    {
                        return std::nullopt;
                    }

                    if (!allowManifestRetrieval)
                    {
                        manifestRetrievalDeferred = true;
                        return std::nullopt;
                    }

                    if (!retrievalAttempted)
                    {
                        retrievalAttempted = true;
                        // Use the search market rather than defaulting the manifest lookup to the OS region.
                        std::map<std::string_view, std::string> queryParams;
                        for (const auto& requestFilter : request.Filters)
                        {
                            if (requestFilter.Field == PackageMatchField::Market)
                            {
                                auto [market, inserted] = queryParams.emplace(MarketQueryParam, requestFilter.Value);
                                if ((requestFilter.Type != MatchType::Exact && requestFilter.Type != MatchType::CaseInsensitive) ||
                                    (!inserted && !Utility::ICUCaseInsensitiveEquals(market->second, requestFilter.Value)))
                                {
                                    AICLI_LOG(Repo, Info, << "Manifest lookup cannot represent the requested market filters.");
                                    return std::nullopt;
                                }
                            }
                        }
                        try
                        {
                            queryParams = GetValidatedQueryParams(queryParams);
                        }
                        catch (const UnsupportedRequestException& e)
                        {
                            AICLI_LOG(Repo, Info, << "Manifest lookup cannot validate search metadata for " <<
                                package.PackageInformation.PackageIdentifier << ": " << e.what());
                            return std::nullopt;
                        }

                        AICLI_LOG(Repo, Verbose, << "Retrieving manifests to validate search criteria for " << package.PackageInformation.PackageIdentifier);
                        auto manifests = GetManifestsInternal(package.PackageInformation.PackageIdentifier, queryParams);
                        PopulateManifestCache(package, std::move(manifests));
                    }

                    return matchesField(filter);
                };
            }

            // Only proven mismatches are removed; retained NormalizedNameAndPublisher candidates are unvalidated.
            auto match = MatchesRequest(request, matchesField, resolveField);
            if (match && !match.value())
            {
                AICLI_LOG(Repo, Verbose, << "Discarding REST package " << package.PackageInformation.PackageIdentifier <<
                    ": does not match search request " << request.ToString());
                continue;
            }
            matches.emplace_back(std::move(package));
        }
        result.Matches = std::move(matches);
        return manifestRetrievalDeferred;
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

        return FindManifestByVersionAndChannel(GetManifests(packageId, queryParams), version, channel);
    }

    bool Interface::MeetsOptimizedSearchCriteria(const SearchRequest& request, bool allowSubstringMatch) const
    {
        // Optimization: If the user wants to install a certain package with an exact match on package id and a particular rest source, we will
        // call the package manifest endpoint to get the manifest directly instead of running a search for it.
        if (!request.Query && request.Inclusions.size() == 0 &&
            request.Filters.size() == 1 && request.Filters[0].Field == PackageMatchField::Id)
        {
            MatchType matchType = request.Filters[0].Type;

            if (matchType == MatchType::Exact || matchType == MatchType::CaseInsensitive ||
                (allowSubstringMatch && matchType == MatchType::Substring))
            {
                AICLI_LOG(Repo, Verbose, << "Search request meets optimized search criteria.");
                return true;
            }
        }

        return false;
    }

    IRestClient::SearchResult Interface::OptimizedSearch(const SearchRequest& request) const
    {
        SearchResult searchResult;
        const auto& idFilter = request.Filters[0];
        std::vector<Manifest::Manifest> manifests = GetManifests(idFilter.Value);

        if (!manifests.empty())
        {
            auto& manifest = manifests.at(0);
            if (MatchesRequest(idFilter, manifest.Id) == false)
            {
                AICLI_LOG(Repo, Error, << "Manifest response identifier '" << manifest.Id <<
                    "' does not match search request " << request.ToString());
                THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
            }

            PackageInfo packageInfo = PackageInfo{
                manifest.Id,
                manifest.DefaultLocalization.Get<AppInstaller::Manifest::Localization::PackageName>(),
                manifest.DefaultLocalization.Get<AppInstaller::Manifest::Localization::Publisher>() };

            Package package = Package{ std::move(packageInfo), CreateVersionInfos(std::move(manifests)) };
            searchResult.Matches.emplace_back(std::move(package));
        }

        return searchResult;
    }

    std::vector<Manifest::Manifest> Interface::GetManifests(const std::string& packageId, const std::map<std::string_view, std::string>& params) const
    {
        return GetManifestsInternal(packageId, GetValidatedQueryParams(params));
    }

    std::vector<Manifest::Manifest> Interface::GetManifestsInternal(const std::string& packageId, const std::map<std::string_view, std::string>& validatedParams) const
    {
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
                AppInstaller::Manifest::ValidateManifest(manifestItem, AppInstaller::Manifest::ManifestValidateOption{ false });

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

    SearchRequest Interface::GetValidatedSearchRequest(const SearchRequest& searchRequest) const
    {
        return searchRequest;
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
