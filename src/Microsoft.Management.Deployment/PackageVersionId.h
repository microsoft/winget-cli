#pragma once
#include "PackageVersionId.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


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
