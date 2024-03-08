// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerErrors.h>
#include <winget/GroupPolicy.h>
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    // Enable custom code to run before creating any object through the factory.
    // Currently that means requiring the overall WinGet policy to be enabled.
    template <typename TCppWinRTClass>
    class wrl_factory_for_winrt_com_class : public ::wil::wrl_factory_for_winrt_com_class<TCppWinRTClass>
    {
    public:
        IFACEMETHODIMP CreateInstance(_In_opt_::IUnknown* unknownOuter, REFIID riid, _COM_Outptr_ void** object) noexcept try
        {
            *object = nullptr;
            RETURN_HR_IF(APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY, !::AppInstaller::Settings::GroupPolicies().IsEnabled(::AppInstaller::Settings::TogglePolicy::Policy::WinGet));

            return ::wil::wrl_factory_for_winrt_com_class<TCppWinRTClass>::CreateInstance(unknownOuter, riid, object);
        }
        CATCH_RETURN()
    };

#define CoCreatableMicrosoftManagementDeploymentClass(className) \
    CoCreatableClassWithFactory(className, ::winrt::Microsoft::Management::Deployment::implementation::wrl_factory_for_winrt_com_class<className>)
}
