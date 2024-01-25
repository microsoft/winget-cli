// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "MicrosoftEntraIdAuthenticationInfo.g.h"
#include <winget/Authentication.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct MicrosoftEntraIdAuthenticationInfo : MicrosoftEntraIdAuthenticationInfoT<MicrosoftEntraIdAuthenticationInfo>
    {
        MicrosoftEntraIdAuthenticationInfo() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(::AppInstaller::Authentication::MicrosoftEntraIdAuthenticationInfo authInfo);
#endif

        hstring Resource();
        hstring Scope();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ::AppInstaller::Authentication::MicrosoftEntraIdAuthenticationInfo m_authInfo;
#endif
    };
}
