// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FindPackagesOptions.h"
#include "FindPackagesOptions.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> FindPackagesOptions::Selectors()
    {
        return m_findPackagesOptions.Selectors();
    }
    winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> FindPackagesOptions::Filters()
    {
        return m_findPackagesOptions.Filters();
    }
    uint32_t FindPackagesOptions::ResultLimit()
    {
        return m_findPackagesOptions.ResultLimit();
    }
    void FindPackagesOptions::ResultLimit(uint32_t value)
    {
        m_findPackagesOptions.ResultLimit(value);
    }
}
