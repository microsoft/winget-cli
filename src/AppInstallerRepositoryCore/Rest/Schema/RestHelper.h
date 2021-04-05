// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "cpprest/json.h"

namespace AppInstaller::Repository::Rest::Schema
{
    // Rest source helper.
    struct RestHelper
    {
        static utility::string_t GetRestAPIBaseUri(std::string restApiUri);

        static bool IsValidUri(const utility::string_t& restApiUri);

        static utility::string_t AppendPathToUri(const utility::string_t& restApiUri, const utility::string_t& path);

        static utility::string_t MakeQueryParam(std::string_view queryName, const std::string& queryValue);

        static utility::string_t AppendQueryParamsToUri(const utility::string_t& uri, const std::map<std::string_view, std::string>& queryParameters);

        static std::optional<utility::string_t> GetContinuationToken(const web::json::value& jsonObject);
    };
}
