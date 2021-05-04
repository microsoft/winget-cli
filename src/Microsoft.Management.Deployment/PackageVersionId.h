#pragma once
#include "PackageVersionId.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageVersionId : PackageVersionIdT<PackageVersionId>
    {
        PackageVersionId() = default;
        PackageVersionId(::AppInstaller::Repository::PackageVersionKey packageVersionKey);

        hstring AppCatalogId();
        hstring Version();
        hstring Channel();
    private:
        ::AppInstaller::Repository::PackageVersionKey m_packageVersionKey;
    };
}
