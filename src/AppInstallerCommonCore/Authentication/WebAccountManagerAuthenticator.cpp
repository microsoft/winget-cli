// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>
#include <AppInstallerLogging.h>
#include <AppInstallerRuntime.h>
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
        // WebAccountManager manages accounts as user. When running as system, it can only retrieve domain joined device token.
        // This is very rare scenario for rest source to require a device token. And it needs approval to provision winget client registration.
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), Runtime::IsRunningAsSystem());
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_AUTHENTICATION_INFO, !m_authInfo.ValidateIntegrity());
        THROW_HR_IF(E_UNEXPECTED, m_authArgs.Mode == AuthenticationMode::Unknown);

        if (IsMicrosoftEntraIdAuthenticationType())
        {
            m_webAccountProvider = WebAuthenticationCoreManager::FindAccountProviderAsync(s_MicrosoftEntraIdProviderId, s_MicrosoftEntraIdAuthority).get();
            THROW_HR_IF_MSG(E_UNEXPECTED, !m_webAccountProvider, "Authentication Provider not found for Microsoft Entra Id");
            AICLI_LOG(Core, Info, << "WebAccountManagerAuthenticator created for MicrosoftEntraId. Resource: " << m_authInfo.MicrosoftEntraIdInfo->Resource << ", Scope: " << m_authInfo.MicrosoftEntraIdInfo->Scope);
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

    // WebAccountManager manages token and cache at OS level.
    // So for each authentication request, we call WebAccountManager api to retrieve token.
    // We do not need to implement own cache logic.
    AuthenticationResult WebAccountManagerAuthenticator::AuthenticateForToken()
    {
        std::lock_guard<std::mutex> lock{ m_authLock };

        AICLI_LOG(Core, Info, << "Started WebAccountManagerAuthenticator::AuthenticateForToken.");

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
            // Previous authentication successful. Just retrieve the token with the authenticated account.
            // In rare cases silent flow fails, use interactive flow.
            result = GetTokenSilent(m_authenticatedAccount);
            if (FAILED(result.Status) && m_authArgs.Mode != AuthenticationMode::Silent)
            {
                result = GetToken(m_authenticatedAccount);
            }
        }

        AICLI_LOG(Core, Info, << "Finished WebAccountManagerAuthenticator::AuthenticateForToken. Result: " << result.Status);

        return result;
    }

    WebAccount WebAccountManagerAuthenticator::FindWebAccount(std::string_view accountName)
    {
        AICLI_LOG(Core, Info, << "FindWebAccount called. Desired Account: " << accountName);

        WebAccount result = nullptr;

        if (IsMicrosoftEntraIdAuthenticationType())
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
                AICLI_LOG(Core, Warning, << "FindAllAccountsAsync failed. Status: " << findAccountsResult.Status());
                auto providerError = findAccountsResult.ProviderError();
                if (providerError)
                {
                    AICLI_LOG(Core, Warning,
                        << "FindAllAccountsAsync Provider Error. ErrorCode: " << providerError.ErrorCode()
                        << ", Message: " << Utility::ConvertToUTF8(providerError.ErrorMessage()));
                }
            }
        }

        AICLI_LOG(Core, Info, << "FindWebAccount result: " << ((result != nullptr) ? "found" : "not found"));

        return result;
    }

    WebTokenRequest WebAccountManagerAuthenticator::CreateTokenRequest(bool forceInteractive)
    {
        WebTokenRequest request = nullptr;

        if (IsMicrosoftEntraIdAuthenticationType())
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
        AICLI_LOG(Core, Info, << "Started GetToken. ForceInteractive: " << forceInteractive);

        auto request = CreateTokenRequest(forceInteractive);
        if (!request)
        {
            AICLI_LOG(Core, Error, << "CreateTokenRequest returned empty request");
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
            AICLI_LOG(Core, Error, << "RequestTokenForWindowAsync failed. Result: " << requestOperationResult);
            return {};
        }

        return HandleGetTokenResult(requestOperation.get());
    }

    AuthenticationResult WebAccountManagerAuthenticator::GetTokenSilent(WebAccount webAccount)
    {
        AICLI_LOG(Core, Info, << "Started GetTokenSilent.");

        auto request = CreateTokenRequest(false);
        if (!request)
        {
            AICLI_LOG(Core, Error, << "CreateTokenRequest returned empty request");
            return {};
        }

        return HandleGetTokenResult(WebAuthenticationCoreManager::GetTokenSilentlyAsync(request).get());
    }

    AuthenticationResult WebAccountManagerAuthenticator::HandleGetTokenResult(WebTokenRequestResult requestResult)
    {
        AuthenticationResult result;

        if (!requestResult)
        {
            AICLI_LOG(Core, Error, << "WebTokenRequestResult is null");
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
                // Assign authenticated account for future token retrieval.
                m_authenticatedAccount = authenticatedAccount;
                AICLI_LOG(Core, Info, << "Authentication success");
            }
            else
            {
                AICLI_LOG(Core, Error, << "Authentication success. But the authenticated account is not the desired one.");
                result.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_INCORRECT_ACCOUNT;
            }
        }
        else if (requestResult.ResponseStatus() == WebTokenRequestStatus::AccountSwitch)
        {
            AICLI_LOG(Core, Error, << "Authentication failed. The authenticated account is not the desired one.");
            result.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_INCORRECT_ACCOUNT;
        }
        else if (requestResult.ResponseStatus() == WebTokenRequestStatus::ProviderError ||
            requestResult.ResponseStatus() == WebTokenRequestStatus::AccountProviderNotAvailable)
        {
            AICLI_LOG(Core, Error, << "Authentication failed. Provider failed.");
            auto responseError = requestResult.ResponseError();
            if (responseError)
            {
                AICLI_LOG(Core, Error, << "Provider Error. Code: " << responseError.ErrorCode() << ", Message: " << Utility::ConvertToUTF8(responseError.ErrorMessage()));
            }
            result.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED;
        }
        else if (requestResult.ResponseStatus() == WebTokenRequestStatus::UserCancel)
        {
            AICLI_LOG(Core, Error, << "Authentication failed. User cancelled.");
            result.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_CANCELLED_BY_USER;
        }
        else if (requestResult.ResponseStatus() == WebTokenRequestStatus::UserInteractionRequired)
        {
            AICLI_LOG(Core, Error, << "Authentication failed. Interactive authentication required.");
            result.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_INTERACTIVE_REQUIRED;
        }

        AICLI_LOG(Core, Info, << "HandleGetTokenResult Result: " << result.Status);

        return result;
    }

    bool WebAccountManagerAuthenticator::IsMicrosoftEntraIdAuthenticationType()
    {
        return m_authInfo.Type == AuthenticationType::MicrosoftEntraId || m_authInfo.Type == AuthenticationType::MicrosoftEntraIdForAzureBlobStorage;
    }
}
