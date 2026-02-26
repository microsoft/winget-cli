// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/1_9/Interface.h"

namespace AppInstaller::Repository::Rest::Schema::V1_10
{
    // Interface to this schema version exposed through IRestClient.
    struct Interface : public V1_9::Interface
    {
        Interface(const std::string& restApi, const Http::HttpClientHelper& helper, IRestClient::Information information, const Http::HttpClientHelper::HttpRequestHeaders& additionalHeaders = {}, Authentication::AuthenticationArguments authArgs = {});

        Interface(const Interface&) = delete;
        Interface& operator=(const Interface&) = delete;

        Interface(Interface&&) = default;
        Interface& operator=(Interface&&) = default;

        Utility::Version GetVersion() const override;
    };
}
