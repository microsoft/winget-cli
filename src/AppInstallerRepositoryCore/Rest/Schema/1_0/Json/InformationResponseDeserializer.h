// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <cpprest/json.h>
#include "Rest/Schema/IRestClient.h"

namespace AppInstaller::Repository::Rest::Schema::V1_0::Json
{
    // Information response Deserializer.
    struct InformationResponseDeserializer
    {
        // Gets the information model for given response
        IRestClient::Information Deserialize(const web::json::value& dataObject) const;

    protected:
        std::optional<IRestClient::Information> DeserializeInformation(const web::json::value& dataObject) const;
    };
}
