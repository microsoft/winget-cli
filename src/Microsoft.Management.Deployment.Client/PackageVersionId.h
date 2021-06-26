// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageVersionId.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageVersionId : PackageVersionIdT<PackageVersionId>
    {
        PackageVersionId() = default;

        hstring PackageCatalogId();
        hstring Version();
        hstring Channel();
    };
}
