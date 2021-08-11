// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/1_0/Json/SearchResponseDeserializer.h"

namespace AppInstaller::Repository::Rest::Schema::V1_1::Json
{
    // Search Result Deserializer.
    struct SearchResponseDeserializer : public V1_0::Json::SearchResponseDeserializer
    {
    protected:
        std::optional<IRestClient::SearchResult> DeserializeSearchResult(const web::json::value& searchResultJsonObject) const override;
    };
}
