// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <PackageCatalogInfo.h>
#include "PackageCatalogInfo.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    hstring PackageCatalogInfo::Id()
    {
        throw hresult_not_implemented();
    }
    hstring PackageCatalogInfo::Name()
    {
        throw hresult_not_implemented();
    }
    hstring PackageCatalogInfo::Type()
    {
        throw hresult_not_implemented();
    }
    hstring PackageCatalogInfo::Argument()
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::DateTime PackageCatalogInfo::LastUpdateTime()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogOrigin PackageCatalogInfo::Origin()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel PackageCatalogInfo::TrustLevel()
    {
        throw hresult_not_implemented();
    }
}
