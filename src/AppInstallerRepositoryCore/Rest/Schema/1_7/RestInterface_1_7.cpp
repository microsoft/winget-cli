// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_7/Interface.h"
#include "Rest/Schema/IRestClient.h"
#include "Rest/Schema/HttpClientHelper.h"
#include "Rest/Schema/CommonRestConstants.h"
#include <winget/JsonUtil.h>

namespace AppInstaller::Repository::Rest::Schema::V1_7
{
    Interface::Interface(
        const std::string& restApi,
        IRestClient::Information information,
        const HttpClientHelper::HttpRequestHeaders& additionalHeaders,
        Authentication::AuthenticationArguments authArgs,
        const HttpClientHelper& httpClientHelper) : V1_6::Interface(restApi, std::move(information), additionalHeaders, httpClientHelper), m_authArgs(std::move(authArgs))
    {
        m_requiredRestApiHeaders[JSON::GetUtilityString(ContractVersion)] = JSON::GetUtilityString(Version_1_7_0.ToString());

        if (m_information.Authentication.Type == Authentication::AuthenticationType::MicrosoftEntraId)
        {
            m_authenticator = std::make_unique<Authentication::Authenticator>(m_information.Authentication, m_authArgs);
        }
        else if (m_information.Authentication.Type == Authentication::AuthenticationType::Unknown)
        {
            AICLI_LOG(Repo, Error, << "Authentication type unknown for rest source. Source Identifier: " << m_information.SourceIdentifier);
            THROW_HR(APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED);
        }
    }

    Utility::Version Interface::GetVersion() const
    {
        return Version_1_7_0;
    }

    HttpClientHelper::HttpRequestHeaders Interface::GetAuthHeaders() const
    {
        HttpClientHelper::HttpRequestHeaders result;

        if (m_information.Authentication.Type == Authentication::AuthenticationType::MicrosoftEntraId)
        {
            auto authResult = m_authenticator->AuthenticateForToken();
            if (FAILED(authResult.Status))
            {
                AICLI_LOG(Repo, Error, << "Authentication failed. Result: " << authResult.Status);
                THROW_HR(authResult.Status);
            }
            result.insert_or_assign(web::http::header_names::authorization, JSON::GetUtilityString(authResult.Token));
        }

        return result;
    }
}
