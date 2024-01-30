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
        if (!PostThreadMessageW(m_windowThreadId, WM_CLOSE, 0, 0))
        {
            m_terminateWindowThread = true;
        }

        m_windowThread.join();
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
                    0, 0, 0, 0, /* size and position */
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
