// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_1/Interface.h"
#include "Rest/Schema/CommonRestConstants.h"
#include "Rest/Schema/IRestClient.h"
#include <winget/HttpClientHelper.h>
#include <winget/JsonUtil.h>

using namespace std::string_view_literals;

namespace AppInstaller::Repository::Rest::Schema::V1_1
{
    namespace
    {
        // Query params
        constexpr std::string_view MarketQueryParam = "Market"sv;

        // Response constants
        constexpr std::string_view UnsupportedPackageMatchFields = "UnsupportedPackageMatchFields"sv;
        constexpr std::string_view RequiredPackageMatchFields = "RequiredPackageMatchFields"sv;
        constexpr std::string_view UnsupportedQueryParameters = "UnsupportedQueryParameters"sv;
        constexpr std::string_view RequiredQueryParameters = "RequiredQueryParameters"sv;
    }

    Interface::Interface(
        const std::string& restApi,
        const Http::HttpClientHelper& httpClientHelper,
        IRestClient::Information information,
        const Http::HttpClientHelper::HttpRequestHeaders& additionalHeaders) : V1_0::Interface(restApi, httpClientHelper), m_information(std::move(information))
    {
        m_requiredRestApiHeaders[JSON::GetUtilityString(ContractVersion)] = JSON::GetUtilityString(Version_1_1_0.ToString());

        if (!additionalHeaders.empty())
        {
            m_requiredRestApiHeaders.insert(additionalHeaders.begin(), additionalHeaders.end());
        }
    }

    Utility::Version Interface::GetVersion() const
    {
        return Version_1_1_0;
    }

    IRestClient::Information Interface::GetSourceInformation() const
    {
        return m_information;
    }

    std::map<std::string_view, std::string> Interface::GetValidatedQueryParams(const std::map<std::string_view, std::string>& params) const
    {
        std::map<std::string_view, std::string> result = params;

        for (auto const& param : m_information.RequiredQueryParameters)
        {
            if (params.end() == std::find_if(params.begin(), params.end(), [&](const auto& pair) { return Utility::CaseInsensitiveEquals(pair.first, param); }))
            {
                if (Utility::CaseInsensitiveEquals(param, MarketQueryParam))
                {
                    result.emplace(MarketQueryParam, Runtime::GetOSRegion());
                    continue;
                }

                AICLI_LOG(Repo, Error, << "Search request is not supported by the rest source. Required query Parameter: " << param);
                throw UnsupportedRequestException({}, {}, {}, m_information.RequiredQueryParameters);
            }
        }

        for (auto const& param : m_information.UnsupportedQueryParameters)
        {
            if (params.end() != std::find_if(params.begin(), params.end(), [&](const auto& pair) { return Utility::CaseInsensitiveEquals(pair.first, param); }))
            {
                AICLI_LOG(Repo, Error, << "Search request is not supported by the rest source. Unsupported query Parameter: " << param);
                throw UnsupportedRequestException({}, {}, m_information.UnsupportedQueryParameters, {});
            }
        }

        return result;
    }

