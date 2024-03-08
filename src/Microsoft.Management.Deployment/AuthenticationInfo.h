// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AuthenticationInfo.g.h"
#include <winget/Authentication.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AuthenticationInfo : AuthenticationInfoT<AuthenticationInfo>
    {
        AuthenticationInfo() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(::AppInstaller::Authentication::AuthenticationInfo authenticationInfo);
#endif

        winrt::Microsoft::Management::Deployment::AuthenticationType AuthenticationType();
        winrt::Microsoft::Management::Deployment::MicrosoftEntraIdAuthenticationInfo MicrosoftEntraIdAuthenticationInfo();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::AuthenticationType m_authenticationType = winrt::Microsoft::Management::Deployment::AuthenticationType::None;
        winrt::Microsoft::Management::Deployment::MicrosoftEntraIdAuthenticationInfo m_microsoftEntraIdAuthenticationInfo{ nullptr };
#endif
    };
}
