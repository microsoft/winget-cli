// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageManagerSettings.g.h"
#include "Public/ComClsids.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_INPROC_ONLY_COM_CLSID_PackageManagerSettings)]
    struct PackageManagerSettings : PackageManagerSettingsT<PackageManagerSettings>
    {
        PackageManagerSettings() = default;

        // Contract 4.0
        bool SetCallerIdentifier(hstring const& callerIdentifier);
        bool SetStateIdentifier(hstring const& stateIdentifier);
        bool SetUserSettings(hstring const& settingsContent);
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PackageManagerSettings : PackageManagerSettingsT<PackageManagerSettings, implementation::PackageManagerSettings>
    {
    };
}
#endif
