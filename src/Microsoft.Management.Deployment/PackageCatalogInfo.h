// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageCatalogInfo.g.h"
#include <winget/RepositorySource.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageCatalogInfo : PackageCatalogInfoT<PackageCatalogInfo>
    {
        PackageCatalogInfo() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(const ::AppInstaller::Repository::SourceDetails& sourceDetails);
        ::AppInstaller::Repository::SourceDetails& GetSourceDetails();
#endif

        hstring Id();
        hstring Name();
        hstring Type();
        hstring Argument();
        winrt::Windows::Foundation::DateTime LastUpdateTime();
        winrt::Microsoft::Management::Deployment::PackageCatalogOrigin Origin();
        winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel TrustLevel();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ::AppInstaller::Repository::SourceDetails m_sourceDetails;
#endif
    };
}
