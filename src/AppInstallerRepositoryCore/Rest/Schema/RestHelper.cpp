// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestHelper.h"
#include "Rest/Schema/JsonHelper.h"
#include "Rest/Schema/1_0/Json/CommonJsonConstants.h"

using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Repository::Rest::Schema::V1_0::Json;

namespace AppInstaller::Repository::Rest::Schema
{
    utility::string_t RestHelper::GetRestAPIBaseUri(std::string restApiUri)
    {
        // Trim
        std::string uri = restApiUri;
        if (!uri.empty())
        {
            uri = AppInstaller::Utility::Trim(uri);

            // Remove trailing forward slash
            if (uri.back() == '/')
            {
                uri.pop_back();
            }
        }

        // Encode the Uri
        return web::uri::encode_uri(JsonHelper::GetUtilityString(uri));
    }

    bool RestHelper::IsValidUri(const utility::string_t& restApiUri)
    {
        return web::uri::validate(restApiUri);
    }

    utility::string_t RestHelper::AppendPathToUri(const utility::string_t& restApiUri, const utility::string_t& path)
    {
        web::uri_builder builder(restApiUri);
        builder.append_path(path, true);
        return builder.to_string();
    }

    utility::string_t RestHelper::MakeQueryParam(std::string_view queryName, const std::string& queryValue)
    {
        std::string queryParam;
        queryParam.append(queryName).append("=").append(queryValue);

        return utility::conversions::to_string_t(queryParam);
    }

    utility::string_t RestHelper::AppendQueryParamsToUri(const utility::string_t& uri, const std::map<std::string_view, std::string>& queryParameters)
    {
        web::http::uri_builder builder{ uri };

        for (auto& pair : queryParameters)
        {
            builder.append_query(RestHelper::MakeQueryParam(pair.first, pair.second), true);
        }

        return builder.to_string();
    }

    std::optional<utility::string_t> RestHelper::GetContinuationToken(const web::json::value& jsonObject)
    {
        std::optional<std::string> continuationToken = JsonHelper::GetRawStringValueFromJsonNode(jsonObject, JsonHelper::GetUtilityString(ContinuationToken));

        if (continuationToken)
        {
            return utility::conversions::to_string_t(continuationToken.value());
        }

        return {};
    }
}
