// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Authentication.h"
#include "WebAccountManagerAuthenticator.h"
#include <AppInstallerStrings.h>

using namespace std::string_view_literals;

namespace AppInstaller::Authentication
{
    namespace
    {
        const std::string c_BearerTokenPrefix = "Bearer ";
    }

    Authenticator::Authenticator(AuthenticationInfo info, AuthenticationArguments args)
    {
        THROW_HR_IF(E_UNEXPECTED, args.Mode == AuthenticationMode::Unknown);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED, info.Type == AuthenticationType::Unknown);
        THROW_HR_IF(E_UNEXPECTED, info.Type == AuthenticationType::None);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_AUTHENTICATION_INFO, info.ValidateIntegrity());

        if (info.Type == AuthenticationType::MicrosoftEntraId)
        {
            m_authProvider = std::make_unique<WebAccountManagerAuthenticator>(std::move(info), std::move(args));
        }
    }

    AuthenticationResult Authenticator::AuthenticateForToken()
    {
        THROW_HR_IF(E_UNEXPECTED, !m_authProvider);

        return m_authProvider->AuthenticateForToken();
    }

    bool AuthenticationInfo::ValidateIntegrity()
    {
        if (Type == AuthenticationType::MicrosoftEntraId)
        {
            return MicrosoftEntraIdInfo.has_value() && !MicrosoftEntraIdInfo->Resource.empty();
        }

        return true;
    }

    AuthenticationWindowBase AuthenticationWindowBase::Create()
    {
        return AuthenticationWindowBase();
    }

    HWND AuthenticationWindowBase::GetHandle()
    {
        return m_windowHandle.get();
    }

    std::string_view AuthenticationTypeToString(AuthenticationType in)
    {
        switch (in)
        {
        case AuthenticationType::None:
            return "none"sv;
        case AuthenticationType::MicrosoftEntraId:
            return "microsoftEntraId"sv;
        }

        return "unknown"sv;
    }

    AuthenticationType ConvertToAuthenticationType(std::string_view in)
    {
        std::string inStrLower = Utility::ToLower(in);
        AuthenticationType result = AuthenticationType::Unknown;

        if (inStrLower == "none")
        {
            result = AuthenticationType::None;
        }
        else if (inStrLower == "microsoftentraid")
        {
            result = AuthenticationType::MicrosoftEntraId;
        }

        return result;
    }

    std::string_view AuthenticationModeToString(AuthenticationMode in)
    {
        switch (in)
        {
        case AuthenticationMode::Silent:
            return "silent"sv;
        case AuthenticationMode::SilentPreferred:
            return "silentPreferred"sv;
        case AuthenticationMode::Interactive:
            return "interactive"sv;
        }

        return "unknown"sv;
    }

    AuthenticationMode ConvertToAuthenticationMode(std::string_view in)
    {
        std::string inStrLower = Utility::ToLower(in);
        AuthenticationMode result = AuthenticationMode::Unknown;

        if (inStrLower == "silent")
        {
            result = AuthenticationMode::Silent;
        }
        else if (inStrLower == "silentpreferred")
        {
            result = AuthenticationMode::SilentPreferred;
        }
        else if (inStrLower == "interactive")
        {
            result = AuthenticationMode::Interactive;
        }

        return result;
    }

    std::string AppInstaller::Authentication::CreateBearerToken(std::string rawToken)
    {
        return c_BearerTokenPrefix + rawToken;
    }
}
