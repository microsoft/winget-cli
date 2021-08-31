// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <PackageVersionInfo.h>
#include "PackageVersionInfo.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    hstring PackageVersionInfo::GetMetadata(winrt::Microsoft::Management::Deployment::PackageVersionMetadataField const&)
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersionInfo::Id()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersionInfo::DisplayName()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersionInfo::Version()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersionInfo::Channel()
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::Collections::IVectorView<hstring> PackageVersionInfo::PackageFamilyNames()
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::Collections::IVectorView<hstring> PackageVersionInfo::ProductCodes()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalog PackageVersionInfo::PackageCatalog()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::CompareResult PackageVersionInfo::CompareToVersion(hstring versionString)
    {
        throw hresult_not_implemented();
    }
}
