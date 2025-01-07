// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Authentication.h"
#include "WebAccountManagerAuthenticator.h"
#include <AppInstallerStrings.h>
#include <AppInstallerLogging.h>

using namespace std::string_view_literals;

namespace AppInstaller::Authentication
{
    namespace
    {
        constexpr std::string_view s_BearerTokenPrefix = "Bearer "sv;
        // Default Azure Blob Storage resource value. Used when manifest author did not provide specific blob resource.
        constexpr std::string_view s_DefaultAzureBlobStorageResource = "https://storage.azure.com/"sv;
    }

    Authenticator::Authenticator(AuthenticationInfo info, AuthenticationArguments args)
    {
        THROW_HR_IF(E_UNEXPECTED, args.Mode == AuthenticationMode::Unknown);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED, info.Type == AuthenticationType::Unknown);
        THROW_HR_IF(E_UNEXPECTED, info.Type == AuthenticationType::None);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_AUTHENTICATION_INFO, !info.ValidateIntegrity());

        AICLI_LOG(Core, Info, << "AuthenticationArguments values. Mode: " << AuthenticationModeToString(args.Mode) << ", Account: " << args.AuthenticationAccount);

        if (info.Type == AuthenticationType::MicrosoftEntraId || info.Type == AuthenticationType::MicrosoftEntraIdForAzureBlobStorage)
        {
            AICLI_LOG(Core, Info, << "Creating WebAccountManagerAuthenticator for " << AuthenticationTypeToString(info.Type));
            m_authProvider = std::make_unique<WebAccountManagerAuthenticator>(std::move(info), std::move(args));
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    static AuthenticationResult* s_AuthenticationResult_TestHook_Override = nullptr;

    void TestHook_SetAuthenticationResult_Override(Authentication::AuthenticationResult* authResult)
    {
        s_AuthenticationResult_TestHook_Override = authResult;
    }
#endif

    // Each authentication provider uses its own mechanism for caching.
    // Here we directly call authentication provider to authenticate.
    AuthenticationResult Authenticator::AuthenticateForToken()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_AuthenticationResult_TestHook_Override)
        {
            return *s_AuthenticationResult_TestHook_Override;
        }
#endif

        THROW_HR_IF(E_UNEXPECTED, !m_authProvider);

        return m_authProvider->AuthenticateForToken();
    }

    bool MicrosoftEntraIdAuthenticationInfo::operator<(const MicrosoftEntraIdAuthenticationInfo& other) const
    {
        // std::tie implements tuple comparison, wherein it checks the first item in the tuple,
        // iff the first elements are equal, then the second element is used for comparison, and so on
        return std::tie(Resource, Scope) < std::tie(other.Resource, other.Scope);
    }

    bool AuthenticationInfo::operator<(const AuthenticationInfo& other) const
    {
        // std::tie implements tuple comparison, wherein it checks the first item in the tuple,
        // iff the first elements are equal, then the second element is used for comparison, and so on
        return std::tie(Type, MicrosoftEntraIdInfo) < std::tie(other.Type, other.MicrosoftEntraIdInfo);
    }

    void AuthenticationInfo::UpdateRequiredFieldsIfNecessary()
    {
        // If MicrosoftEntraIdForAzureBlobStorage, populate default resource value if missing.
        if (Type == AuthenticationType::MicrosoftEntraIdForAzureBlobStorage)
        {
            if (MicrosoftEntraIdInfo.has_value())
            {
                if (MicrosoftEntraIdInfo->Resource.empty())
                {
                    MicrosoftEntraIdInfo->Resource = s_DefaultAzureBlobStorageResource;
                    MicrosoftEntraIdInfo->Scope = "";
                }
            }
            else
            {
                MicrosoftEntraIdAuthenticationInfo authInfo;
                authInfo.Resource = s_DefaultAzureBlobStorageResource;
                MicrosoftEntraIdInfo = std::move(authInfo);
            }
        }
    }

    bool AuthenticationInfo::ValidateIntegrity() const
    {
        // For MicrosoftEntraId, Resource is required.
        if (Type == AuthenticationType::MicrosoftEntraId || Type == AuthenticationType::MicrosoftEntraIdForAzureBlobStorage)
        {
            return MicrosoftEntraIdInfo.has_value() && !MicrosoftEntraIdInfo->Resource.empty();
        }

        return true;
    }

    AuthenticationWindowBase::AuthenticationWindowBase()
    {
        InitializeWindowThread();
    }

    HWND AuthenticationWindowBase::GetHandle()
    {
        return m_windowHandle;
    }

    AuthenticationWindowBase::~AuthenticationWindowBase()
    {
        if (!PostMessageW(m_windowHandle, WM_CLOSE, 0, 0))
        {
            m_terminateWindowThread = true;
        }

        if (m_windowThread.joinable())
        {
            m_windowThread.join();
        }
    }

    void AuthenticationWindowBase::InitializeWindowThread()
    {
        static std::once_flag s_registerWindowClassOnce;
        static LPCWSTR s_windowsClassName = L"WingetAuthenticationParentWindowClass";
        static HMODULE hModule = GetModuleHandle(NULL);
        THROW_LAST_ERROR_IF_NULL_MSG(hModule, "Failed to get resource module for authentication window");

        std::call_once(s_registerWindowClassOnce,
            [&]()
            {
                WNDCLASS wc = {};
                wc.lpfnWndProc = AuthenticationWindowBase::WindowProcessFunction;
                wc.hInstance = hModule;
                wc.lpszClassName = s_windowsClassName;
                THROW_LAST_ERROR_IF_MSG(!RegisterClassW(&wc), "Failed to get resource module for authentication window");
            });

        wil::unique_event waitForWindowReady;
        waitForWindowReady.create();

        m_windowThread = std::thread(
            [&]()
            {
                m_windowHandle = CreateWindowW(
                    s_windowsClassName,
                    L"WingetAuthenticationParentWindow",
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, /* size and position */
                    NULL, /* hWndParent */
                    NULL, /* hMenu */
                    hModule,
                    NULL); /* lpParam */
                THROW_LAST_ERROR_IF_NULL_MSG(hModule, "Failed to create authentication parent window");

                // Best effort only
                SetForegroundWindow(m_windowHandle);

                m_windowThreadId = GetCurrentThreadId();

                // Set window ready event
                waitForWindowReady.SetEvent();

                // Message loop
                MSG msg;
                BOOL getMsgResult;
                while ((getMsgResult = GetMessage(&msg, NULL, 0, 0)) != 0)
                {
                    if (m_terminateWindowThread || getMsgResult == -1)
                    {
                        return;
                    }
                    else
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            });

        THROW_HR_IF_MSG(APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED, !waitForWindowReady.wait(10000), "Creating authentication parent window timed out");
    }

    LRESULT __stdcall AuthenticationWindowBase::WindowProcessFunction(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_ENDSESSION:
        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }

        return 0;
    }

    std::string_view AuthenticationTypeToString(AuthenticationType in)
    {
        switch (in)
        {
        case AuthenticationType::None:
            return "none"sv;
        case AuthenticationType::MicrosoftEntraId:
            return "microsoftEntraId"sv;
        case AuthenticationType::MicrosoftEntraIdForAzureBlobStorage:
            return "microsoftEntraIdForAzureBlobStorage"sv;
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
        else if (inStrLower == "microsoftentraidforazureblobstorage")
        {
            result = AuthenticationType::MicrosoftEntraIdForAzureBlobStorage;
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
        return std::string{ s_BearerTokenPrefix } + rawToken;
    }
}
