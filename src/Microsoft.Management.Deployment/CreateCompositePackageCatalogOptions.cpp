// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "CreateCompositePackageCatalogOptions.h"
#pragma warning( pop )
#include "CreateCompositePackageCatalogOptions.g.cpp"
#include "Helpers.h"

const GUID CreateCompositePackageCatalogOptionsCLSID1 = { 0x526534B8, 0x7E46, 0x47C8, { 0x84, 0x16, 0xB1, 0x68, 0x5C, 0x32, 0x7D, 0x37 } }; //526534B8-7E46-47C8-8416-B1685C327D37
const GUID CreateCompositePackageCatalogOptionsCLSID2 = { 0x6444B10D, 0xFE84, 0x430F, { 0x93, 0x2B, 0x3D, 0x4F, 0xE5, 0x19, 0x5B, 0xDF } }; //6444B10D-FE84-430F-932B-3D4FE5195BDF

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageCatalogReference> CreateCompositePackageCatalogOptions::Catalogs()
    {
        return m_catalogs;
    }
    winrt::Microsoft::Management::Deployment::CompositeSearchBehavior CreateCompositePackageCatalogOptions::CompositeSearchBehavior()
    {
        return m_compositeSearchBehavior;
    }
    void CreateCompositePackageCatalogOptions::CompositeSearchBehavior(winrt::Microsoft::Management::Deployment::CompositeSearchBehavior const& value)
    {
        m_compositeSearchBehavior = value;
    }
    CoCreatableCppWinRtClassWithCLSID(CreateCompositePackageCatalogOptions, 1, &CreateCompositePackageCatalogOptionsCLSID1);
    CoCreatableCppWinRtClassWithCLSID(CreateCompositePackageCatalogOptions, 2, &CreateCompositePackageCatalogOptionsCLSID2);
}
