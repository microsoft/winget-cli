// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageCatalogReference.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageCatalogReference : PackageCatalogReferenceT<PackageCatalogReference>
    {
        PackageCatalogReference() = default;
        void Initialize(winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo);
        void Initialize(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions options);

        bool IsComposite();
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo Info();
        winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::ConnectResult> ConnectAsync();
        winrt::Microsoft::Management::Deployment::ConnectResult Connect();
        hstring AdditionalPackageCatalogArguments();
        void AdditionalPackageCatalogArguments(hstring const& value);
    private:
        winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions m_compositePackageCatalogOptions{ nullptr };
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo m_info{ nullptr };
        std::optional<std::string> m_compositeAdditionalPackageCatalogArguments;
    };
}
