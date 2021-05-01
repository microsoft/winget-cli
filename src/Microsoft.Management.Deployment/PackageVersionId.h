#pragma once
#include "PackageVersionId.g.h"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageVersionId : PackageVersionIdT<PackageVersionId>
    {
        PackageVersionId() = default;

        hstring AppCatalogId();
        hstring Version();
        hstring Channel();
    };
}
