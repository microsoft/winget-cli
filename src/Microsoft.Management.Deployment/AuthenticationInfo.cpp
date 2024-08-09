// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AuthenticationInfo.h"
#include "AuthenticationInfo.g.cpp"
#include "MicrosoftEntraIdAuthenticationInfo.h"
#include "Converters.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void AuthenticationInfo::Initialize(::AppInstaller::Authentication::AuthenticationInfo authenticationInfo)
    {
        m_authenticationType = GetDeploymentAuthenticationType(authenticationInfo.Type);

        if (authenticationInfo.MicrosoftEntraIdInfo.has_value())
        {
            auto microsoftEntraIdAuthenticationInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::MicrosoftEntraIdAuthenticationInfo>>();
            microsoftEntraIdAuthenticationInfo->Initialize(authenticationInfo.MicrosoftEntraIdInfo.value());
            m_microsoftEntraIdAuthenticationInfo = *microsoftEntraIdAuthenticationInfo;
        }
    }
    winrt::Microsoft::Management::Deployment::AuthenticationType AuthenticationInfo::AuthenticationType()
    {
        return m_authenticationType;
    }
    winrt::Microsoft::Management::Deployment::MicrosoftEntraIdAuthenticationInfo AuthenticationInfo::MicrosoftEntraIdAuthenticationInfo()
    {
        return m_microsoftEntraIdAuthenticationInfo;
    }
}
