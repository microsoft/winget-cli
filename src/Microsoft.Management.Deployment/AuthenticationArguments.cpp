// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "AuthenticationArguments.h"
#pragma warning( pop )
#include "AuthenticationArguments.g.cpp"
#include "Helpers.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::AuthenticationMode AuthenticationArguments::AuthenticationMode()
    {
        return m_authenticationMode;
    }
    void AuthenticationArguments::AuthenticationMode(winrt::Microsoft::Management::Deployment::AuthenticationMode const& value)
    {
        m_authenticationMode = value;
    }
    hstring AuthenticationArguments::AuthenticationAccount()
    {
        return winrt::hstring(m_authenticationAccount);
    }
    void AuthenticationArguments::AuthenticationAccount(hstring const& value)
    {
        m_authenticationAccount = value;
    }
    CoCreatableMicrosoftManagementDeploymentClass(AuthenticationArguments);
}
