// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/CommonRestConstants.h"
#include "Rest/Schema/IRestClient.h"
#include "SearchResponseDeserializer.h"
#include <winget/JsonUtil.h>
#include <winget/Rest.h>

namespace AppInstaller::Repository::Rest::Schema::V1_0::Json
{
    namespace
    {
        // Search response constants
        constexpr std::string_view PackageIdentifier = "PackageIdentifier"sv;
        constexpr std::string_view PackageName = "PackageName"sv;
        constexpr std::string_view Publisher = "Publisher"sv;
        constexpr std::string_view PackageFamilyNames = "PackageFamilyNames"sv;
        constexpr std::string_view ProductCodes = "ProductCodes"sv;
        constexpr std::string_view Versions = "Versions"sv;
        constexpr std::string_view PackageVersion = "PackageVersion"sv;
        constexpr std::string_view Channel = "Channel"sv;
    }

    IRestClient::SearchResult SearchResponseDeserializer::Deserialize(const web::json::value& searchResponseObject) const
    {
        std::optional<IRestClient::SearchResult> response = DeserializeSearchResult(searchResponseObject);

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA, !response);

        return response.value();
    }

    std::optional<IRestClient::SearchResult> SearchResponseDeserializer::DeserializeSearchResult(const web::json::value& searchResponseObject) const
    {
        // Make search result from json output.
        if (searchResponseObject.is_null())
        {
            AICLI_LOG(Repo, Error, << "Missing json object.");
            return {};
        }

        IRestClient::SearchResult result;
        try
        {
            std::optional<std::reference_wrapper<const web::json::array>> dataArray = JSON::GetRawJsonArrayFromJsonNode(searchResponseObject, JSON::GetUtilityString(Data));
            if (!dataArray || dataArray.value().get().size() == 0)
            {
                AICLI_LOG(Repo, Verbose, << "No search results returned.");
                return result;
            }

            for (auto& manifestItem : dataArray.value().get())
            {
                std::optional<std::string> packageId = JSON::GetRawStringValueFromJsonNode(manifestItem, JSON::GetUtilityString(PackageIdentifier));
                std::optional<std::string> packageName = JSON::GetRawStringValueFromJsonNode(manifestItem, JSON::GetUtilityString(PackageName));
                std::optional<std::string> publisher = JSON::GetRawStringValueFromJsonNode(manifestItem, JSON::GetUtilityString(Publisher));

                if (!JSON::IsValidNonEmptyStringValue(packageId) || !JSON::IsValidNonEmptyStringValue(packageName) || !JSON::IsValidNonEmptyStringValue(publisher))
                {
                    AICLI_LOG(Repo, Error, << "Missing required package fields in manifest search results.");
                    return {};
                }

                std::optional<std::reference_wrapper<const web::json::array>> versionValue = JSON::GetRawJsonArrayFromJsonNode(manifestItem, JSON::GetUtilityString(Versions));
                std::vector<IRestClient::VersionInfo> versionList;

                if (versionValue)
                {
                    for (auto& versionItem : versionValue.value().get())
                    {
                        auto versionInfo = DeserializeVersionInfo(versionItem);
                        if (!versionInfo.has_value())
                        {
                            AICLI_LOG(Repo, Error, << "Received incomplete package version in package: " << packageId.value());
                            return {};
                        }

                        versionList.emplace_back(std::move(*versionInfo));
                    }
                }

                if (versionList.size() == 0)
                {
                    AICLI_LOG(Repo, Error, << "Received no versions in package: " << packageId.value());
                    return {};
                }

                IRestClient::PackageInfo packageInfo{
                        std::move(packageId.value()), std::move(packageName.value()), std::move(publisher.value()) };
                IRestClient::Package package{ std::move(packageInfo), std::move(versionList) };
                result.Matches.emplace_back(std::move(package));
            }

            return result;
        }
        catch (const std::exception& e)
        {
            AICLI_LOG(Repo, Error, << "Error encountered while deserializing search result. Reason: " << e.what());
        }
        catch (...)
        {
            AICLI_LOG(Repo, Error, << "Error encountered while deserializing search result...");
        }

        return {};
    }

    std::optional<IRestClient::VersionInfo> SearchResponseDeserializer::DeserializeVersionInfo(const web::json::value& versionInfoJsonObject) const
    {
        std::optional<std::string> version = JSON::GetRawStringValueFromJsonNode(versionInfoJsonObject, JSON::GetUtilityString(PackageVersion));
        if (!JSON::IsValidNonEmptyStringValue(version))
        {
            AICLI_LOG(Repo, Error, << "Received incomplete package version");
            return {};
        }

        std::string channel = JSON::GetRawStringValueFromJsonNode(versionInfoJsonObject, JSON::GetUtilityString(Channel)).value_or("");
        std::vector<std::string> packageFamilyNames = AppInstaller::Rest::GetUniqueItems(JSON::GetRawStringArrayFromJsonNode(versionInfoJsonObject, JSON::GetUtilityString(PackageFamilyNames)));
        std::vector<std::string> productCodes = AppInstaller::Rest::GetUniqueItems(JSON::GetRawStringArrayFromJsonNode(versionInfoJsonObject, JSON::GetUtilityString(ProductCodes)));

        return IRestClient::VersionInfo{
            AppInstaller::Utility::VersionAndChannel{std::move(version.value()), std::move(channel)},
            {},
            std::move(packageFamilyNames),
            std::move(productCodes) };
    }
}
