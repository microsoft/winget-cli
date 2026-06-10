// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <cpprest/json.h>
#include "Rest/Schema/1_0/Json/SearchResponseDeserializer.h"

namespace AppInstaller::Repository::Rest::Schema::V1_4::Json
{
    // Search Result Deserializer.
    struct SearchResponseDeserializer : public V1_0::Json::SearchResponseDeserializer
    {
    protected:
        std::optional<IRestClient::VersionInfo> DeserializeVersionInfo(const web::json::value& versionInfoJsonObject) const;
    };
}
