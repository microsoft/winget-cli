// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "cpprest/json.h"
#include "Rest/Schema/IRestClient.h"

namespace AppInstaller::Repository::Rest::Schema::Json
{
    // Information response Deserializer.
    struct InformationResponseDeserializer
    {
        // Gets the information model for given response
        std::optional<IRestClient::Information> Deserialize(const web::json::value& dataObject) const;
    };
}