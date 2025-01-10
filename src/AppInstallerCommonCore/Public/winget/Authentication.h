// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <string_view>
#include <optional>
#include "AppInstallerErrors.h"

namespace AppInstaller::Authentication
{
    // The authentication type supported
    enum class AuthenticationType
    {
        Unknown,
        None,
        MicrosoftEntraId,
        MicrosoftEntraIdForAzureBlobStorage,
    };

    std::string_view AuthenticationTypeToString(AuthenticationType in);
    AuthenticationType ConvertToAuthenticationType(std::string_view in);

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

    std::string_view AuthenticationModeToString(AuthenticationMode in);
    AuthenticationMode ConvertToAuthenticationMode(std::string_view in);

    // Authentication info for Microsoft Entra Id authentication;
    struct MicrosoftEntraIdAuthenticationInfo
    {
        // Resource is required
        std::string Resource;

        // Scope is optional
        std::string Scope;

        bool operator<(const MicrosoftEntraIdAuthenticationInfo& other) const;
    };

    // Authentication info struct used to initialize Authenticator, this is from source information.
    struct AuthenticationInfo
    {
        AuthenticationType Type = AuthenticationType::None;
        std::optional<MicrosoftEntraIdAuthenticationInfo> MicrosoftEntraIdInfo;

        bool operator<(const AuthenticationInfo& other) const;

        // Update default values for missing required fields for known authentication type.
        void UpdateRequiredFieldsIfNecessary();

        // Validates data integrity against known authentication type.
        bool ValidateIntegrity() const;
    };

    // Authentication arguments struct used to initialize Authenticator, this is from user input.
    struct AuthenticationArguments
    {
        AuthenticationMode Mode = AuthenticationMode::Unknown;

        // Optional. If set, the value will be used to acquire the specific account and also be validated with authentication result.
        std::string AuthenticationAccount;
    };

    // The authentication result
    struct AuthenticationResult
    {
        // Default to failed. S_OK on authentication success.
        HRESULT Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED;

        // The token result on authentication success.
        std::string Token;
    };

    // Individual authentication provider interface. Authenticator will delegate authentication to authentication provider.
    struct IAuthenticationProvider
    {
        virtual ~IAuthenticationProvider() = default;

        // Authenticate and return string result.
        virtual AuthenticationResult AuthenticateForToken() = 0;
    };

    // The public facing authenticator
    struct Authenticator
    {
        Authenticator(AuthenticationInfo info, AuthenticationArguments args);

        // Authenticate and return string result.
        AuthenticationResult AuthenticateForToken();

    private:
        std::unique_ptr<IAuthenticationProvider> m_authProvider;
    };

    // This is the class for authentication window parent window.
    // When authenticating interactively, some api needs handle to a parent window.
    // This class will initiate a new thread and create a hidden window but with foreground priority (best effort).
    // This class handles terminating the window thread on destruction
    struct AuthenticationWindowBase
    {
        // The constructor will initiate the authentication parent window and thread
        AuthenticationWindowBase();

        AuthenticationWindowBase(const AuthenticationWindowBase&) = delete;
        AuthenticationWindowBase& operator=(const AuthenticationWindowBase&) = delete;

        AuthenticationWindowBase(AuthenticationWindowBase&&) = delete;
        AuthenticationWindowBase& operator=(AuthenticationWindowBase&&) = delete;

        // Get the native window handle
        HWND GetHandle();

        // The destructor will terminate the authentication parent window and thread
        ~AuthenticationWindowBase();

    private:
        HWND m_windowHandle;
        DWORD m_windowThreadId;
        std::thread m_windowThread;
        // In case PostThreadMessage() fails, let window thread exit immediately.
        std::atomic<bool> m_terminateWindowThread = false;

        void InitializeWindowThread();
        static LRESULT WINAPI WindowProcessFunction(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    };

    // Create bearer token from a raw token
    std::string CreateBearerToken(std::string rawToken);
}
