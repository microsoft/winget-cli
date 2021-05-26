// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageCatalogReference.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageCatalogReference : PackageCatalogReferenceT<PackageCatalogReference>
    {
        PackageCatalogReference() = default;
        void Initialize(
            ::AppInstaller::Repository::SourceDetails sourceDetails,
            winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo);
        void Initialize(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions options);

        winrt::Microsoft::Management::Deployment::PackageCatalogInfo Info();
        winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::ConnectResult> ConnectAsync();
        winrt::Microsoft::Management::Deployment::ConnectResult Connect();
    private:
        winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions m_compositePackageCatalogOptions{ nullptr };
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo m_info{ nullptr };
        std::shared_ptr<::AppInstaller::Repository::ISource> m_source;
        ::AppInstaller::Repository::SourceDetails m_sourceDetails{};
    };
}
