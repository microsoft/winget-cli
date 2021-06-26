// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageCatalog.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageCatalog : PackageCatalogT<PackageCatalog>
    {
        PackageCatalog() = default;

        bool IsComposite();
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo Info();
        winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::FindPackagesResult> FindPackagesAsync(winrt::Microsoft::Management::Deployment::FindPackagesOptions options);
        winrt::Microsoft::Management::Deployment::FindPackagesResult FindPackages(winrt::Microsoft::Management::Deployment::FindPackagesOptions const& options);
    };
}
