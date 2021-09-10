#pragma once
#include "PackageManager.g.h"

namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    const CLSID CLSID_PackageManager2 = { 0x74CB3139, 0xB7C5, 0x4B9E, { 0x93, 0x88, 0xE6, 0x61, 0x6D, 0xEA, 0x28, 0x8C } };  //74CB3139-B7C5-4B9E-9388-E6616DEA288C

    struct PackageManager : PackageManagerT<PackageManager, implementation::PackageManager>
    {
        auto ActivateInstance() const
        {
            return winrt::create_instance<winrt::Microsoft::Management::Deployment::PackageManager>(CLSID_PackageManager2, CLSCTX_ALL);
        }
    };
}
