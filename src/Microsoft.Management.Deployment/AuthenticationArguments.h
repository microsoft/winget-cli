// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AuthenticationArguments.g.h"
#include "Public/ComClsids.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_AuthenticationArguments)]
    struct AuthenticationArguments : AuthenticationArgumentsT<AuthenticationArguments>
    {
        AuthenticationArguments() = default;

        winrt::Microsoft::Management::Deployment::AuthenticationBehavior AuthenticationBehavior();
        void AuthenticationBehavior(winrt::Microsoft::Management::Deployment::AuthenticationBehavior const& value);
        hstring AuthenticationAccount();
        void AuthenticationAccount(hstring const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::AuthenticationBehavior m_authenticationBehavior = winrt::Microsoft::Management::Deployment::AuthenticationBehavior::Silent;
        std::wstring m_authenticationAccount = L"";
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct AuthenticationArguments : AuthenticationArgumentsT<AuthenticationArguments, implementation::AuthenticationArguments>
    {
    };
}
#endif