// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AuthenticationArguments.h"
#include "AuthenticationArguments.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::AuthenticationBehavior AuthenticationArguments::AuthenticationBehavior()
    {
        return m_authenticationBehavior;
    }
    void AuthenticationArguments::AuthenticationBehavior(winrt::Microsoft::Management::Deployment::AuthenticationBehavior const& value)
    {
        m_authenticationBehavior = value;
    }
    hstring AuthenticationArguments::AuthenticationAccount()
    {
        return winrt::hstring(m_authenticationAccount);
    }
    void AuthenticationArguments::AuthenticationAccount(hstring const& value)
    {
        m_authenticationAccount = value;
    }
}
