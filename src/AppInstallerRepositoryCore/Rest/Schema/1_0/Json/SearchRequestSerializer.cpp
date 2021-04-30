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

        std::optional<std::string_view> ConvertPackageMatchFieldToString(AppInstaller::Repository::PackageMatchField field)
        {
            // Match fields supported by Rest API schema.
            switch (field)
            {
            case PackageMatchField::Command:
                return "Command"sv;
            case PackageMatchField::Id:
                return "PackageIdentifier"sv;
            case PackageMatchField::Moniker:
                return "Moniker"sv;
            case PackageMatchField::Name:
                return "PackageName"sv;
            case PackageMatchField::Tag:
                return "Tag"sv;
            case PackageMatchField::PackageFamilyName:
                return "PackageFamilyName"sv;
            case PackageMatchField::ProductCode:
                return "ProductCode"sv;
            case PackageMatchField::NormalizedNameAndPublisher:
                return "NormalizedPackageNameAndPublisher"sv;
            }

            return {};
        }

        std::optional<std::string_view> ConvertMatchTypeToString(AppInstaller::Repository::MatchType type)
        {
            // Match types supported by Rest API schema.
            switch (type)
            {
            case MatchType::Exact:
                return "Exact"sv;
            case MatchType::CaseInsensitive:
                return "CaseInsensitive"sv;
            case MatchType::StartsWith:
                return "StartsWith"sv;
            case MatchType::Substring:
                return "Substring"sv;
            case MatchType::Wildcard:
                return "Wildcard"sv;
            case MatchType::Fuzzy:
                return "Fuzzy"sv;
            case MatchType::FuzzySubstring:
                return "FuzzySubstring"sv;
            }

            return {};
        }
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
                std::optional<web::json::value> requestMatchJson = GetRequestMatchJsonObject(requestMatch);
                if (requestMatchJson)
                {
                    json_body[JsonHelper::GetUtilityString(Query)] = std::move(requestMatchJson.value());
                }
            }

            if (!searchRequest.Filters.empty())
            {
                web::json::value filters = web::json::value::array();

                int i = 0;
                for (auto& filter : searchRequest.Filters)
                {
                    std::optional<web::json::value> jsonObject = GetPackageMatchFilterJsonObject(filter);

                    if (jsonObject)
                    {
                        filters[i++] = std::move(jsonObject.value());
                    }
                }

                json_body[JsonHelper::GetUtilityString(Filters)] = filters;
            }

            if (!searchRequest.Inclusions.empty())
            {
                web::json::value inclusions = web::json::value::array();

                int i = 0;
                for (auto& inclusion : searchRequest.Inclusions)
                {
                    std::optional<web::json::value> jsonObject = GetPackageMatchFilterJsonObject(inclusion);

                    if (jsonObject)
                    {
                        inclusions[i++] = std::move(jsonObject.value());
                    }
                }

                json_body[JsonHelper::GetUtilityString(Inclusions)] = inclusions;
            }

            return json_body;
        }
        catch (const std::exception& e)
        {
            AICLI_LOG(Repo, Error, << "Error occurred while serializing search request. Reason: " << e.what());
        }
        catch (...)
        {
            AICLI_LOG(Repo, Error, << "Error occurred while serializing search request");
        }

        return {};
    }

    std::optional<web::json::value> SearchRequestSerializer::GetPackageMatchFilterJsonObject(const PackageMatchFilter& packageMatchFilter) const
    {
        web::json::value filter = web::json::value::object();
        std::optional<std::string_view> matchField = ConvertPackageMatchFieldToString(packageMatchFilter.Field);
        
        if (!matchField)
        {
            AICLI_LOG(Repo, Warning, << "Skipping unsupported package match field: " << packageMatchFilter.Field);
            return {};
        }

        filter[JsonHelper::GetUtilityString(PackageMatchField)] = web::json::value::string(JsonHelper::GetUtilityString(matchField.value()));
        AppInstaller::Repository::RequestMatch requestMatch{ packageMatchFilter.Type, packageMatchFilter.Value };
        std::optional<web::json::value> requestMatchJson = GetRequestMatchJsonObject(requestMatch);

        if (!requestMatchJson)
        {
            AICLI_LOG(Repo, Warning, << "Skipping unsupported request match object.");
            return {};
        }

        filter[JsonHelper::GetUtilityString(RequestMatch)] = std::move(requestMatchJson.value());
        return filter;
    }

    std::optional<web::json::value> SearchRequestSerializer::GetRequestMatchJsonObject(const AppInstaller::Repository::RequestMatch& requestMatch) const
    {
        web::json::value match = web::json::value::object();
        match[JsonHelper::GetUtilityString(KeyWord)] = web::json::value::string(JsonHelper::GetUtilityString(requestMatch.Value));

        std::optional<std::string_view> matchType = ConvertMatchTypeToString(requestMatch.Type);
        if (!matchType)
        {
            AICLI_LOG(Repo, Warning, << "Skipping unsupported match type: " << requestMatch.Type);
            return {};
        }

        match[JsonHelper::GetUtilityString(MatchType)] = web::json::value::string(JsonHelper::GetUtilityString(matchType.value()));
        return match;
    }
}
