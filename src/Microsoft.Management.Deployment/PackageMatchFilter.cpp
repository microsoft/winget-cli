// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include <AppInstallerRepositorySearch.h>
#include "Workflows/WorkflowBase.h"
#include "Converters.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "PackageMatchFilter.h"
#pragma warning( pop )
#include "PackageMatchFilter.g.cpp"
#include "Helpers.h"

const GUID PackageMatchFilterCLSID1 = { 0xD02C9DAF, 0x99DC, 0x429C, { 0xB5, 0x03, 0x4E, 0x50, 0x4E, 0x4A, 0xB0, 0x00 } }; //D02C9DAF-99DC-429C-B503-4E504E4AB000
const GUID PackageMatchFilterCLSID2 = { 0xADBF3B4A, 0xDB8A, 0x496C, { 0xA5, 0x79, 0x62, 0xB5, 0x8F, 0x5F, 0xB1, 0x3F } }; //ADBF3B4A-DB8A-496C-A579-62B58F5FB13F

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageMatchFilter::Initialize(::AppInstaller::Repository::PackageMatchFilter matchFilter)
    {
        m_value = winrt::to_hstring(matchFilter.Value);
        m_matchField = GetDeploymentMatchField(matchFilter.Field);
        m_packageFieldMatchOption = GetDeploymentMatchOption(matchFilter.Type);
    }
    winrt::Microsoft::Management::Deployment::PackageFieldMatchOption PackageMatchFilter::Option()
    {
        return m_packageFieldMatchOption;
    }
    void PackageMatchFilter::Option(winrt::Microsoft::Management::Deployment::PackageFieldMatchOption const& value)
    {
        m_packageFieldMatchOption = value;
    }
    winrt::Microsoft::Management::Deployment::PackageMatchField PackageMatchFilter::Field()
    {
        return m_matchField;
    }
    void PackageMatchFilter::Field(winrt::Microsoft::Management::Deployment::PackageMatchField const& value)
    {
        m_matchField = value;
    }
    hstring PackageMatchFilter::Value()
    {
        return hstring(m_value);
    }
    void PackageMatchFilter::Value(hstring const& value)
    {
        m_value = value;
    }
    CoCreatableCppWinRtClassWithCLSID(PackageMatchFilter, 1, &PackageMatchFilterCLSID1);
    CoCreatableCppWinRtClassWithCLSID(PackageMatchFilter, 2, &PackageMatchFilterCLSID2);
}
