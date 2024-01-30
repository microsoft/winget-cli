// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>
#include "WebAccountManagerAuthenticator.h"

using namespace std::string_view_literals;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Security::Authentication::Web::Core;
using namespace winrt::Windows::Security::Credentials;

namespace AppInstaller::Authentication
{
    namespace
    {
        constexpr std::wstring_view s_MicrosoftEntraIdProviderId = L"https://login.microsoft.com"sv;
        constexpr std::wstring_view s_MicrosoftEntraIdAuthority = L"organizations"sv;
        constexpr std::wstring_view s_MicrosoftEntraIdClientId = L"7b8ea11a-7f45-4b3a-ab51-794d5863af15"sv;
        constexpr std::wstring_view s_MicrosoftEntraIdResourceHeader = L"resource"sv;
        constexpr std::wstring_view s_MicrosoftEntraIdLoginHintHeader = L"LoginHint"sv;
    }

    WebAccountManagerAuthenticator::WebAccountManagerAuthenticator(AuthenticationInfo info, AuthenticationArguments args) : m_authInfo(std::move(info)), m_authArgs(std::move(args))
    {
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_AUTHENTICATION_INFO, !m_authInfo.ValidateIntegrity());
        THROW_HR_IF(E_UNEXPECTED, m_authArgs.Mode == AuthenticationMode::Unknown);

