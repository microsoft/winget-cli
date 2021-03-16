// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/Schema/IRestClient.h"
#include "Rest/HttpClientHelper.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"

using namespace std::string_view_literals;

namespace AppInstaller::Repository::Rest::Schema::V1_0
{
    // Endpoint constants
    constexpr std::string_view ManifestSearchPostEndpoint = "/api/manifestSearch?"sv;
    constexpr std::string_view ManifestByVersionAndChannelGetEndpoint = "/api/packageManifests/"sv;

    // General API response constants
    constexpr std::string_view Data = "data"sv;

    // Search body constants
    constexpr std::string_view FetchAllManifests = "fetchAllManifests"sv;

    // Search response constants
    constexpr std::string_view PackageIdentifier = "PackageIdentifier"sv;
    constexpr std::string_view PackageName = "PackageName"sv;
    constexpr std::string_view Publisher = "Publisher"sv;
    constexpr std::string_view PackageFamilyName = "PackageFamilyName"sv;
    constexpr std::string_view ProductCode = "ProductCode"sv;
    constexpr std::string_view Versions = "Versions"sv;
    constexpr std::string_view Version = "version"sv;
    constexpr std::string_view Channel = "Channel"sv;

    namespace
    {
        utility::string_t GetJsonKeyNameString(std::string_view nodeName)
        {
            return utility::conversions::to_string_t(nodeName.data());
        }

        web::json::value GetSearchBody(const SearchRequest& searchRequest)
        {
            // TODO: Use search request to construct search body.
            UNREFERENCED_PARAMETER(searchRequest);

            web::json::value json_body;
            json_body[GetJsonKeyNameString(FetchAllManifests)] = web::json::value::string(L"true");

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

        std::optional<std::string> GetStringFromJsonValue(const web::json::value& value)
        {
            if (value.is_null() || !value.is_string())
            {
                return {};
            }

            return utility::conversions::to_utf8string(value.as_string());
        }

        std::optional<std::string> ValidateAndGetStringFromJsonStringValue(const web::json::value& value)
        {
            std::optional<std::string> result = GetStringFromJsonValue(value);

            if (!result.has_value())
            {
                return {};
            }

            std::string stringValue = Utility::Trim(result.value());

            if (Utility::IsEmptyOrWhitespace(stringValue))
            {
                return {};
            }

            return stringValue;
        }
    }

    Interface::Interface(const std::string& restApi)
    {
        m_restApiUri = GetRestAPIBaseUri(restApi);
        m_searchEndpoint = GetSearchEndpoint(m_restApiUri);
    }

    IRestClient::SearchResult Interface::Search(const SearchRequest& request) const
    {
        SearchResult result;

        // TODO: Handle continuation token.
        HttpClientHelper clientHelper{ m_searchEndpoint };
        web::json::value jsonObject = clientHelper.HandlePost(GetSearchBody(request));

        // Parse json and add results to SearchResult.
        if (jsonObject.is_null())
        {
            return result;
        }

        auto& dataArray = jsonObject.at(GetJsonKeyNameString(Data)).as_array();

        for (auto& manifestItem : dataArray)
        {
            std::optional<std::string> packageId = ValidateAndGetStringFromJsonStringValue(manifestItem.at(GetJsonKeyNameString(PackageIdentifier)));
            std::optional<std::string> packageName = ValidateAndGetStringFromJsonStringValue(manifestItem.at(GetJsonKeyNameString(PackageName)));
            std::optional<std::string> publisher = ValidateAndGetStringFromJsonStringValue(manifestItem.at(GetJsonKeyNameString(Publisher)));
            std::optional<std::string> packageFamilyName = GetStringFromJsonValue(manifestItem.at(GetJsonKeyNameString(PackageFamilyName)));
            std::optional<std::string> productCode = GetStringFromJsonValue(manifestItem.at(GetJsonKeyNameString(ProductCode)));
            web::json::value versionValue = manifestItem.at(GetJsonKeyNameString(Versions));

            if (!packageId.has_value() || !packageName.has_value() || !publisher.has_value() || versionValue.is_null()|| !versionValue.is_array() || versionValue.as_array().size() == 0)
            {
                AICLI_LOG(Repo, Verbose, << "Received incomplete package. Skipping package: " << packageId.value_or(""));
                continue;
            }

            std::vector<AppInstaller::Utility::VersionAndChannel> versionList;
            for (auto& versionItem : versionValue.as_array())
            {
                std::optional<std::string> version = ValidateAndGetStringFromJsonStringValue(versionItem.at(GetJsonKeyNameString(Version)));
                std::optional<std::string> channel = GetStringFromJsonValue(versionItem.at(GetJsonKeyNameString(Channel)));

                if (!version.has_value())
                {
                    AICLI_LOG(Repo, Verbose, << "Received incomplete package version. Skipping version from package: " << packageId.value());
                    continue;
                }

                versionList.emplace_back(AppInstaller::Utility::VersionAndChannel{ std::move(version.value()), std::move(channel.value_or("")) });
            }

            if (versionList.size() == 0)
            {
                AICLI_LOG(Repo, Verbose, << "Received no valid versions. Skipping package: " << packageId.value());
                continue;
            }

            PackageInfo packageInfo = PackageInfo{ std::move(packageId.value()), std::move(packageName.value()), std::move(publisher.value()) };
            Package package = Package{ std::move(packageInfo), std::move(versionList) };
            result.Matches.emplace_back(std::move(package));
        }

        return result;
    }

    std::optional<Manifest::Manifest> Interface::GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const
    {
        HttpClientHelper clientHelper{ GetManifestByVersionEndpoint(m_restApiUri, packageId, version, channel) };
        web::json::value jsonObject = clientHelper.HandleGet();

        if (jsonObject.is_null())
        {
            return {};
        }

        // Parse json and return Manifest
        (void)jsonObject.at(GetJsonKeyNameString(Data));
        return {};
    }
}
