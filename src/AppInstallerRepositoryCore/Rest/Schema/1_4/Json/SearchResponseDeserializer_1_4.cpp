// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SearchResponseDeserializer.h"
#include "Rest/Schema/RestHelper.h"
#include <winget/JsonUtil.h>

namespace AppInstaller::Repository::Rest::Schema::V1_4::Json
{
    namespace
    {
        // Search response constants
        constexpr std::string_view UpgradeCodes = "UpgradeCodes"sv;
        constexpr std::string_view AppsAndFeaturesEntryVersions = "AppsAndFeaturesEntryVersions"sv;
    }

    std::optional<IRestClient::VersionInfo> SearchResponseDeserializer::DeserializeVersionInfo(const web::json::value& versionInfoJsonObject) const
    {
        auto result = V1_0::Json::SearchResponseDeserializer::DeserializeVersionInfo(versionInfoJsonObject);
        if (result.has_value())
        {
            result->UpgradeCodes = RestHelper::GetUniqueItems(JSON::GetRawStringArrayFromJsonNode(versionInfoJsonObject, JSON::GetUtilityString(UpgradeCodes)));
            auto arpVersions = RestHelper::GetUniqueItems(JSON::GetRawStringArrayFromJsonNode(versionInfoJsonObject, JSON::GetUtilityString(AppsAndFeaturesEntryVersions)));
            for (auto const& version : arpVersions)
            {
                result->ArpVersions.emplace_back(Utility::Version{ version });
            }
            // Sort the arp versions for later querying
            std::sort(result->ArpVersions.begin(), result->ArpVersions.end());
        }

        return result;
    }
}
