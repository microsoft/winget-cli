// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/IRestClient.h"

namespace AppInstaller::Repository::Rest::Schema
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
