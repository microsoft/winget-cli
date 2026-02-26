// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MicrosoftEntraIdAuthenticationInfo.h"
#include "MicrosoftEntraIdAuthenticationInfo.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void MicrosoftEntraIdAuthenticationInfo::Initialize(::AppInstaller::Authentication::MicrosoftEntraIdAuthenticationInfo authInfo)
    {
        m_authInfo = std::move(authInfo);
    }
    hstring MicrosoftEntraIdAuthenticationInfo::Resource()
    {
        return winrt::to_hstring(m_authInfo.Resource);
    }
    hstring MicrosoftEntraIdAuthenticationInfo::Scope()
    {
        return winrt::to_hstring(m_authInfo.Scope);
    }
}
