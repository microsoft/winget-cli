#pragma once
#include "PackageManager.g.h"

namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PackageManager : PackageManagerT<PackageManager, implementation::PackageManager>
    {
        auto ActivateInstance() const
        {
            return winrt::create_instance<winrt::Microsoft::Management::Deployment::PackageManager>(__uuidof(implementation::PackageManager), CLSCTX_ALL);
        }
    };
}
