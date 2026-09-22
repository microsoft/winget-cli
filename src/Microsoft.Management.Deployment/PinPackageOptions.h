// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PinPackageOptions.g.h"
#include "public/ComClsids.h"
#include <winget/ModuleCountBase.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_PinPackageOptions)]
    struct PinPackageOptions : PinPackageOptionsT<PinPackageOptions>
    {
        PinPackageOptions() = default;

        winrt::Microsoft::Management::Deployment::PackagePinType PinType();
        void PinType(winrt::Microsoft::Management::Deployment::PackagePinType value);

        hstring GatedVersion();
        void GatedVersion(hstring const& value);

        bool PinInstalledPackage();
        void PinInstalledPackage(bool value);

        bool Force();
        void Force(bool value);

        hstring Note();
        void Note(hstring const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::PackagePinType m_pinType = winrt::Microsoft::Management::Deployment::PackagePinType::Pinning;
        hstring m_gatedVersion;
        bool m_pinInstalledPackage = false;
        bool m_force = false;
        hstring m_note;
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PinPackageOptions :
        PinPackageOptionsT<PinPackageOptions, implementation::PinPackageOptions>,
        AppInstaller::WinRT::ModuleCountBase
    {
    };
}
#endif
