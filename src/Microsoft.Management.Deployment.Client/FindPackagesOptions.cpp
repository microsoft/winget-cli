// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FindPackagesOptions.h"
#include "FindPackagesOptions.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> FindPackagesOptions::Selectors()
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> FindPackagesOptions::Filters()
    {
        throw hresult_not_implemented();
    }
    uint32_t FindPackagesOptions::ResultLimit()
    {
        throw hresult_not_implemented();
    }
    void FindPackagesOptions::ResultLimit(uint32_t value)
    {
        throw hresult_not_implemented();
    }
}
