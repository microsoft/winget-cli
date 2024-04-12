// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_7/Interface.h"
#include "Rest/Schema/CommonRestConstants.h"
#include "Rest/Schema/IRestClient.h"
#include <winget/HttpClientHelper.h>
#include <winget/JsonUtil.h>

namespace AppInstaller::Repository::Rest::Schema::V1_7
{
    Interface::Interface(
        const std::string& restApi,
        const Http::HttpClientHelper& httpClientHelper,
        IRestClient::Information information,
        const Http::HttpClientHelper::HttpRequestHeaders& additionalHeaders,
        Authentication::AuthenticationArguments authArgs) : V1_6::Interface(restApi, httpClientHelper, std::move(information), additionalHeaders), m_authArgs(std::move(authArgs))
    {
        m_requiredRestApiHeaders[JSON::GetUtilityString(ContractVersion)] = JSON::GetUtilityString(Version_1_7_0.ToString());

        if (m_information.Authentication.Type == Authentication::AuthenticationType::MicrosoftEntraId)
        {
            AICLI_LOG(Repo, Info, << "Creating authenticator for MicrosoftEntraId authentication. Source Identifier: " << m_information.SourceIdentifier);
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

    Http::HttpClientHelper::HttpRequestHeaders Interface::GetAuthHeaders() const
    {
        Http::HttpClientHelper::HttpRequestHeaders result;

        if (m_information.Authentication.Type == Authentication::AuthenticationType::MicrosoftEntraId)
        {
            auto authResult = m_authenticator->AuthenticateForToken();
            if (FAILED(authResult.Status))
            {
                AICLI_LOG(Repo, Error, << "Authentication failed. Result: " << authResult.Status);
                THROW_HR_MSG(authResult.Status, "Failed to authenticate for MicrosoftEntraId");
            }
            result.insert_or_assign(web::http::header_names::authorization, JSON::GetUtilityString(Authentication::CreateBearerToken(authResult.Token)));
        }

        return result;
    }
}
