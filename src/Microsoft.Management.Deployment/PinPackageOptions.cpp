// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "PinPackageOptions.h"
#pragma warning( pop )
#include "PinPackageOptions.g.cpp"
#include "Helpers.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::PackagePinType PinPackageOptions::PinType()
    {
        return m_pinType;
    }

    void PinPackageOptions::PinType(winrt::Microsoft::Management::Deployment::PackagePinType value)
    {
        m_pinType = value;
    }

    hstring PinPackageOptions::GatedVersion()
    {
        return m_gatedVersion;
    }

    void PinPackageOptions::GatedVersion(hstring const& value)
    {
        m_gatedVersion = value;
    }

    bool PinPackageOptions::PinInstalledPackage()
    {
        return m_pinInstalledPackage;
    }

    void PinPackageOptions::PinInstalledPackage(bool value)
    {
        m_pinInstalledPackage = value;
    }

    bool PinPackageOptions::Force()
    {
        return m_force;
    }

    void PinPackageOptions::Force(bool value)
    {
        m_force = value;
    }

    hstring PinPackageOptions::Note()
    {
        return m_note;
    }

    void PinPackageOptions::Note(hstring const& value)
    {
        m_note = value;
    }

    CoCreatableMicrosoftManagementDeploymentClass(PinPackageOptions);
}
