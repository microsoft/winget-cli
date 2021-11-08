// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "CreateCompositePackageCatalogOptions.g.h"

namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct CreateCompositePackageCatalogOptions : CreateCompositePackageCatalogOptionsT<CreateCompositePackageCatalogOptions, implementation::CreateCompositePackageCatalogOptions>
    {
        auto ActivateInstance() const
        {
            return winrt::create_instance<winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions>(__uuidof(implementation::CreateCompositePackageCatalogOptions), CLSCTX_ALL);
        }
    };
}
