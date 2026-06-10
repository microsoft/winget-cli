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

    CoCreatableMicrosoftManagementDeploymentClass(FindPackagesOptions);
}
