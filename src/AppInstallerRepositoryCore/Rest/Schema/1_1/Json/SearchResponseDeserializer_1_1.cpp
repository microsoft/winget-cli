// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/IRestClient.h"
#include "SearchResponseDeserializer.h"
#include "Rest/Schema/JsonHelper.h"
#include "Rest/Schema/RestHelper.h"
#include "Rest/Schema/CommonRestConstants.h"

namespace AppInstaller::Repository::Rest::Schema::V1_1::Json
{
    namespace
    {
        // Search response constants
        constexpr std::string_view UnsupportedPackageMatchFields = "UnsupportedPackageMatchFields"sv;
        constexpr std::string_view RequiredPackageMatchFields = "RequiredPackageMatchFields"sv;
    }

    std::optional<IRestClient::SearchResult> SearchResponseDeserializer::DeserializeSearchResult(const web::json::value& searchResponseObject) const
    {
        std::optional<IRestClient::SearchResult> result = V1_0::Json::SearchResponseDeserializer::DeserializeSearchResult(searchResponseObject);

        if (result)
        {
            result.value().RequiredPackageMatchFields = JsonHelper::GetRawStringArrayFromJsonNode(searchResponseObject, JsonHelper::GetUtilityString(RequiredPackageMatchFields));
            result.value().UnsupportedPackageMatchFields = JsonHelper::GetRawStringArrayFromJsonNode(searchResponseObject, JsonHelper::GetUtilityString(UnsupportedPackageMatchFields));
        }

        return result;
    }
}
