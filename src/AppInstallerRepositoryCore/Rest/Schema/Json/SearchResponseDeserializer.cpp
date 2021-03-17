// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/IRestClient.h"
#include "SearchResponseDeserializer.h"
#include <cpprest/json.h>
#include "JsonHelper.h"
#include "CommonRestConstants.h"

namespace AppInstaller::Repository::Rest::Schema::Json
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
        constexpr std::string_view PackageVersion = "Version"sv;
        constexpr std::string_view Channel = "Channel"sv;
    }

    std::optional<IRestClient::SearchResult> SearchResponseDeserializer::Deserialize(const web::json::value& serachResponseObject) const
    {
        // Make search result from json output.
        if (serachResponseObject.is_null())
        {
            return {};
        }

        IRestClient::SearchResult result;
        try
        {
            std::optional<std::reference_wrapper<const web::json::array>> dataArray = JsonHelper::GetRawJsonArrayFromJsonNode(serachResponseObject, JsonHelper::GetJsonKeyNameString(Data));
            if (!dataArray.has_value() || dataArray.value().get().size() == 0)
            {
                AICLI_LOG(Repo, Verbose, << "No search results returned.");
                return result;
            }

            for (auto& manifestItem : dataArray.value().get())
            {
                std::optional<std::string> packageId = JsonHelper::GetRawStringValueFromJsonNode(manifestItem, JsonHelper::GetJsonKeyNameString(PackageIdentifier));
                std::optional<std::string> packageName = JsonHelper::GetRawStringValueFromJsonNode(manifestItem, JsonHelper::GetJsonKeyNameString(PackageName));
                std::optional<std::string> publisher = JsonHelper::GetRawStringValueFromJsonNode(manifestItem, JsonHelper::GetJsonKeyNameString(Publisher));

                if (!JsonHelper::IsValidNonEmptyStringValue(packageId) || !JsonHelper::IsValidNonEmptyStringValue(packageName) || !JsonHelper::IsValidNonEmptyStringValue(packageName))
                {
                    AICLI_LOG(Repo, Verbose, << "Skipping manifest item because of missing required package fields.");
                    continue;
                }

                std::string packageFamilyName = JsonHelper::GetRawStringValueFromJsonNode(manifestItem, JsonHelper::GetJsonKeyNameString(PackageFamilyName)).value_or("");
                std::string productCode = JsonHelper::GetRawStringValueFromJsonNode(manifestItem, JsonHelper::GetJsonKeyNameString(ProductCode)).value_or("");
                std::optional<std::reference_wrapper<const web::json::array>> versionValue = JsonHelper::GetRawJsonArrayFromJsonNode(manifestItem, JsonHelper::GetJsonKeyNameString(Versions));
                std::vector<IRestClient::VersionInfo> versionList;

                if (versionValue.has_value())
                {
                    for (auto& versionItem : versionValue.value().get())
                    {
                        std::optional<std::string> version = JsonHelper::GetRawStringValueFromJsonNode(versionItem, JsonHelper::GetJsonKeyNameString(PackageVersion));
                        if (!JsonHelper::IsValidNonEmptyStringValue(version))
                        {
                            AICLI_LOG(Repo, Verbose, << "Received incomplete package version. Skipping version from package: " << packageId.value());
                            continue;
                        }

                        std::string channel = JsonHelper::GetRawStringValueFromJsonNode(versionItem, JsonHelper::GetJsonKeyNameString(Channel)).value_or("");
                        versionList.emplace_back(IRestClient::VersionInfo{ 
                                AppInstaller::Utility::VersionAndChannel{std::move(version.value()), std::move(channel)}, {} });
                    }
                }

                if (versionList.size() > 0)
                {
                    IRestClient::PackageInfo packageInfo {
                        std::move(packageId.value()), std::move(packageName.value()), std::move(publisher.value()) };
                    IRestClient::Package package { std::move(packageInfo), std::move(versionList) };
                    result.Matches.emplace_back(std::move(package));
                }
                else
                {
                    AICLI_LOG(Repo, Verbose, << "Received no versions. Skipping package: " << packageId.value());
                }
            }
        }
        catch (...)
        {
            AICLI_LOG(Repo, Verbose, << "Error encountered while deserializing search result...");
            return {};
        }

        return result;
    }
}
