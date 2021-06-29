// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "FindPackagesOptions.h"
#pragma warning( pop )
#include "FindPackagesOptions.g.cpp"
#include "Helpers.h"

const GUID FindPackagesOptionsCLSID1 = { 0x572DED96, 0x9C60, 0x4526, { 0x8F, 0x92, 0xEE, 0x7D, 0x91, 0xD3, 0x8C, 0x1A } }; //572DED96-9C60-4526-8F92-EE7D91D38C1A
const GUID FindPackagesOptionsCLSID2 = { 0x2CAD6C15, 0xDF8E, 0x49DD, { 0xA7, 0x48, 0x96, 0xAD, 0xE0, 0xFE, 0x31, 0xB7 } }; //2CAD6C15-DF8E-49DD-A748-96ADE0FE31B7

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> FindPackagesOptions::Selectors()
    {
        return m_selectors;
    }
    winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> FindPackagesOptions::Filters()
    {
        return m_filters;
    }
    uint32_t FindPackagesOptions::ResultLimit()
    {
        return m_resultLimit;
    }
    void FindPackagesOptions::ResultLimit(uint32_t value)
    {
        m_resultLimit = value;
    }
    CoCreatableCppWinRtClassWithCLSID(FindPackagesOptions, 1, &FindPackagesOptionsCLSID1);
    CoCreatableCppWinRtClassWithCLSID(FindPackagesOptions, 2, &FindPackagesOptionsCLSID2);
}
