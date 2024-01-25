// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <optional>

namespace AppInstaller::Authentication
{
    // The authentication type supported
    enum class AuthenticationType
    {
        Unknown,
        None,
        MicrosoftEntraId,
    };

    // The authentication modes
    enum class AuthenticationMode
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
    };

    // Authentication info struct used to initialize Authenticator, this is from source information.
    struct AuthenticationInfo
    {
        AuthenticationType Type = AuthenticationType::None;

        std::optional<MicrosoftEntraIdAuthenticationInfo> MicrosoftEntraIdInfo;

        bool ValidateIntegrity();
    };

    // Authentication arguments struct used to initialize Authenticator, this is from user input.
    struct AuthenticationArguments
    {
        AuthenticationMode Mode = AuthenticationMode::Unknown;

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