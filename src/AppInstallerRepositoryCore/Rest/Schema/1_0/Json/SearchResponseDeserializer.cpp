// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/IRestClient.h"
#include "SearchResponseDeserializer.h"
#include "Rest/Schema/JsonHelper.h"
#include "CommonJsonConstants.h"

namespace AppInstaller::Repository::Rest::Schema::V1_0::Json
{
    namespace
    {
        // Search response constants
        constexpr std::string_view PackageIdentifier = "PackageIdentifier"sv;
        constexpr std::string_view PackageName = "PackageName"sv;
        constexpr std::string_view Publisher = "Publisher"sv;
        constexpr std::string_view PackageFamilyName = "PackageFamilyName"sv;
        constexpr std::string_view ProductCode = "ProductCode"sv;
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
            std::optional<std::reference_wrapper<const web::json::array>> dataArray = JsonHelper::GetRawJsonArrayFromJsonNode(searchResponseObject, JsonHelper::GetUtilityString(Data));
            if (!dataArray || dataArray.value().get().size() == 0)
            {
                AICLI_LOG(Repo, Verbose, << "No search results returned.");
                return result;
            }

            for (auto& manifestItem : dataArray.value().get())
            {
                std::optional<std::string> packageId = JsonHelper::GetRawStringValueFromJsonNode(manifestItem, JsonHelper::GetUtilityString(PackageIdentifier));
                std::optional<std::string> packageName = JsonHelper::GetRawStringValueFromJsonNode(manifestItem, JsonHelper::GetUtilityString(PackageName));
                std::optional<std::string> publisher = JsonHelper::GetRawStringValueFromJsonNode(manifestItem, JsonHelper::GetUtilityString(Publisher));

                if (!JsonHelper::IsValidNonEmptyStringValue(packageId) || !JsonHelper::IsValidNonEmptyStringValue(packageName) || !JsonHelper::IsValidNonEmptyStringValue(publisher))
                {
                    AICLI_LOG(Repo, Error, << "Missing required package fields in manifest search results.");
                    return {};
                }

                std::string packageFamilyName = JsonHelper::GetRawStringValueFromJsonNode(manifestItem, JsonHelper::GetUtilityString(PackageFamilyName)).value_or("");
                std::string productCode = JsonHelper::GetRawStringValueFromJsonNode(manifestItem, JsonHelper::GetUtilityString(ProductCode)).value_or("");
                std::optional<std::reference_wrapper<const web::json::array>> versionValue = JsonHelper::GetRawJsonArrayFromJsonNode(manifestItem, JsonHelper::GetUtilityString(Versions));
                std::vector<IRestClient::VersionInfo> versionList;

                if (versionValue)
                {
                    for (auto& versionItem : versionValue.value().get())
                    {
                        std::optional<std::string> version = JsonHelper::GetRawStringValueFromJsonNode(versionItem, JsonHelper::GetUtilityString(PackageVersion));
                        if (!JsonHelper::IsValidNonEmptyStringValue(version))
                        {
                            AICLI_LOG(Repo, Error, << "Received incomplete package version in package: " << packageId.value());
                            return {};
                        }

                        std::string channel = JsonHelper::GetRawStringValueFromJsonNode(versionItem, JsonHelper::GetUtilityString(Channel)).value_or("");
                        versionList.emplace_back(IRestClient::VersionInfo{
                                AppInstaller::Utility::VersionAndChannel{std::move(version.value()), std::move(channel)}, {} });
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
        }
        catch (...)
        {
            // TODO: Catch known types and log error information from them
            AICLI_LOG(Repo, Error, << "Error encountered while deserializing search result...");
            return {};
        }

        return result;
    }
}
