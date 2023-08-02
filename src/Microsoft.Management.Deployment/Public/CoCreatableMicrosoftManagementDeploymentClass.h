// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerErrors.h>
#include <winget/GroupPolicy.h>
#include <wil\cppwinrt_wrl.h>
#include <Helpers.h>

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

            AppInstaller::Settings::PolicyState wingetCOMInProcOrOutOfProcPolicyState = AppInstaller::Settings::PolicyState::NotConfigured;

            if (IsOutOfProcCOMCall())
            {
                wingetCOMInProcOrOutOfProcPolicyState =  AppInstaller::Settings::GroupPolicies().GetState(::AppInstaller::Settings::TogglePolicy::Policy::WinGetOutOfProcessCOM);
            }
            else
            {
                wingetCOMInProcOrOutOfProcPolicyState = AppInstaller::Settings::GroupPolicies().GetState(::AppInstaller::Settings::TogglePolicy::Policy::WinGetInProcessCOM);
            }

            RETURN_HR_IF(APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY, wingetCOMInProcOrOutOfProcPolicyState == AppInstaller::Settings::PolicyState::Disabled);

            RETURN_HR_IF(APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY, wingetCOMInProcOrOutOfProcPolicyState == AppInstaller::Settings::PolicyState::NotConfigured && !::AppInstaller::Settings::GroupPolicies().IsEnabled(::AppInstaller::Settings::TogglePolicy::Policy::WinGet));

            return ::wil::wrl_factory_for_winrt_com_class<TCppWinRTClass>::CreateInstance(unknownOuter, riid, object);
        }
        CATCH_RETURN()
    };

#define CoCreatableMicrosoftManagementDeploymentClass(className) \
    CoCreatableClassWithFactory(className, ::winrt::Microsoft::Management::Deployment::implementation::wrl_factory_for_winrt_com_class<className>)
}
