// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "PackageManager.h"
#include "PackageManagerSettings.h"
#pragma warning( pop )
#include "PackageManagerSettings.g.cpp"
#include "Helpers.h"
#include "Public/CanUnload.h"
#include <winget/UserSettings.h>
#include <AppInstallerRuntime.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    bool PackageManagerSettings::SetCallerIdentifier(hstring const& callerIdentifier)
    {
        bool success = false;
        static std::once_flag setCallerOnceFlag;
        std::call_once(setCallerOnceFlag,
            [&]()
            {
                SetComCallerName(AppInstaller::Utility::ConvertToUTF8(callerIdentifier));
                success = true;
            });
        return success;
    }
    bool PackageManagerSettings::SetStateIdentifier(hstring const& stateIdentifier)
    {
        bool success = false;
        static std::once_flag setStateOnceFlag;
        std::call_once(setStateOnceFlag,
            [&]()
            {
                AppInstaller::Runtime::SetRuntimePathStateName(AppInstaller::Utility::ConvertToUTF8(stateIdentifier));
                success = true;
            });
        return success;
    }
    bool PackageManagerSettings::SetUserSettings(hstring const& settingsContent)
    {
        bool success = false;
        static std::once_flag setSettingsOnceFlag;
        std::call_once(setSettingsOnceFlag,
            [&]()
            {
                success = AppInstaller::Settings::TryInitializeCustomUserSettings(AppInstaller::Utility::ConvertToUTF8(settingsContent));
                if (success)
                {
                    AppInstaller::Logging::Log().SetEnabledChannels(AppInstaller::Settings::User().Get<AppInstaller::Settings::Setting::LoggingChannelPreference>());
                    AppInstaller::Logging::Log().SetLevel(AppInstaller::Settings::User().Get<AppInstaller::Settings::Setting::LoggingLevelPreference>());
                }
            });
        return success;
    }

    bool PackageManagerSettings::CanUnloadPreference() const
    {
        return GetCanUnload();
    }

    void PackageManagerSettings::CanUnloadPreference(bool value)
    {
        return SetCanUnload(value);
    }

    CoCreatableMicrosoftManagementDeploymentClass(PackageManagerSettings);
}
