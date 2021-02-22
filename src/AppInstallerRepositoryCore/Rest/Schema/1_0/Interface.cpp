// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/Schema/IRestClient.h"
#include "Rest/HttpClientHelper.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"

using namespace web;
using namespace web::json;
using namespace web::http;
using namespace web::http::client;
using namespace AppInstaller::Repository::Rest::Schema;

namespace AppInstaller::Repository::Rest::Schema::V1_0
{
    // Endpoint constants
    const std::string ManifestSearchPostEndpoint = "/api/manifestSearch?";

    // General API response constants
    const std::basic_string Data = L"data";

    // Search body constants
    const std::basic_string FetchAllManifests = L"fetchAllManifests";

    // Search response constants
    const std::basic_string PackageIdentifier = L"PackageIdentifier";
    const std::basic_string PackageName = L"PackageName";
    const std::basic_string Publisher = L"Publisher";
    const std::basic_string PackageFamilyName = L"PackageFamilyName";
    const std::basic_string ProductCode = L"ProductCode";
    const std::basic_string Versions = L"Versions";
    const std::basic_string Version = L"version";
    const std::basic_string Channel = L"Channel";

    namespace
    {
        json::value GetSearchBody(const SearchRequest& searchRequest)
        {
            // TODO: Use search request to construct search body.
            UNREFERENCED_PARAMETER(searchRequest);

            json::value json_body;
            json_body[FetchAllManifests] = web::json::value::string(L"true");

            return json_body;
        }

        std::string GetRestAPIBaseUri(const std::string& restApiUri)
        {
            std::string formattedUri = restApiUri;
            if (formattedUri.back() == '/')
            {
                formattedUri.pop_back();
            }

            return formattedUri;
        }

        utility::string_t GetSearchEndpoint(const std::string& restApiUri)
        {
            std::string fullSearchAPI = GetRestAPIBaseUri(restApiUri) + ManifestSearchPostEndpoint;
            utility::string_t searchAPI = utility::conversions::to_string_t(fullSearchAPI);
            return searchAPI;
        }

        utility::string_t GetManifestByVersionEndpoint(const std::string& restApiUri, const std::string& packageId, const std::string& version)
        {
            // TODO: Replace with manifest version endpoint 
            std::string versionEndpoint = GetRestAPIBaseUri(restApiUri) + "/api/packages/" + packageId + "/versions/" + version;
            utility::string_t versionApi = utility::conversions::to_string_t(versionEndpoint);
            return versionApi;
        }

        std::string GetStringFromJsonStringValue(const json::value& value)
        {
            return value.is_null() ? "" : utility::conversions::to_utf8string(value.as_string());
        }
    }

    IRestClient::SearchResult Interface::Search(const std::string& restApiUri, const SearchRequest& request) const
    {
        UNREFERENCED_PARAMETER(request);
        SearchResult result;

        // TODO: Handle continuation token.
        HttpClientHelper clientHelper(GetSearchEndpoint(restApiUri));
        json::value jsonObject = clientHelper.HandlePost(GetSearchBody(request));

        // Parse json and add results to SearchResult.
        if (jsonObject.is_null())
        {
            return result;
        }

        auto& dataArray = jsonObject.at(Data).as_array();

        for (auto& manifestItem : dataArray)
        {
            std::string packageId = GetStringFromJsonStringValue(manifestItem.at(PackageIdentifier));
            std::string packageName = GetStringFromJsonStringValue(manifestItem.at(PackageName));
            std::string publisher = GetStringFromJsonStringValue(manifestItem.at(Publisher));
            std::string packageFamilyName = GetStringFromJsonStringValue(manifestItem.at(PackageFamilyName));
            std::string productCode = GetStringFromJsonStringValue(manifestItem.at(ProductCode));
            json::value versionValue = manifestItem.at(Versions);

            if (packageId.empty() || packageName.empty() || publisher.empty() || versionValue.is_null() || versionValue.as_array().size() == 0)
            {
                continue;
            }

            std::vector<VersionAndChannel> versionList;
            for (auto& versionItem : versionValue.as_array())
            {
                std::string version = GetStringFromJsonStringValue(versionItem.at(Version));
                std::string channel = GetStringFromJsonStringValue(versionItem.at(Channel));
                versionList.emplace_back(VersionAndChannel(version, channel));
            }

            PackageInfo packageInfo = PackageInfo(packageId, packageName, publisher);
            Package package = Package(std::move(packageInfo), std::move(versionList));
            result.Matches.emplace_back(std::move(package));
        }

        return result;
    }

    std::optional<std::string> Interface::GetManifestByVersion(const std::string& restApiUri, const std::string& packageId, const std::string& version) const
    {
        HttpClientHelper clientHelper(GetManifestByVersionEndpoint(restApiUri, packageId, version));
        json::value jsonObject = clientHelper.HandleGet();

        if (jsonObject.is_null())
        {
            return std::string();
        }

        // Parse json and add results to SearchResult
        auto& manifestObject = jsonObject.at(Data);
        return manifestObject.is_null() ? std::string() : utility::conversions::to_utf8string(manifestObject.serialize());
    }
}
