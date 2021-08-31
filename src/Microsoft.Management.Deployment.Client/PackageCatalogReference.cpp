// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <PackageCatalogReference.h>
#include "PackageCatalogReference.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    bool PackageCatalogReference::IsComposite()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogInfo PackageCatalogReference::Info()
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::ConnectResult> PackageCatalogReference::ConnectAsync()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::ConnectResult PackageCatalogReference::Connect()
    {
        throw hresult_not_implemented();
    }
}
