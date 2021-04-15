// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/IRestClient.h"
#include "SearchRequestSerializer.h"
#include "Rest/Schema/JsonHelper.h"
#include "CommonJsonConstants.h"

namespace AppInstaller::Repository::Rest::Schema::V1_0::Json
{
    namespace
    {
        // Search request constants
        constexpr std::string_view Query = "Query"sv;
        constexpr std::string_view Filters = "Filters"sv;
        constexpr std::string_view Inclusions = "Inclusions"sv;
        constexpr std::string_view MaximumResults = "MaximumResults"sv;
        constexpr std::string_view RequestMatch = "RequestMatch"sv;
        constexpr std::string_view KeyWord = "KeyWord"sv;
        constexpr std::string_view MatchType = "MatchType"sv;
        constexpr std::string_view PackageMatchField = "PackageMatchField"sv;
        constexpr std::string_view FetchAllManifests = "FetchAllManifests"sv;
    }

    web::json::value SearchRequestSerializer::Serialize(const SearchRequest& searchRequest) const
    {
        std::optional<web::json::value> result = SerializeSearchRequest(searchRequest);

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INTERNAL_ERROR, !result);

        return result.value();
    }

    std::optional<web::json::value> SearchRequestSerializer::SerializeSearchRequest(const SearchRequest& searchRequest) const
    {
        try
        {
            web::json::value json_body;
            if (searchRequest.MaximumResults > 0)
            {
                json_body[JsonHelper::GetUtilityString(MaximumResults)] = searchRequest.MaximumResults;
            }

            if (searchRequest.IsForEverything())
            {
                json_body[JsonHelper::GetUtilityString(FetchAllManifests)] = web::json::value::boolean(true);
                return json_body;
            }

            if (searchRequest.Query)
            {
                auto& requestMatch = searchRequest.Query.value();
                web::json::value requestMatchObject = web::json::value::object();
                json_body[JsonHelper::GetUtilityString(Query)] = GetRequestMatchJsonObject(requestMatch);;
            }

            if (!searchRequest.Filters.empty())
            {
                web::json::value filters = web::json::value::array();

                int i = 0;
                for (auto& filter : searchRequest.Filters)
                {
                    filters[i++] = GetPackageMatchFilterJsonObject(filter);
                }

                json_body[JsonHelper::GetUtilityString(Filters)] = filters;
            }

            if (!searchRequest.Inclusions.empty())
            {
                web::json::value inclusions = web::json::value::array();

                int i = 0;
                for (auto& inclusion : searchRequest.Inclusions)
                {
                    inclusions[i++] = GetPackageMatchFilterJsonObject(inclusion);
                }

                json_body[JsonHelper::GetUtilityString(Inclusions)] = inclusions;
            }

            return json_body;
        }
        catch (...)
        {
            AICLI_LOG(Repo, Verbose, << "Error occurred while serializing search request");
        }

        return {};
    }

    web::json::value SearchRequestSerializer::GetPackageMatchFilterJsonObject(const PackageMatchFilter& packageMatchFilter) const
    {
        web::json::value filter = web::json::value::object();
        filter[JsonHelper::GetUtilityString(PackageMatchField)] = web::json::value::string(JsonHelper::GetUtilityString(
            PackageMatchFieldToString(packageMatchFilter.Field)));
        AppInstaller::Repository::RequestMatch requestMatch{ packageMatchFilter.Type, packageMatchFilter.Value };
        filter[JsonHelper::GetUtilityString(RequestMatch)] = GetRequestMatchJsonObject(requestMatch);

        return filter;
    }

    web::json::value SearchRequestSerializer::GetRequestMatchJsonObject(const AppInstaller::Repository::RequestMatch& requestMatch) const
    {
        web::json::value match = web::json::value::object();
        match[JsonHelper::GetUtilityString(KeyWord)] = web::json::value::string(JsonHelper::GetUtilityString(requestMatch.Value));
        match[JsonHelper::GetUtilityString(MatchType)] = web::json::value::string(JsonHelper::GetUtilityString(MatchTypeToString(requestMatch.Type)));

        return match;
    }
}
