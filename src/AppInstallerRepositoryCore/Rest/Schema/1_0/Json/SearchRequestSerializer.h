// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <cpprest/json.h>
#include "Rest/Schema/IRestClient.h"

namespace AppInstaller::Repository::Rest::Schema::V1_0::Json
{
    // Search Result Serializer.
    struct SearchRequestSerializer
    {
        web::json::value Serialize(const SearchRequest& searchRequest) const;

    protected:
        std::optional<web::json::value> SerializeSearchRequest(const SearchRequest& searchRequest) const;

        std::optional<web::json::value> GetRequestMatchJsonObject(const AppInstaller::Repository::RequestMatch& requestMatch) const;

        std::optional<web::json::value> GetPackageMatchFilterJsonObject(const PackageMatchFilter& packageMatchFilter) const;
    };
}
