// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackagePin.h"
#include "PackagePin.g.cpp"
#include "Converters.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackagePin::Initialize(const ::AppInstaller::Pinning::Pin& pin)
    {
        m_packageId = winrt::to_hstring(pin.GetKey().PackageId);
        m_sourceId = winrt::to_hstring(pin.GetKey().SourceId);
        m_type = ConvertPinType(pin.GetType());
        m_gatedVersion = winrt::to_hstring(pin.GetGatedVersion().ToString());
        const auto& dateAdded = pin.GetDateAdded();
        if (dateAdded.has_value())
        {
            m_dateAdded = winrt::clock::from_sys(*dateAdded);
        }
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

    winrt::Windows::Foundation::IReference<winrt::Windows::Foundation::DateTime> PackagePin::DateAdded()
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
