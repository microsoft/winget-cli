// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <cpprest/json.h>

namespace AppInstaller::Rest
{
    utility::string_t GetRestAPIBaseUri(std::string restApiUri);

    bool IsValidUri(const utility::string_t& restApiUri);

    utility::string_t AppendPathToUri(const utility::string_t& restApiUri, const utility::string_t& path);

    utility::string_t MakeQueryParam(std::string_view queryName, const std::string& queryValue);

    utility::string_t AppendQueryParamsToUri(const utility::string_t& uri, const std::map<std::string_view, std::string>& queryParameters);

    std::vector<std::string> GetUniqueItems(const std::vector<std::string>& list);
}

