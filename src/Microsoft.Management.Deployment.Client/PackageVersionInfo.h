// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageVersionInfo.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageVersionInfo : PackageVersionInfoT<PackageVersionInfo>
    {
        PackageVersionInfo() = default;

        hstring GetMetadata(winrt::Microsoft::Management::Deployment::PackageVersionMetadataField const& metadataField);
        hstring Id();
        hstring DisplayName();
        hstring Version();
        hstring Channel();
        winrt::Windows::Foundation::Collections::IVectorView<hstring> PackageFamilyNames();
        winrt::Windows::Foundation::Collections::IVectorView<hstring> ProductCodes();
        winrt::Microsoft::Management::Deployment::PackageCatalog PackageCatalog();
    };
}
