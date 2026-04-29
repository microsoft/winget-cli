// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackagePin.g.h"
#include <winget/Pin.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackagePin : PackagePinT<PackagePin>
    {
        PackagePin() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(const ::AppInstaller::Pinning::Pin& pin);
#endif

        hstring PackageId();
        hstring SourceId();
        winrt::Microsoft::Management::Deployment::PackagePinType Type();
        hstring GatedVersion();
        hstring DateAdded();
        hstring Note();
        bool IsForInstalledPackage();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hstring m_packageId;
        hstring m_sourceId;
        winrt::Microsoft::Management::Deployment::PackagePinType m_type = winrt::Microsoft::Management::Deployment::PackagePinType::Unknown;
        hstring m_gatedVersion;
        hstring m_dateAdded;
        hstring m_note;
        bool m_isForInstalledPackage = false;
#endif
    };
}