    web::json::value Interface::GetValidatedSearchBody(const SearchRequest& searchRequest) const
    {
        SearchRequest resultSearchRequest = searchRequest;

        for (auto const& field : m_information.RequiredPackageMatchFields)
        {
            PackageMatchField matchField = ConvertStringToPackageMatchField(field);

            if (searchRequest.Filters.end() == std::find_if(searchRequest.Filters.begin(), searchRequest.Filters.end(), [&](const PackageMatchFilter& filter) { return filter.Field == matchField; }))
            {
                if (matchField == PackageMatchField::Market)
                {
                    resultSearchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Market, MatchType::CaseInsensitive, Runtime::GetOSRegion()));
                    continue;
                }

                AICLI_LOG(Repo, Error, << "Search request is not supported by the rest source. Required package match field: " << field);
                throw UnsupportedRequestException({}, m_information.RequiredPackageMatchFields, {}, {});
            }
        }

        for (auto const& field : m_information.UnsupportedPackageMatchFields)
        {
            PackageMatchField matchField = ConvertStringToPackageMatchField(field);

            if (matchField == PackageMatchField::Unknown)
            {
                continue;
            }

            if (searchRequest.Inclusions.end() != std::find_if(searchRequest.Inclusions.begin(), searchRequest.Inclusions.end(), [&](const PackageMatchFilter& inclusion) { return inclusion.Field == matchField; }))
            {
                AICLI_LOG(Repo, Info, << "Search request Inclusions contains package match field not supported by the rest source. Ignoring the field. Unsupported package match field: " << field);

                resultSearchRequest.Inclusions.erase(
                    std::remove_if(
                        resultSearchRequest.Inclusions.begin(), resultSearchRequest.Inclusions.end(),
                        [&](const PackageMatchFilter& inclusion) { return inclusion.Field == matchField; }),
                    resultSearchRequest.Inclusions.end());
            }

            if (searchRequest.Filters.end() != std::find_if(searchRequest.Filters.begin(), searchRequest.Filters.end(), [&](const PackageMatchFilter& filter) { return filter.Field == matchField; }))
            {
                AICLI_LOG(Repo, Error, << "Search request is not supported by the rest source. Unsupported package match field: " << field);
                throw UnsupportedRequestException(m_information.UnsupportedPackageMatchFields, {}, {}, {});
            }
        }

        return V1_0::Interface::GetValidatedSearchBody(resultSearchRequest);
    }

    IRestClient::SearchResult Interface::GetSearchResult(const web::json::value& searchResponseObject) const
    {
        IRestClient::SearchResult result = V1_0::Interface::GetSearchResult(searchResponseObject);

        if (result.Matches.size() == 0)
        {
            auto requiredPackageMatchFields = JSON::GetRawStringArrayFromJsonNode(searchResponseObject, JSON::GetUtilityString(RequiredPackageMatchFields));
            auto unsupportedPackageMatchFields = JSON::GetRawStringArrayFromJsonNode(searchResponseObject, JSON::GetUtilityString(UnsupportedPackageMatchFields));

            if (requiredPackageMatchFields.size() != 0 || unsupportedPackageMatchFields.size() != 0)
            {
                AICLI_LOG(Repo, Error, << "Search request is not supported by the rest source");
                throw UnsupportedRequestException(std::move(unsupportedPackageMatchFields), std::move(requiredPackageMatchFields), {}, {});
            }
        }

        return result;
    }

    std::vector<Manifest::Manifest> Interface::GetParsedManifests(const web::json::value& manifestsResponseObject) const
    {
        auto result = V1_0::Interface::GetParsedManifests(manifestsResponseObject);

        if (result.size() == 0)
        {
            auto requiredQueryParameters = JSON::GetRawStringArrayFromJsonNode(manifestsResponseObject, JSON::GetUtilityString(RequiredQueryParameters));
            auto unsupportedQueryParameters = JSON::GetRawStringArrayFromJsonNode(manifestsResponseObject, JSON::GetUtilityString(UnsupportedQueryParameters));

            if (requiredQueryParameters.size() != 0 || unsupportedQueryParameters.size() != 0)
            {
                AICLI_LOG(Repo, Error, << "Search request is not supported by the rest source");
                throw UnsupportedRequestException({}, {}, std::move(unsupportedQueryParameters), std::move(requiredQueryParameters));
            }
        }

        return result;
    }

    PackageMatchField Interface::ConvertStringToPackageMatchField(std::string_view field) const
    {
        std::string toLower = Utility::ToLower(field);

        if (toLower == "command")
        {
            return PackageMatchField::Command;
        }
        else if (toLower == "packageidentifier")
        {
            return PackageMatchField::Id;
        }
        else if (toLower == "moniker")
        {
            return PackageMatchField::Moniker;
        }
        else if (toLower == "packagename")
        {
            return PackageMatchField::Name;
        }
        else if (toLower == "tag")
        {
            return PackageMatchField::Tag;
        }
        else if (toLower == "packagefamilyname")
        {
            return PackageMatchField::PackageFamilyName;
        }
        else if (toLower == "productcode")
        {
            return PackageMatchField::ProductCode;
        }
        else if (toLower == "normalizedpackagenameandpublisher")
        {
            return PackageMatchField::NormalizedNameAndPublisher;
        }
        else if (toLower == "market")
        {
            return PackageMatchField::Market;
        }

        return PackageMatchField::Unknown;
    }
}
