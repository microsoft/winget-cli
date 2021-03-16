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
            else if(!channel.empty())
            {
                versionEndpoint.append("&channel=").append(channel);
            }

            return utility::conversions::to_string_t(versionEndpoint);
        }

        std::optional<std::string> GetStringFromJsonStringValue(const web::json::value& value)
        {
            if (value.is_null() || !value.is_string())
            {
                return {};
            }

            return utility::conversions::to_utf8string(value.as_string());
        }

        std::string ValidateAndGetStringFromJsonStringValue(const web::json::value& value)
        {
            std::optional<std::string> result = GetStringFromJsonStringValue(value);

            if (!result.has_value() && Utility::IsEmptyOrWhitespace(result.value()))
            {
                THROW_HR_MSG(E_UNEXPECTED, "Missing required value");
            }

            return result.value();
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

        try
        {
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
                try
                {
                    std::string packageId = ValidateAndGetStringFromJsonStringValue(manifestItem.at(GetJsonKeyNameString(PackageIdentifier)));
                    std::string packageName = ValidateAndGetStringFromJsonStringValue(manifestItem.at(GetJsonKeyNameString(PackageName)));
                    std::string publisher = ValidateAndGetStringFromJsonStringValue(manifestItem.at(GetJsonKeyNameString(Publisher)));
                    std::optional<std::string> packageFamilyName = GetStringFromJsonStringValue(manifestItem.at(GetJsonKeyNameString(PackageFamilyName)));
                    std::optional<std::string> productCode = GetStringFromJsonStringValue(manifestItem.at(GetJsonKeyNameString(ProductCode)));
                    web::json::value versionValue = manifestItem.at(GetJsonKeyNameString(Versions));

                    if (versionValue.is_null() || versionValue.as_array().size() == 0)
                    {
                        AICLI_LOG(Repo, Verbose, << "Received incomplete package. Skipping package: " << packageId);
                        continue;
                    }

                    std::vector<AppInstaller::Utility::VersionAndChannel> versionList;
                    for (auto& versionItem : versionValue.as_array())
                    {
                        try
                        {
                            std::string version = ValidateAndGetStringFromJsonStringValue(versionItem.at(GetJsonKeyNameString(Version)));
                            std::optional<std::string> channel = GetStringFromJsonStringValue(versionItem.at(GetJsonKeyNameString(Channel)));

                            versionList.emplace_back(AppInstaller::Utility::VersionAndChannel{ std::move(version), std::move(channel.value_or("")) });
                        }
                        catch (...)
                        {
                            AICLI_LOG(Repo, Verbose, << "Received incomplete package version. Skipping version from package: " << packageId);
                        }
                    }

                    if (versionList.size() == 0)
                    {
                        AICLI_LOG(Repo, Verbose, << "Received no valid versions. Skipping package: " << packageId);
                        continue;
                    }

                    PackageInfo packageInfo = PackageInfo{ std::move(packageId), std::move(packageName), std::move(publisher) };
                    Package package = Package{ std::move(packageInfo), std::move(versionList) };
                    result.Matches.emplace_back(std::move(package));
                }
                catch (...)
                {
                    AICLI_LOG(Repo, Verbose, << "Received invalid manifest item. Skipping.");
                }
            }
        }
        catch (...)
        {
            AICLI_LOG(Repo, Verbose, << "Error occurred while attempting to search from Rest source.");
        }

        return result;
    }

    std::optional<Manifest::Manifest> Interface::GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const
    {
        try
        {
            HttpClientHelper clientHelper{ GetManifestByVersionEndpoint(m_restApiUri, packageId, version, channel) };
            web::json::value jsonObject = clientHelper.HandleGet();

            if (jsonObject.is_null())
            {
                return {};
            }

            // Parse json and return Manifest
            (void)jsonObject.at(GetJsonKeyNameString(Data));
        }
        catch (...)
        {
            AICLI_LOG(Repo, Verbose, << "Error occurred while attempting to get manifest by version");
        }

        return {};
    }
}
