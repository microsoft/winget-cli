// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerStrings.h"
#include "winget/Rest.h"
#include <winget/JsonUtil.h>

namespace AppInstaller::Rest
{
    utility::string_t GetRestAPIBaseUri(std::string uri)
    {
        // Trim
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
        return web::uri::encode_uri(JSON::GetUtilityString(uri));
    }

    bool IsValidUri(const utility::string_t& restApiUri)
    {
        return web::uri::validate(restApiUri);
    }

    utility::string_t AppendPathToUri(const utility::string_t& restApiUri, const utility::string_t& path)
    {
        web::uri_builder builder(restApiUri);
        builder.append_path(path, true);
        return builder.to_string();
    }

    utility::string_t MakeQueryParam(std::string_view queryName, const std::string& queryValue)
    {
        std::string queryParam;
        queryParam.append(queryName).append("=").append(queryValue);

        return utility::conversions::to_string_t(queryParam);
    }

    utility::string_t AppendQueryParamsToUri(const utility::string_t& uri, const std::map<std::string_view, std::string>& queryParameters)
    {
        web::http::uri_builder builder{ uri };

        for (auto& pair : queryParameters)
        {
            builder.append_query(MakeQueryParam(pair.first, pair.second), true);
        }

        return builder.to_string();
    }

    std::vector<std::string> GetUniqueItems(const std::vector<std::string>& list)
    {
        std::set<std::string> set;
        for (const auto& item : list)
        {
            set.emplace(item);
        }

        std::vector<std::string> result{ set.begin(), set.end() };
        return result;
    }
}
