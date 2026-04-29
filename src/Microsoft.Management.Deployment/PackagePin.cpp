// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackagePin.h"
#include "PackagePin.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    namespace
    {
        winrt::Microsoft::Management::Deployment::PackagePinType ConvertPinType(::AppInstaller::Pinning::PinType type)
        {
            switch (type)
            {
            case ::AppInstaller::Pinning::PinType::PinnedByManifest:
                return winrt::Microsoft::Management::Deployment::PackagePinType::PinnedByManifest;
            case ::AppInstaller::Pinning::PinType::Pinning:
                return winrt::Microsoft::Management::Deployment::PackagePinType::Pinning;
            case ::AppInstaller::Pinning::PinType::Gating:
                return winrt::Microsoft::Management::Deployment::PackagePinType::Gating;
            case ::AppInstaller::Pinning::PinType::Blocking:
                return winrt::Microsoft::Management::Deployment::PackagePinType::Blocking;
            default:
                return winrt::Microsoft::Management::Deployment::PackagePinType::Unknown;
            }
        }
    }

    void PackagePin::Initialize(const ::AppInstaller::Pinning::Pin& pin)
    {
        m_packageId = winrt::to_hstring(pin.GetKey().PackageId);
        m_sourceId = winrt::to_hstring(pin.GetKey().SourceId);
        m_type = ConvertPinType(pin.GetType());
        m_gatedVersion = winrt::to_hstring(pin.GetGatedVersion().ToString());
        m_dateAdded = winrt::to_hstring(pin.GetDateAdded());
        m_note = pin.GetNote() ? winrt::to_hstring(*pin.GetNote()) : hstring{};
        m_isForInstalledPackage = pin.GetKey().IsForInstalled();
    }

    hstring PackagePin::PackageId()
    {
        return m_packageId;
    }

    hstring PackagePin::SourceId()
    {
        return m_sourceId;
    }

    winrt::Microsoft::Management::Deployment::PackagePinType PackagePin::Type()
    {
        return m_type;
    }

    hstring PackagePin::GatedVersion()
    {
        return m_gatedVersion;
    }

    hstring PackagePin::DateAdded()
    {
        return m_dateAdded;
    }

    hstring PackagePin::Note()
    {
        return m_note;
    }

    bool PackagePin::IsForInstalledPackage()
    {
        return m_isForInstalledPackage;
    }
}
