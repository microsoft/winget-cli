// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageCatalog.h"
#include "PackageCatalog.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    bool PackageCatalog::IsComposite()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogInfo PackageCatalog::Info()
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::FindPackagesResult> PackageCatalog::FindPackagesAsync(winrt::Microsoft::Management::Deployment::FindPackagesOptions options)
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::FindPackagesResult PackageCatalog::FindPackages(winrt::Microsoft::Management::Deployment::FindPackagesOptions const& options)
    {
        UNREFERENCED_PARAMETER(options);
        throw hresult_not_implemented();
    }
}
