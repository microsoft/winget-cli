// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <atomic>
#include "Public/winget/Authentication.h"
#include <winrt/Windows.Security.Authentication.Web.Core.h>
#include <winrt/Windows.Security.Credentials.h>

namespace AppInstaller::Authentication
{
    struct WebAccountManagerAuthenticator : public IAuthenticationProvider
    {
        WebAccountManagerAuthenticator(AuthenticationInfo info, AuthenticationArguments args);

        AuthenticationResult AuthenticateForToken();

    private:
        AuthenticationInfo m_authInfo;
        AuthenticationArguments m_authArgs;
        winrt::Windows::Security::Credentials::WebAccountProvider m_webAccountProvider = nullptr;
        winrt::Windows::Security::Credentials::WebAccount m_authenticatedAccount = nullptr;
        std::mutex m_authLock;

        winrt::Windows::Security::Credentials::WebAccount FindWebAccount(std::string_view accountName);
        winrt::Windows::Security::Authentication::Web::Core::WebTokenRequest CreateTokenRequest(bool forceInteractive);
        AuthenticationResult GetToken(winrt::Windows::Security::Credentials::WebAccount webAccount, bool forceInteractive = false);
        AuthenticationResult GetTokenSilent(winrt::Windows::Security::Credentials::WebAccount webAccount);
        AuthenticationResult HandleGetTokenResult(winrt::Windows::Security::Authentication::Web::Core::WebTokenRequestResult requestResult);

        bool IsMicrosoftEntraIdAuthenticationType();
    };
}