        if (m_authInfo.Type == AuthenticationType::MicrosoftEntraId)
        {
            m_webAccountProvider = WebAuthenticationCoreManager::FindAccountProviderAsync(s_MicrosoftEntraIdProviderId, s_MicrosoftEntraIdAuthority).get();
            THROW_HR_IF_MSG(E_UNEXPECTED, !m_webAccountProvider, "Authentication Provider not found for Microsoft Entra Id");
        }
        else if (m_authInfo.Type == AuthenticationType::None)
        {
            THROW_HR_MSG(E_UNEXPECTED, "WebAccountManagerAuthenticator initialized with authentication type none");
        }
        else
        {
            THROW_HR(APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED);
        }
    }

    AuthenticationResult WebAccountManagerAuthenticator::AuthenticateForToken()
    {
        std::lock_guard<std::mutex> lock{ m_authLock };

        AuthenticationResult result;

        if (!m_authenticatedAccount)
        {
            // This is the first time invocation or previous authentication failed

            // Find the account to use if user provided account name and the account is signed in before. Best effort only.
            WebAccount webAccount = nullptr;
            if (!m_authArgs.AuthenticationAccount.empty())
            {
                webAccount = FindWebAccount(m_authArgs.AuthenticationAccount);
            }

            if (m_authArgs.Mode == AuthenticationMode::Interactive)
            {
                result = GetToken(webAccount, true);
            }
            else if (m_authArgs.Mode == AuthenticationMode::SilentPreferred)
            {
                result = GetTokenSilent(webAccount);
                if (FAILED(result.Status))
                {
                    result = GetToken(webAccount);
                }
            }
            else if (m_authArgs.Mode == AuthenticationMode::Silent)
            {
                result = GetTokenSilent(webAccount);
            }
        }
        else
        {
            result = GetTokenSilent(m_authenticatedAccount);
            if (FAILED(result.Status) && m_authArgs.Mode != AuthenticationMode::Silent)
            {
                result = GetToken(m_authenticatedAccount);
            }
        }

        return result;
    }

    WebAccount WebAccountManagerAuthenticator::FindWebAccount(std::string_view accountName)
    {
        WebAccount result = nullptr;

        if (accountName.empty())
        {
            return result;
        }

        if (m_authInfo.Type == AuthenticationType::MicrosoftEntraId)
        {
            auto findAccountsResult = WebAuthenticationCoreManager::FindAllAccountsAsync(m_webAccountProvider, s_MicrosoftEntraIdClientId).get();
            if (findAccountsResult.Status() == FindAllWebAccountsStatus::Success)
            {
                for (auto const& account : findAccountsResult.Accounts())
                {
                    if (Utility::CaseInsensitiveEquals(accountName, Utility::ConvertToUTF8(account.UserName())))
                    {
                        result = account;
                        break;
                    }
                }
            }
            else
            {
                // Log info
            }
        }

        return result;
    }

    WebTokenRequest WebAccountManagerAuthenticator::CreateTokenRequest(bool forceInteractive)
    {
        WebTokenRequest request = nullptr;

        if (m_authInfo.Type == AuthenticationType::MicrosoftEntraId)
        {
            request = WebTokenRequest
            {
                m_webAccountProvider,
                Utility::ConvertToUTF16(m_authInfo.MicrosoftEntraIdInfo->Scope),
                s_MicrosoftEntraIdClientId,
                forceInteractive ? WebTokenRequestPromptType::ForceAuthentication : WebTokenRequestPromptType::Default
            };

            request.Properties().Insert(s_MicrosoftEntraIdResourceHeader, Utility::ConvertToUTF16(m_authInfo.MicrosoftEntraIdInfo->Resource));
            if (!m_authArgs.AuthenticationAccount.empty())
            {
                request.Properties().Insert(s_MicrosoftEntraIdLoginHintHeader, Utility::ConvertToUTF16(m_authArgs.AuthenticationAccount));
            }
        }

        return request;
    }

    AuthenticationResult WebAccountManagerAuthenticator::GetToken(WebAccount webAccount, bool forceInteractive)
    {
        auto request = CreateTokenRequest(forceInteractive);
        if (!request)
        {
            return {};
        }

        IAsyncOperation<WebTokenRequestResult> requestOperation;
        constexpr winrt::guid iidAsyncRequestResult{ winrt::guid_of<IAsyncOperation<WebTokenRequestResult>>() };
        auto authManagerFactory = winrt::get_activation_factory<WebAuthenticationCoreManager>();
        winrt::com_ptr<IWebAuthenticationCoreManagerInterop> authManagerInterop{ authManagerFactory.as<IWebAuthenticationCoreManagerInterop>() };

        HRESULT requestOperationResult = APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED;
        AuthenticationWindowBase parentWindow;
        if (webAccount)
        {
            requestOperationResult = authManagerInterop->RequestTokenWithWebAccountForWindowAsync(
                parentWindow.GetHandle(),
                request.as<::IInspectable>().get(),
                webAccount.as<::IInspectable>().get(),
                iidAsyncRequestResult,
                reinterpret_cast<void**>(&requestOperation));
        }
        else
        {
            requestOperationResult = authManagerInterop->RequestTokenForWindowAsync(
                parentWindow.GetHandle(),
                request.as<::IInspectable>().get(),
                iidAsyncRequestResult,
                reinterpret_cast<void**>(&requestOperation));
        }

        if (FAILED(requestOperationResult))
        {
            return {};
        }

        return HandleGetTokenResult(requestOperation.get());
    }

    AuthenticationResult WebAccountManagerAuthenticator::GetTokenSilent(WebAccount webAccount)
    {
        auto request = CreateTokenRequest(false);
        if (!request)
        {
            return {};
        }

        return HandleGetTokenResult(WebAuthenticationCoreManager::GetTokenSilentlyAsync(request).get());
    }

    AuthenticationResult WebAccountManagerAuthenticator::HandleGetTokenResult(WebTokenRequestResult requestResult)
    {
        AuthenticationResult result;

        if (!requestResult)
        {
            return result;
        }

        if (requestResult.ResponseStatus() == WebTokenRequestStatus::Success)
        {
            auto responseData = requestResult.ResponseData().GetAt(0);
            auto authenticatedAccount = responseData.WebAccount();

            // Check token's corresponding account matches user input if applicable.
            if (m_authArgs.AuthenticationAccount.empty() || Utility::CaseInsensitiveEquals(m_authArgs.AuthenticationAccount, Utility::ConvertToUTF8(authenticatedAccount.UserName())))
            {
                result.Status = S_OK;
                result.Token = Utility::ConvertToUTF8(responseData.Token());
                m_authenticatedAccount = authenticatedAccount;
            }
            else
            {
                result.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_INCORRECT_ACCOUNT;
            }
        }
        else if (requestResult.ResponseStatus() == WebTokenRequestStatus::AccountSwitch)
        {
            result.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_INCORRECT_ACCOUNT;
        }
        else if (requestResult.ResponseStatus() == WebTokenRequestStatus::ProviderError ||
            requestResult.ResponseStatus() == WebTokenRequestStatus::AccountProviderNotAvailable)
        {
            auto responseError = requestResult.ResponseError();
            if (responseError)
            {

            }
            result.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED;
        }
        else if (requestResult.ResponseStatus() == WebTokenRequestStatus::UserCancel)
        {
            result.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_CANCELLED_BY_USER;
        }
        else if (requestResult.ResponseStatus() == WebTokenRequestStatus::UserInteractionRequired)
        {
            result.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_INTERACTIVE_REQUIRED;
        }

        return result;
    }
}
