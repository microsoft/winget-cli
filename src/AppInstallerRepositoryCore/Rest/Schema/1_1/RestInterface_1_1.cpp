// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_1/Interface.h"
#include "Rest/Schema/IRestClient.h"
#include "Rest/Schema/HttpClientHelper.h"
#include "Rest/Schema/JsonHelper.h"
#include "Rest/Schema/RestHelper.h"
#include "Rest/Schema/CommonRestConstants.h"
#include "Rest/Schema/1_1/Json/ManifestDeserializer.h"
#include "Rest/Schema/1_1/Json/SearchResponseDeserializer.h"
#include "Rest/Schema/1_1/Json/SearchRequestSerializer.h"

using namespace std::string_view_literals;
using namespace AppInstaller::Repository::Rest::Schema::V1_1::Json;

namespace AppInstaller::Repository::Rest::Schema::V1_1
{
    Interface::Interface(const std::string& restApi, IRestClient::Information information, const HttpClientHelper& httpClientHelper) :
        V1_0::Interface(restApi, httpClientHelper), m_information(std::move(information))
    {
    }

    Utility::Version Interface::GetVersion() const
    {
        return Version_1_1_0;
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

                throw UnsupportedQueryException({}, {}, {}, m_information.RequiredQueryParameters);
            }
        }

        for (auto const& param : m_information.UnsupportedQueryParameters)
        {
            if (params.end() != std::find_if(params.begin(), params.end(), [&](const auto& pair) { return Utility::CaseInsensitiveEquals(pair.first, param); }))
            {
                throw UnsupportedQueryException({}, {}, m_information.UnsupportedQueryParameters, {});
            }
        }

        return result;
    }

    web::json::value Interface::GetValidatedSearchBody(const SearchRequest& searchRequest) const
    {
        SearchRequest resultSearchRequest = searchRequest;

        for (auto const& field : m_information.RequiredPackageMatchFields)
        {
            PackageMatchField matchField = StringToPackageMatchField(field);

            if (searchRequest.Filters.end() == std::find_if(searchRequest.Filters.begin(), searchRequest.Filters.end(), [&](const PackageMatchFilter& filter) { return filter.Field == matchField; }))
            {
                if (matchField == PackageMatchField::Market)
                {
                    resultSearchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Market, MatchType::CaseInsensitive, Runtime::GetOSRegion()));
                    continue;
                }

                throw UnsupportedQueryException({}, m_information.RequiredPackageMatchFields, {}, {});
            }
        }

        for (auto const& field : m_information.UnsupportedPackageMatchFields)
        {
            PackageMatchField matchField = StringToPackageMatchField(field);

            if (matchField == PackageMatchField::Unknown)
            {
                continue;
            }

            if (searchRequest.Filters.end() != std::find_if(searchRequest.Filters.begin(), searchRequest.Filters.end(), [&](const PackageMatchFilter& filter) { return filter.Field == matchField; }))
            {
                throw UnsupportedQueryException(m_information.UnsupportedPackageMatchFields, {}, {}, {});
            }
        }

        SearchRequestSerializer serializer;
        return serializer.Serialize(searchRequest);
    }
}
