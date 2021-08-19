// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageCatalogInfo.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageCatalogInfo : PackageCatalogInfoT<PackageCatalogInfo>
    {
        PackageCatalogInfo() = default;
        void Initialize(const ::AppInstaller::Repository::SourceDetails& sourceDetails);
        ::AppInstaller::Repository::SourceDetails GetSourceDetails();

        hstring Id();
        hstring Name();
        hstring Type();
        hstring Argument();
        winrt::Windows::Foundation::DateTime LastUpdateTime();
        winrt::Microsoft::Management::Deployment::PackageCatalogOrigin Origin();
        winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel TrustLevel();
    private:
        ::AppInstaller::Repository::SourceDetails m_sourceDetails{};
    };
}
