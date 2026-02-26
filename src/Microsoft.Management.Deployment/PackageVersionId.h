// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageVersionId.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageVersionId : PackageVersionIdT<PackageVersionId>
    {
        PackageVersionId() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(::AppInstaller::Repository::PackageVersionKey packageVersionKey);
#endif

        hstring PackageCatalogId();
        hstring Version();
        hstring Channel();
        
#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ::AppInstaller::Repository::PackageVersionKey m_packageVersionKey{};
#endif
    };
}
