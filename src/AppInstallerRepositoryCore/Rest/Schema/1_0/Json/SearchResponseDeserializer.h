// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <cpprest/json.h>
#include "Rest/Schema/IRestClient.h"

namespace AppInstaller::Repository::Rest::Schema::V1_0::Json
{
    // Search Result Deserializer.
    struct SearchResponseDeserializer
    {
        // Gets the search result for given version
        IRestClient::SearchResult Deserialize(const web::json::value& searchResultJsonObject) const;

    protected:
        virtual std::optional<IRestClient::SearchResult> DeserializeSearchResult(const web::json::value& searchResultJsonObject) const;
        virtual std::optional<IRestClient::VersionInfo> DeserializeVersionInfo(const web::json::value& versionInfoJsonObject) const;
    };
}
