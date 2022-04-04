#pragma once
#include "PackageManagerSettings.g.h"

namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PackageManagerSettings : PackageManagerSettingsT<PackageManagerSettings, implementation::PackageManagerSettings>
    {
        auto ActivateInstance() const
        {
            return winrt::create_instance<winrt::Microsoft::Management::Deployment::PackageManagerSettings>(__uuidof(implementation::PackageManagerSettings), CLSCTX_ALL);
        }
    };
}
