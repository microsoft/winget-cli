// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/1_6/Interface.h"

namespace AppInstaller::Repository::Rest::Schema::V1_7
{
    // Interface to this schema version exposed through IRestClient.
    struct Interface : public V1_6::Interface
    {
        Interface(const std::string& restApi, IRestClient::Information information, const HttpClientHelper::HttpRequestHeaders& additionalHeaders = {}, Authentication::AuthenticationArguments authArgs = {}, const HttpClientHelper& httpClientHelper = {});

        Interface(const Interface&) = delete;
        Interface& operator=(const Interface&) = delete;

        Interface(Interface&&) = default;
        Interface& operator=(Interface&&) = default;

        Utility::Version GetVersion() const override;

        HttpClientHelper::HttpRequestHeaders GetAuthHeaders() const override;

    protected:
        std::unique_ptr<Authentication::Authenticator> m_authenticator;
        Authentication::AuthenticationArguments m_authArgs;
    };
}
