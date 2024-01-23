// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>

namespace AppInstaller::Authentication
{
    // The authentication type supported
    enum class AuthenticationType
    {
        Unknown,
        None,
        MicrosoftEntraId,
    };

    // The authentication behaviors
    enum class AuthenticationBehavior
    {
        Unknown,

        // Always do interactive authentication on first request, following requests may use cached result.
        Interactive,

        // Try silent flow first. If failed, use interactive flow.
        SilentPreferred,

        // Only do silent flow. If failed, the authentication failed.
        Silent,
    };

    // Authentication info for Microsoft Entra Id authentication;
    struct MicrosoftEntraIdAuthenticationInfo
    {
        // Resource is required
        std::string Resource;

        // Scope is optional
        std::string Scope;

        bool Empty()
        {
            return Resource.empty();
        }
    };

    // Authentication info struct used to initialize Authenticator, this is from source information.
    struct AuthenticationInfo
    {
        AuthenticationType Type = AuthenticationType::None;

        MicrosoftEntraIdAuthenticationInfo MicrosoftEntraIdInfo;
    };

    // Authentication arguments struct used to initialize Authenticator, this is from user input.
    struct AuthenticationArguments
    {
        AuthenticationBehavior Behavior = AuthenticationBehavior::Unknown;

        // Optional. If set, the value will be used to acuquire the specific account and also be validated with authentication result.
        std::string AuthenticationAccount;
    };

    struct Authenticator
    {
        Authenticator(AuthenticationInfo info, AuthenticationArguments args);

        std::string AuthenticateForToken();
    };

    std::string CreateBearerToken(std::string rawToken);
}