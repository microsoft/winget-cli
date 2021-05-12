#pragma once
#include "PackageVersionId.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("521b6efb-21b6-43fd-aea0-ce86fe94f0b2")]
    struct PackageVersionId : PackageVersionIdT<PackageVersionId>
    {
        PackageVersionId() = default;
        PackageVersionId(::AppInstaller::Repository::PackageVersionKey packageVersionKey);
        void Initialize(::AppInstaller::Repository::PackageVersionKey packageVersionKey);

        hstring AppCatalogId();
        hstring Version();
        hstring Channel();
    private:
        ::AppInstaller::Repository::PackageVersionKey m_packageVersionKey;
    };
}
